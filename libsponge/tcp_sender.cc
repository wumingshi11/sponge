#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno -_abs_ack; }

void TCPSender::fill_window() {
  if (_fin) return;
  uint16_t max_size = _window_size < TCPConfig::MAX_PAYLOAD_SIZE ? _window_size : TCPConfig::MAX_PAYLOAD_SIZE;
  TCPSegment seg;
  if (_next_seqno == 0) seg.header().syn = true;
  seg.header().seqno = wrap(_next_seqno, _isn);
  seg.payload() = _stream.read(max_size);
  _window_size -= seg.payload().size();

  if (_stream.input_ended() && _window_size > 0 && (!_sential || _abs_ack == _next_seqno)) {
    seg.header().fin = true;
    _fin = true;
  }
 
  if (seg.header().syn || seg.header().fin || seg.payload().size() != 0) {
    _segments_out.push(seg);
    _outstandingSeg.emplace(_next_seqno, outstandingseg{seg, _initial_retransmission_timeout, _initial_retransmission_timeout, _sential});
    _next_seqno += seg.length_in_sequence_space();
    if (_window_size > 0) fill_window();
  }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
  uint64_t tmp = unwrap(ackno, _isn, _next_seqno);
  if (tmp > _next_seqno ) return;
  if (tmp > _abs_ack) _abs_ack = tmp;
  while (!_outstandingSeg.empty() && 
         _outstandingSeg.cbegin()->first + _outstandingSeg.cbegin()->second.seg.length_in_sequence_space() 
         <= _abs_ack) {
    _outstandingSeg.erase(_outstandingSeg.cbegin());
    _retransmissions = 0;
  }
  _window_size = window_size - (_next_seqno - _abs_ack);
  if (_window_size == 0) {
    _window_size = 1;
    _sential = true;
  } else {
    _sential = false;
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
  auto &seg = *_outstandingSeg.begin();
  if (!_outstandingSeg.empty() && seg.second.remain <= ms_since_last_tick) {
      _retransmissions++;
      _segments_out.push(seg.second.seg);
      if (!seg.second.sential) seg.second.base = seg.second.base * 2;
      seg.second.remain = seg.second.base;
    } else {
      seg.second.remain -= ms_since_last_tick;
    }
  //fill_window();
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmissions; }

void TCPSender::send_empty_segment() {
    TCPHeader header;
    int b = 1;
    if (_stream.input_ended()) b = 2;
    header.ackno = wrap(_stream.bytes_written() + b, _isn);
    header.win = _stream.remaining_capacity();
    Buffer buffer{header.serialize()};
    TCPSegment seg;
    seg.parse(buffer);
    _segments_out.push(seg);
}
