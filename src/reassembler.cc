#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if (is_last_substring) {
    last_index = first_index + data.size();
  }

  auto bytes_pushed = output_.writer().bytes_pushed();
  if (first_index <= bytes_pushed) {
    if (first_index + data.size() > bytes_pushed) {
      auto temp_str = data.substr(bytes_pushed - first_index);
      output_.writer().push(temp_str);

      vector<int> remove_index;
      for (size_t i = 0; i < pending_assemble.size(); ++i) {
        bytes_pushed = output_.writer().bytes_pushed();
        if (pending_assemble[i].first <= bytes_pushed) {
          if (pending_assemble[i].first +
              pending_assemble[i].second.size() > bytes_pushed) {
            temp_str = pending_assemble[i].second.substr(bytes_pushed -
                                                         pending_assemble[i].first);
            output_.writer().push(temp_str);
          }

          remove_index.push_back(i);
        }
      }

      if (remove_index.size()) {
        pending_assemble.erase(pending_assemble.begin() + remove_index[0],
                               pending_assemble.begin() + remove_index[remove_index.size() - 1] + 1);
      }

      uint64_t temp_pending = 0;
      for (size_t i = 0; i < pending_assemble.size(); ++i) {
        temp_pending += pending_assemble[i].second.size();
      }
      bytes_pending_ = temp_pending;
    }
  } else {
    if (first_index < output_.writer().available_capacity() + output_.writer().bytes_pushed()) {
      pending_assemble.push_back({first_index, data.substr(0,
      output_.writer().available_capacity() + output_.writer().bytes_pushed() - first_index)});
    }

    sort(pending_assemble.begin(), pending_assemble.end());

    vector<std::pair<uint64_t, std::string>> merged;
    for (size_t i = 0; i < pending_assemble.size(); ++i) {
      uint64_t L = pending_assemble[i].first, R = pending_assemble[i].second.size() + L;
        if (!merged.size() ||
            merged.back().second.size() + merged.back().first < L) {
          merged.push_back({L, pending_assemble[i].second});
        } else {
          uint64_t R2 = merged.back().first + merged.back().second.size();
          if (R2 < R) {
            merged.back().second += pending_assemble[i].second.substr(R2 - L);
          }
        }
    }

    uint64_t temp_pending = 0;
    for (size_t i = 0; i < merged.size(); ++i) {
      temp_pending += merged[i].second.size();
    }
    bytes_pending_ = temp_pending;

    pending_assemble = merged;
  }

  if (last_index != -1 and static_cast<uint64_t>( last_index ) == output_.writer().bytes_pushed())
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pending_;
}
