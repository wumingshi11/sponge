#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (!seg.header().syn && abs_index == 0) return;
    if (seg.header().syn) {
      start = WrappingInt32(seg.header().seqno);
      abs_index = 1;
    } else {
      auto tmp = unwrap(seg.header().seqno, start, abs_index);
      if (tmp == 0) return;
      if (tmp > abs_index) {
        if (tmp - abs_index < _capacity) abs_index = tmp;
        else return;
      } else {
        if (abs_index - tmp < _capacity) abs_index = tmp;
        else return;
      } 
      
    }
    std::string data = seg.payload().copy();
    _reassembler.push_substring(data, abs_index - 1, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {   
  optional<WrappingInt32> result;
  if (abs_index > 0) {
    int b = 1;
    if (_reassembler.stream_out().input_ended()) b = 2;
    uint64_t next = _reassembler.stream_out().bytes_written();
    result = wrap(next + b, start);
  }
  return result;
}

size_t TCPReceiver::window_size() const { 
  return _capacity - (_reassembler.stream_out().bytes_written() - _reassembler.stream_out().bytes_read()); 
}
