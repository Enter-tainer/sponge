#include "byte_stream.hh"
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool ByteStream::push_byte(char c) {
    if (!is_full()) {
        int cur = (end++) % buffer.size();
        end %= buffer.size();
        buffer[cur] = c;
        ++written_bytes;
        empty = false;
        return true;
    }
    return false;
}
bool ByteStream::pop_byte(char &c) {
    if (!is_empty()) {
        int cur = (beg++) % buffer.size();
        beg %= buffer.size();
        c = buffer[cur];
        ++read_bytes;
        if (beg == end) {
            empty = true;
        }
        return true;
    }
    return false;
}

bool ByteStream::is_full() const { return beg == end && !empty; }

bool ByteStream::is_empty() const { return beg == end && empty; }
ByteStream::ByteStream(const size_t capacity) {
    buffer.resize(capacity);
    fill(buffer.begin(), buffer.end(), 0);
}

size_t ByteStream::write(const string &data) {
    size_t cnt = 0;
    for (cnt = 0; !is_full() && cnt < data.length(); ++cnt) {
        push_byte(data[cnt]);
    }
    return cnt;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res;
    size_t peek_cnt = len > buffer.size() ? buffer.size() : len;
    for (size_t i = beg, c = 0; c < peek_cnt; ++i, i %= buffer.size(), ++c) {
        res.push_back(buffer[i]);
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { this->read(len); }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string res;
    size_t peek_cnt = len > buffer.size() ? buffer.size() : len;
    for (size_t i = beg, c = 0; c < peek_cnt; ++i, i %= buffer.size(), ++c) {
        char t;
        pop_byte(t);
        res.push_back(t);
    }
    return res;
}

void ByteStream::end_input() { end_of_input = true; }

bool ByteStream::input_ended() const { return end_of_input; }

size_t ByteStream::buffer_size() const { return buffer.size() - remaining_capacity(); }

bool ByteStream::buffer_empty() const { return is_empty(); }

bool ByteStream::eof() const { return end_of_input && is_empty(); }

size_t ByteStream::bytes_written() const { return written_bytes; }

size_t ByteStream::bytes_read() const { return read_bytes; }

size_t ByteStream::remaining_capacity() const {
    if (is_full()) {
        return 0;
    }
    if (is_empty()) {
        return buffer.size();
    }
    return buffer.size() - (end + buffer.size() - beg) % buffer.size();
}
