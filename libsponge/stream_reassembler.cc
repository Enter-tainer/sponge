#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void StreamReassembler::add_new_seg(segment &new_seg, bool eof) {
    if (new_seg.idx > _first_unacceptable) {
        return;
    }
    int overflow_bytes = new_seg.idx + new_seg.len - _first_unacceptable;
    if (overflow_bytes > 0) {
        new_seg.data.resize(static_cast<size_t>(new_seg.len - overflow_bytes));
        new_seg.len = new_seg.data.length();
        eof = false;
    }
    if (new_seg.idx < _first_unassembled) {
        int new_len = new_seg.len - (_first_unassembled - new_seg.idx);
        if (new_len < 0) {
            return;
        }
        new_seg.len = new_len;
        new_seg.data = new_seg.data.substr(_first_unassembled - new_seg.idx, new_seg.len);
        new_seg.idx = _first_unassembled;
    }
    process_overlap(new_seg);
    _eof |= eof;
}

void StreamReassembler::process_overlap(segment &new_seg) {
    for (auto i = segs.begin(); i != segs.end();) {
        auto next = ++i;
        --i;
        if ((new_seg.idx >= i->idx && new_seg.idx < i->idx + i->len) ||
            (i->idx >= new_seg.idx && i->idx < new_seg.idx + new_seg.len)) {
            merge_seg(new_seg, *i);
            _un_asmed -= i->len;
            segs.erase(i);
        }
        i = next;
    }
    _un_asmed += new_seg.len;
    segs.insert(new_seg);
}

void StreamReassembler::merge_seg(segment &l, const segment &r) {
    size_t new_l = min(l.idx, r.idx);
    size_t new_r = max(l.idx + l.len, r.idx + r.len);
    segment ll = l, rr = r;
    if (l.idx != new_l) {
        swap(ll, rr);
    }
    string buffer;
    buffer.resize(new_r - new_l);
    for (size_t i = 0; i < ll.len; ++i) {
        buffer[i] = ll.data[i];
    }
    for (size_t i = rr.idx - ll.idx, j = 0; j < rr.len; ++i, ++j) {
        buffer[i] = rr.data[j];
    }
    l.data = std::move(buffer);
    l.idx = new_l;
    l.len = new_r - new_l;
}

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _first_unacceptable(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(string data, const size_t index, const bool eof) {
    _first_unread = _output.bytes_read();
    _first_unacceptable = _first_unread + _capacity;
    segment new_seg = {data.length(), index, std::move(data)};
    add_new_seg(new_seg, eof);
    write_ready_data();
    if (empty() && _eof) {
        _output.end_input();
    }
}

void StreamReassembler::write_single_seg(const segment &seg) {
    _output.write(seg.data);
    _first_unassembled += seg.len;
}

void StreamReassembler::write_ready_data() {
    while (!segs.empty() && segs.begin()->idx == _first_unassembled) {
        write_single_seg(*segs.begin());
        _un_asmed -= segs.begin()->len;
        segs.erase(segs.begin());
    }
}
size_t StreamReassembler::unassembled_bytes() const { return _un_asmed; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
