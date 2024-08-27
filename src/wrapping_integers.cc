#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint32_t wrap_value = (n + zero_point.raw_value_) % (1ULL << 32);
  return Wrap32 {wrap_value};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t point_raw = zero_point.raw_value_;
  uint64_t this_raw = this->raw_value_;
  uint64_t result1 = 0, result2 = 0, result3 = 0;

  auto count = (checkpoint) / (1ULL << 32);
  if (this_raw < point_raw) {
    if (!count)
      return this_raw + (1ULL << 32) - point_raw;
    if (count == 1) {
      result1 = this_raw + (1ULL << 32) * count - point_raw;
      result2 = this_raw + (1ULL << 32) * count - point_raw;
      result3 = this_raw + (1ULL << 32) * (count + 1) - point_raw;
    } else {
      result1 = this_raw + (1ULL << 32) * (count - 1) - point_raw;
      result2 = this_raw + (1ULL << 32) * count - point_raw;
      result3 = this_raw + (1ULL << 32) * (count + 1) - point_raw;
    }
  } else {
    if (count > 0 and count < (1ULL << 32)) {
      result1 = this_raw + (1ULL << 32) * (count - 1) - point_raw;
      result2 = this_raw + (1ULL << 32) * count - point_raw;
      result3 = this_raw + (1ULL << 32) * (count + 1) - point_raw;
    } else if (!count) {
      result1 = this_raw + (1ULL << 32) * count - point_raw;
      result2 = this_raw + (1ULL << 32) * count - point_raw;
      result3 = this_raw + (1ULL << 32) * (count + 1) - point_raw;
    } else if (count == (1ULL << 32)){
      result1 = this_raw + (1ULL << 32) * (count - 1) - point_raw;
      result2 = this_raw + (1ULL << 32) * count - point_raw;
      result3 = this_raw + (1ULL << 32) * count - point_raw;
    }
  }

  if (result1 >= checkpoint)
    return result1;
  else if (result1 < checkpoint and result2 >= checkpoint) {
    return checkpoint - result1 < result2 - checkpoint ? result1 : result2;
  } else if (result2 < checkpoint and result3 >= checkpoint) {
    return checkpoint - result2 < result3 - checkpoint ? result2 : result3;
  } else if (checkpoint > result3) {
    if (result3 <= (1ULL << 63)) {
      return checkpoint - result3 < result3 + (1ULL<<32) - checkpoint ? result3 : result3 + (1ULL<<32);
    }
  }
  return result3;
}
