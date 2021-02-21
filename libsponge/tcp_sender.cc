#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _newest_ackno);  // FIXME: is it correct?
    if (abs_ackno > _newest_ackno) {
        _newest_ackno = abs_ackno;
        for (auto i = _out_standing_segs.begin(); i != _out_standing_segs.end();) {
            auto next = i + 1;
            uint64_t last_byte_abs_seq_no =
                i->length_in_sequence_space() + unwrap(i->header().seqno, _isn, _newest_ackno);  // FIXME: correct?
            if (last_byte_abs_seq_no < _newest_ackno) {
                _out_standing_segs.erase(i);
                _receive_new_ack();
                _timer_enable = !_out_standing_segs.empty();
            }
            i = next;
        }
    }
    _window_size = window_size == 0 ? 1 : window_size;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPSender::send_empty_segment() {}
