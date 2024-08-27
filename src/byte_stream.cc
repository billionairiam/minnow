#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_(capacity) {
  buffer_.reserve(capacity);
}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if (closed_ || error_) {
    return;
  }

  uint64_t space_left = available_capacity();
  uint64_t to_push = std::min(space_left, static_cast<uint64_t>(data.size()));
  buffer_ += data.substr(0, to_push);

  bytes_pushed_ += to_push;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ and buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  if (buffer_.empty()) {
    return std::string_view();
  }

  string_view sv( buffer_.data() , this->bytes_buffered());
  return sv;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if (has_error()) {
    return;
  }

  if (len > buffer_.size()) {
    len = buffer_.size();
  }

  bytes_popped_ += len;

  buffer_.erase( buffer_.begin() , buffer_.begin() + len );
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}
