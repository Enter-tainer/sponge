#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
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
    , _stream(capacity) {
    _rto = _initial_retransmission_timeout;
}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::_send_segment(TCPSegment &seg) {
    LogGuard _l("_send_segment");
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _segments_out.push(seg);
    _out_standing_segs.push_back(seg);
    _bytes_in_flight += seg.length_in_sequence_space();
    // cerr << "sending" << seg.header().summary() << " with length: " << seg.payload().size()
    // << " with data: " << seg.payload().copy() << "\n";
    _start_timer_if_not_running();
}
void TCPSender::fill_window() {
    LogGuard _l("filling window");
    if (!_syn_sent) {
        // cerr << "sending syn\n";
        _syn_sent = true;
        TCPSegment syn;
        syn.header().syn = true;
        _send_segment(syn);
        return;
    }
    if (_syn_sent && !_out_standing_segs.empty() && _out_standing_segs.begin()->header().syn) {
        // cerr << "syn sent but not acked\n";
        // syn not acked
        return;
    }
    if (_stream.buffer_empty() && !_stream.eof()) {
        // cerr << "buffer is empty, waiting for new data\n";
        return;
    }

    if (_fin_sent) {
        // cerr << "fin is sent\n";
        return;
    }

    if (_window_size != 0) {
        // cerr << "window size ==" << _window_size << " , sending data\n";
        // cerr << "current window size: " << _window_size << "\n";
        while (_get_free_space()) {
            // cerr << "current free space: " << _get_free_space() << "\n";
            TCPSegment seg;
            size_t len = min(TCPConfig::MAX_PAYLOAD_SIZE, min(_stream.buffer_size(), _get_free_space()));
            seg.payload() = _stream.read(len);
            if (_stream.eof() && !_fin_sent && _get_free_space() > len) {
                seg.header().fin = true;
                _fin_sent = true;
            }
            _send_segment(seg);
            if (_stream.buffer_empty()) {
                break;
            }
        }
    } else if (_get_free_space() == 0) {
        // cerr << "0-window size\n";
        TCPSegment seg;
        if (_stream.eof()) {
            seg.header().fin = true;
            _fin_sent = true;
            _send_segment(seg);
        } else if (!_stream.buffer_empty()) {
            seg.payload() = _stream.read(1);
            _send_segment(seg);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    LogGuard _l("ack_received");
    uint64_t abs_ackno = unwrap(ackno, _isn, _newest_ackno);  // FIXME: is it correct?
    // cerr << "getting ackno: " << ackno << "\n";
    // cerr << "ackno's abs value: " << abs_ackno << "\n";
    // cerr << "current newest_ackno: " << _newest_ackno << "\n";
    if (abs_ackno >= _newest_ackno && abs_ackno <= _next_seqno) {
        // cerr << "update newest to current value\n";
        _bytes_in_flight -= abs_ackno - _newest_ackno;
        _newest_ackno = abs_ackno;
        for (auto i = _out_standing_segs.begin(); i != _out_standing_segs.end();) {
            uint64_t last_byte_abs_seq_no =
                i->length_in_sequence_space() + unwrap(i->header().seqno, _isn, _newest_ackno);  // FIXME: correct?
            if (last_byte_abs_seq_no <= _newest_ackno) {
                // cerr << "erase outstanding seg" << i->header().summary() << " with length: " << i->payload().size()
                // << "\n";
                _out_standing_segs.erase(i);
                _receive_new_ack();
                if (!_out_standing_segs.empty()) {
                    _start_timer_if_not_running();
                };
            } else {
                i++;
            }
        }
    }
    _window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    LogGuard _l("sender tick");
    // cerr << ms_since_last_tick << "ms passed\n";
    if (!_timer_enable) {
        // cerr << "timer is disabled\n";
        return;
    }
    bool expire = _time_pass(ms_since_last_tick);
    if (expire) {
        // expire
        auto earliest = *min_element(
            _out_standing_segs.begin(), _out_standing_segs.end(), [this](const TCPSegment &a, const TCPSegment &b) {
                return unwrap(a.header().seqno, _isn, _newest_ackno) < unwrap(b.header().seqno, _isn, _newest_ackno);
            });
        // cerr << "resending " << earliest.header().summary() << "with data: " << earliest.payload().copy() << "\n";
        _segments_out.push(earliest);
        if (_window_size != 0 || earliest.header().syn) {
            _double_rto_and_start_timer();
        } else {
            _start_timer_if_not_running();
        }
    }
}

void TCPSender::send_empty_segment() {
    TCPSegment res;
    TCPHeader &header = res.header();
    header.seqno = wrap(_next_seqno, _isn);
    _segments_out.push(res);
}
