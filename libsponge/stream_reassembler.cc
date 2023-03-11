#include "stream_reassembler.hh"
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) ,streams(){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {

    for (size_t i = 0; i < data.size(); i++) {
      if (_output.bytes_written() > index + i) continue;
      if (i + index - _output.bytes_read() > _capacity) break;
      streams.emplace(data[i], index + i);
    }

    string s;
    size_t s_index = _output.bytes_written();
    while (!empty() && streams.begin()->index == s_index) {
      s.push_back(streams.begin()->c);
      streams.erase(streams.begin());
      s_index++;
    }
    _output.write(s);     

    if (eof) _eof_index = index + data.size();
    if (_output.bytes_written() == _eof_index) {
      _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return streams.size(); }

bool StreamReassembler::empty() const { return streams.empty(); }
