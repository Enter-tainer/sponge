#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::write(const string &data) {
    size_t sz = _sender.stream_in().write(data);
    if (_receive_syn) {
        _sender.fill_window();
    }
    _write_segs(false);
    return sz;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    LogGuard _l("tcp connection tick");
    if (!_active) {
        cerr << "conn no longer active\n";
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _dirty_shutdown(true);
    }
    if (_receive_syn) {
        _sender.fill_window();
    }
    _write_segs(false);
}

void TCPConnection::end_input_stream() {
    LogGuard _l("tcp connection end_input_stream");
    _sender.stream_in().end_input();
    if (_receive_syn) {
        _sender.fill_window();
    }
    _write_segs(false);
}

void TCPConnection::connect() {
    LogGuard _l("tcp connection connect");

    _sender.fill_window();
    _write_segs(false);
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    LogGuard _l("tcp connection segment_received");
    if (!_active) {
        cerr << "conn no longer active\n";
        return;
    }
    _time_since_last_segment_received = 0;
    if (seg.header().syn) {
        _receive_syn = true;
    }
    if (seg.header().rst) {
        cerr << "peer send rst to me\n";
        _dirty_shutdown(false);
        // rst
    } else {
        if (!_receive_syn && seg.header().ack) {
            cerr << "peer send ack before syn\n";
            return;
        }
        _receiver.segment_received(seg);
        if (seg.header().ack) {
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }
        if (seg.length_in_sequence_space() != 0) {
            if (_receive_syn) {
                _sender.fill_window();
            }
            if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
            }
            _write_segs(false);
        }
    }
}
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _dirty_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::_write_segs(bool with_rst) {
    LogGuard _l("tcp connection _write_segs");
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
            seg.header().ack = true;
        }
        if (with_rst) {
            seg.header().rst = true;
        }
        _segments_out.push(seg);
    }
    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        cerr << "do not linger\n";
        _linger_after_streams_finish = false;
    }
    if (_receiver.stream_out().input_ended() && _sender.bytes_in_flight() == 0 && _sender.stream_in().eof()) {
        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
}

void TCPConnection::_dirty_shutdown(bool send_rst) {
    if (send_rst) {
        if (_receive_syn) {
            _sender.fill_window();
        }
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        _write_segs(true);
    }
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}
