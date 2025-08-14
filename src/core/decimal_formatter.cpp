#include <sqlinq/types/decimal.hpp>

namespace sqlinq::details {

std::to_chars_result to_chars(char *first, char *last,
                              const DecimalTraits::value_type &v,
                              std::size_t scale) noexcept {
  std::to_chars_result result;
  if (first == nullptr || last == nullptr) {
    return {nullptr, std::errc::invalid_argument};
  }

  if (v == DecimalTraits::nan_sentinel) {
    const char nan_str[] = {'N', 'a', 'N'};
    const char *nan_str_last = nan_str + sizeof(nan_str);
    if (std::distance(first, last) < std::distance(nan_str, nan_str_last)) {
      return {first, std::errc::invalid_argument};
    }
    result.ptr = std::copy(nan_str, nan_str_last, first);
    result.ec = {};
    return result;
  }

  char sign = (v < 0) ? '-' : '+';
  char buf[DecimalTraits::max_str_length];
  DecimalTraits::value_type value = std::abs(v);

  char *begin = buf;
  char *end = buf + sizeof(buf);

  char *it = end - 1;
  char *sign_it = it - scale;
  *sign_it = '.';
  for (; it != begin; --it) {
    if (it == sign_it) {
      continue;
    }
    *it = (char)(value % 10) + '0';
    value = value / 10;
    if (value == 0 && it < sign_it) {
      break;
    }
  }

  if (sign == '-') {
    --it;
    *it = sign;
  }

  end = (scale == 0) ? end - 1 : end;
  auto src_len = std::distance(it, end);
  auto dst_len = std::distance(first, last);
  if (src_len > dst_len) {
    return {first, std::errc::invalid_argument};
  }

  result.ptr = std::copy(it, end, first);
  result.ec = std::errc{};
  return result;
}
} // namespace sqlinq::details
