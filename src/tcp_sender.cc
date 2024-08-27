#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequence_numbers_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_count_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  if (input_.has_error()) {
    auto send_message = make_empty_message();
    transmit(send_message);
    return;
  }

  if (!handshaked) {
    auto send_message = make_empty_message();
    send_message.SYN = true;

    if (input_.reader().is_finished() or input_.writer().is_closed()) {
      send_message.FIN = true;
      fin_send = true;
    }

    transmit(send_message);

    handshaked = true;

    segments_outstanding_.push(send_message);

    sequence_numbers_in_flight_ += static_cast<uint64_t>(send_message.sequence_length());
  } else {
    while (!fin_send and window_size > sequence_numbers_in_flight_) {
      auto payload = input_.reader().peek();

      size_t payload_size = min(min(static_cast<size_t>(window_size - sequence_numbers_in_flight_), payload.size()),
                                    static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE));

      payload = payload.substr(0, payload_size);

      if (payload_size or input_.reader().is_finished() or input_.writer().is_closed()) {
        auto send_message = make_empty_message();
        send_message.payload = payload;

        if (input_.reader().is_finished() or input_.writer().is_closed()) {
          if (input_.reader().peek().size() == payload_size) {
            send_message.FIN = true;
            fin_send = true;
          }
        }

        if (window_size < send_message.sequence_length() + sequence_numbers_in_flight_) {
          send_message.FIN = false;
          fin_send = false;
        }

        transmit(send_message);
        segments_outstanding_.push(send_message);

        sequence_numbers_in_flight_ += static_cast<uint64_t>(send_message.sequence_length());

        input_.reader().pop(payload.size());
      } else {
        break;
      }
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage segment;

  if (!handshaked) {
    segment.seqno = isn_;
  } else {
    segment.seqno = isn_ + 1 + input_.reader().bytes_popped();
  }

  if (input_.has_error())
    segment.RST = true;

  if (fin_send) {
    segment.seqno = segment.seqno + 1;
  }

  return segment;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if (msg.RST)
    input_.set_error();

  window_size = msg.window_size > 0 ? msg.window_size : 1;
  ack_window_size = msg.window_size;

  while (!segments_outstanding_.empty()) {
    if (segments_outstanding_.front().seqno + segments_outstanding_.front().sequence_length() == msg.ackno) {
      retransmissions_count_ = 0;
      RTO_ms_ = initial_RTO_ms_;
      time_since_last_transmission_ = 0;

      sequence_numbers_in_flight_ -= static_cast<uint64_t>(segments_outstanding_.front().sequence_length());
      segments_outstanding_.pop();
    } else if (segments_outstanding_.back().seqno + segments_outstanding_.back().sequence_length() == msg.ackno) {
      retransmissions_count_ = 0;
      RTO_ms_ = initial_RTO_ms_;
      time_since_last_transmission_ = 0;

      sequence_numbers_in_flight_ = 0;
      segments_outstanding_ = {};
    } else {
      break;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  time_since_last_transmission_ += ms_since_last_tick;

  // Check if retransmission timeout (RTO) has been reached
  if (!segments_outstanding_.empty() and time_since_last_transmission_ >= RTO_ms_) {
      // Retransmit the earliest unacknowledged segment
      transmit(segments_outstanding_.front());

      // Double the RTO for exponential backoff
      if (ack_window_size > 0)
        RTO_ms_ *= 2;

      // Reset the timer
      time_since_last_transmission_ = 0;

      //retransmissions count will increase
      retransmissions_count_++;
  }
}
