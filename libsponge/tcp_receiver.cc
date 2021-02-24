#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const auto header = seg.header();
    if (_syn && header.syn) {
        return;
    }
    if (header.syn) {
        _syn = true;
        _isn = header.seqno.raw_value();
    }
    if (_syn && header.fin) {
        cerr << "fin received" << endl;
        _fin = true;
    }
    uint64_t abs_seqno = unwrap(header.seqno, WrappingInt32(_isn), _checkpoint);
    _reassembler.push_substring(seg.payload().copy(), header.syn ? 0 : abs_seqno - 1, _fin);
    _checkpoint = abs_seqno;
}
