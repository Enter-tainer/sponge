#include "byte_stream.hh"

#include <cstring>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : bytes(""), cap(capacity), used_len(0), is_end(false), has_writen(0), has_read(0) {}

size_t ByteStream::write(const string &data) {
    size_t remain_cap = remaining_capacity();
    if (remain_cap == 0)
        return 0;
    size_t write_len = min(remain_cap, data.length());
    bytes.append(data, 0, write_len);
    used_len += write_len;
    has_writen += write_len;
    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t peek_len = min(len, used_len);
    return bytes.substr(0, peek_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t pop_len = min(len, used_len);
    bytes.erase(0, pop_len);
    used_len -= pop_len;
    has_read += pop_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t read_len = min(len, used_len);
    string ret = peek_output(read_len);
    pop_output(read_len);
    return ret;
}

void ByteStream::end_input() { is_end = true; }

bool ByteStream::input_ended() const { return is_end; }

size_t ByteStream::buffer_size() const { return used_len; }

bool ByteStream::buffer_empty() const { return used_len == 0; }

bool ByteStream::eof() const { return is_end && used_len == 0; }

size_t ByteStream::bytes_written() const { return has_writen; }

size_t ByteStream::bytes_read() const { return has_read; }

size_t ByteStream::remaining_capacity() const { return cap - used_len; }
