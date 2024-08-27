#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if (message.RST) {
    reassembler_.reader().set_error();
  }

  if (message.SYN) {
    reassembler_.insert(0, message.payload, message.FIN);
    zero_point = message.seqno;
  } else {
    if (zero_point.has_value()) {
      uint64_t first_index = message.seqno.unwrap(zero_point.value(), reassembler_.writer().bytes_pushed());
      reassembler_.insert(first_index - 1, message.payload, message.FIN);
    }
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message {
    zero_point.has_value() ? zero_point.value() + reassembler_.writer().bytes_pushed() + 1 : zero_point,
    static_cast<uint16_t>(reassembler_.writer().available_capacity() > UINT16_MAX ?
    UINT16_MAX:
    reassembler_.writer().available_capacity()),
    reassembler_.writer().has_error(),
  };

  if (reassembler_.writer().is_closed() and message.ackno.has_value())
    message.ackno = message.ackno.value() + 1;

  return message;
}
