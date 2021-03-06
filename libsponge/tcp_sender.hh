#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "log_guard.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout, _rto{}, _rt_cnt{};
    int64_t _timer{};

    bool _timer_enable{false};

    std::vector<TCPSegment> _out_standing_segs{};

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    // [ack, window_size + ack)
    uint64_t _window_size{1}, _newest_ackno{}, _bytes_in_flight{};

    bool _syn_sent{false}, _fin_sent{false};

    void _receive_new_ack() {
        _rto = _initial_retransmission_timeout;
        _rt_cnt = 0;
        if (!_out_standing_segs.empty()) {
            _timer_enable = true;
            _timer = _rto;
        } else {
            _timer_enable = false;
        }
    }

    void _start_timer_if_not_running() {
        LogGuard _l("start timer");
        if (!_timer_enable) {
            // std::cerr << "starting timer, timeout: " << _rto << "\n";
            _timer_enable = true;
            _timer = _rto;
        } else {
            // std::cerr << "timer already started\n";
        }
    }

    void _double_rto_and_start_timer() {
        ++_rt_cnt;
        _rto *= 2;
        _start_timer_if_not_running();
    }

    // return true if timer expired
    // if it is expired, stop the timer
    bool _time_pass(unsigned int time) {
        LogGuard _l("time pass");
        // std::cerr << time << " ms passed\n";
        _timer -= time;
        if (_timer <= 0) {
            // std::cerr << "expired\n";
            _timer_enable = false;
            return true;
        }
        return false;
    }

    void stop_timer() { _timer_enable = false; }

    uint64_t _get_free_space() { return _window_size - bytes_in_flight(); }

    void _send_segment(TCPSegment &seg);

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const { return _rt_cnt; };

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
