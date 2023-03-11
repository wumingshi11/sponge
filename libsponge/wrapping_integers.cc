#include "wrapping_integers.hh"
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return WrappingInt32{static_cast<uint32_t>(n + isn.raw_value())};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //DUMMY_CODE(n, isn, checkpoint);
    const uint64_t uint32_max = UINT32_MAX;
    uint64_t absn = static_cast<uint64_t>(n - isn) & uint32_max;
    uint64_t base =  checkpoint & ~(uint32_max);
    uint64_t value = base + absn;
    uint64_t value2;
    if (value < checkpoint) {
      value2 = value + uint32_max + 1;
      if (value2 - checkpoint < checkpoint - value) return value2;
    } else {
      if (value < uint32_max + 1) return value;
      value2 = value - (uint32_max + 1);
      if (value - checkpoint > checkpoint - value2) return value2;
    }
    return value;
}
