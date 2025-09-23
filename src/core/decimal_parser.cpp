#include <cctype>
#include <sqlinq/types/decimal.hpp>

namespace sqlinq::details {
enum class State { Sign, Integer, Fractional };

std::from_chars_result from_chars(const char *first, const char *last,
                                  DecimalTraits::value_type &v,
                                  std::size_t scale) noexcept {
  State state = State::Sign;
  bool has_integer_digit = false;

  if (first == nullptr || last == nullptr) {
    return {nullptr, std::errc::invalid_argument};
  }

  char sign{};
  DecimalTraits::value_type value = 0;
  while (first != last && *first != '\0') {
    char c = *first;
    switch (state) {
    case State::Sign:
      state = State::Integer;
      if (c == '-' || c == '+') {
        sign = c;
        first++;
        continue;
      }
    [[fallthrough]];
    case State::Integer:
      if (isdigit(c)) {
        has_integer_digit = true;
        value = value * 10 + (c - '0');
      } else if (c == '.') {
        if (!has_integer_digit) {
          return {first, std::errc::invalid_argument};
        }
        state = State::Fractional;
      } else {
        return {first, std::errc::invalid_argument};
      }
      break;
    case State::Fractional:
      if (isdigit(c)) {
        if (scale == 0) {
          return {first, std::errc::result_out_of_range};
        }
        value = value * 10 + (c - '0');
        scale--;
      } else {
        return {first, std::errc::invalid_argument};
      }
      break;
    }
    first++;
  }

  if (!has_integer_digit) {
    return {first, std::errc::invalid_argument};
  }

  value = value * DecimalTraits::power_of_10(scale);
  v = value * ((sign == '-') ? -1 : 1);
  return {first, std::errc{}};
}
} // namespace sqlinq::detail
