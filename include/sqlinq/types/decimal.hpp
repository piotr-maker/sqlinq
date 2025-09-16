#ifndef SQLINQ_TYPES_DECIMAL_HPP_
#define SQLINQ_TYPES_DECIMAL_HPP_

#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <ostream>

namespace sqlinq {

namespace details {
struct DecimalTraits {
  using value_type = int64_t;
  static constexpr std::size_t max_precision = 18;
  static constexpr std::size_t max_str_length = max_precision + 2;
  static constexpr value_type nan_sentinel =
      std::numeric_limits<value_type>::min();

  static constexpr value_type power_of_10(std::size_t exp) {
    return exp == 0 ? 1 : 10 * power_of_10(exp - 1);
  }
};

struct DecimalRuntime {
  using value_type = DecimalTraits::value_type;

  value_type value;
  value_type scale;
};

std::from_chars_result from_chars(const char *first, const char *last,
                                  DecimalTraits::value_type &v,
                                  std::size_t scale) noexcept;

std::to_chars_result to_chars(char *first, char *last,
                              const DecimalTraits::value_type &v,
                              std::size_t scale) noexcept;

inline std::string to_string(const DecimalRuntime &dr) {
  char buf[details::DecimalTraits::max_str_length + 1];
  auto [ptr, ec] =
      details::to_chars(buf, buf + sizeof(buf), dr.value, (unsigned)dr.scale);
  assert(ec == std::errc{} && "Should never happened");
  return std::string{buf, ptr};
}
} // namespace details

template <std::size_t Precision, std::size_t Scale> class Decimal {
  static_assert(Precision != 0,
                "Decimal<Precision, Scale>: Precision must be > 0");
  static_assert(
      Precision <= details::DecimalTraits::max_precision,
      "Decimal<Precision, Scale>: Precision must be <= MAX_PRECISION");
  static_assert(Scale < Precision,
                "Decimal<Precision, Scale>: Scale must be < Precision");

public:
  using Traits = details::DecimalTraits;
  using value_type = Traits::value_type;
  static constexpr value_type nan_sentinel = Traits::nan_sentinel;

  explicit constexpr Decimal(int v = 0) {
    value_ = static_cast<value_type>(v) * power_of_10(Scale);
    if (value_ > max_val_ || value_ < min_val_) {
      value_ = nan_sentinel;
    }
  }

  explicit Decimal(double value) {
    value_ = (value_type)(round(value * power_of_10(Scale)));
  }

  Decimal(std::string_view v) {
    auto [ptr, ec] =
        details::from_chars(v.data(), v.data() + v.size(), value_, Scale);
    if (ec != std::errc{}) {
      value_ = nan_sentinel;
    }
  }

  static constexpr Decimal from_raw(value_type val) { return Decimal(val, 0); }
  constexpr bool is_nan() const noexcept { return value_ == nan_sentinel; }

  bool operator==(const Decimal &other) const noexcept {
    return this->value_ == other.value_;
  }
  bool operator!=(const Decimal &other) const noexcept {
    return !(*this == other);
  }
  bool operator<(const Decimal &other) const noexcept {
    return this->value_ < other.value_;
  }
  bool operator>(const Decimal &other) const noexcept {
    return this->value_ > other.value_;
  }
  explicit constexpr operator value_type() const noexcept {
    return this->value_;
  }

  friend std::ostream &operator<<(std::ostream &os, const Decimal d) {
    value_type raw = static_cast<value_type>(d);
    char buf[Traits::max_str_length + 1];
    auto [ptr, ec] = details::to_chars(buf, buf + sizeof(buf), raw, Scale);
    assert(ec == std::errc{} && "Should never happened");
    *ptr = '\0';
    os << buf;
    return os;
  }

private:
  value_type value_;
  value_type scale_ = Scale;

  constexpr Decimal(int64_t v, int /*tag*/) : value_(v) {}

  static constexpr value_type power_of_10(std::size_t exp) {
    return Traits::power_of_10(exp);
  }

  static constexpr value_type max_val_ = power_of_10(Precision) - 1;
  static constexpr value_type min_val_ = -power_of_10(Precision) + 1;

public:
  static constexpr Decimal max() noexcept { return Decimal(max_val_, 0); }
  static constexpr Decimal min() noexcept { return Decimal(min_val_, 0); }
};

template <std::size_t P, std::size_t S>
std::string to_string(const Decimal<P, S> &d) {
  details::DecimalTraits::value_type raw = static_cast<int64_t>(d);
  char buf[details::DecimalTraits::max_str_length + 1];
  auto [ptr, ec] = details::to_chars(buf, buf + sizeof(buf), raw, S);
  assert(ec == std::errc{} && "Should never happened");
  return std::string{buf, ptr};
}
} // namespace sqlinq

#endif /* SQLINQ_TYPES_DECIMAL_HPP_ */
