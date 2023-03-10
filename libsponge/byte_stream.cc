#include "byte_stream.hh"
#include <iostream>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : buf(capacity), maxsize(capacity) { 
  writeable = capacity;
}

size_t ByteStream::write(const string &data) {
    size_t n = 0;
    for(char c : data) {
        if (writeable == 0) {
          cout << "writeable = 0" << write_index << maxsize <<endl;
          break;
          //end_input();
        }
        buf[(write_index++) % maxsize] = c;
        n++;
        writeable--;
    }
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string sub;
    for (uint32_t i = 0; i < len; i++) {
      if (read_index + i == write_index) break;
      sub.push_back(buf[(read_index + i) % maxsize]);
    }
    return sub;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
   if (len > buffer_size()) {
      set_error();
      return;
    }
    read_index = read_index + len;
    writeable += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto sub = peek_output(len);
    pop_output(sub.size());
    return sub;
}

void ByteStream::end_input() {_input_ended = true;}

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return write_index - read_index; }

bool ByteStream::buffer_empty() const { return writeable == maxsize; }

bool ByteStream::eof() const { return write_index == read_index && _input_ended; }

size_t ByteStream::bytes_written() const { return write_index; }

size_t ByteStream::bytes_read() const { return read_index; }

size_t ByteStream::remaining_capacity() const {
   if (write_index < read_index) {
     return read_index - write_index - 1;
   };
   return maxsize - (write_index -  read_index);
}
