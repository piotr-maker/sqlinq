#ifndef SQLINQ_TYPES_DECIMAL_HPP_
#define SQLINQ_TYPES_DECIMAL_HPP_

#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ostream>

namespace sqlinq {

template <std::size_t M, std::size_t D>
class decimal {
  static_assert(M != 0, "Decimal part length too short");
  static_assert(D != 0, "Fractional part length too short");
  static_assert(M <= 18, "Decimal part length too big");
  static_assert(D <= 19, "Fractional part length too big");

public:
  decimal() : decimal_(0), fractal_(0) {}

  explicit constexpr decimal(double value) {
    double dec;
    double frac = modf(value, &dec);
    fractal_ = round(frac * precision_);
    decimal_ = (decltype(decimal_))(dec);
  }

  explicit constexpr decimal(std::string_view value) : decimal_(0), fractal_(0) {
    constexpr const char delimiter = '.';
    std::size_t pos = value.find(delimiter);
    if(pos != value.npos) {
      std::string_view sv = value.substr(pos + sizeof(delimiter));
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), fractal_);
    }
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), decimal_);
  }

  decimal operator+(const decimal& other) const {
    decltype(fractal_) f = this->fractal_ + other.fractal_;
    decltype(decimal_) d = this->decimal_ + other.decimal_ + f / precision_;
    return decimal{d, f % precision_};
  }

  decimal operator-(const decimal& other) const {
    decltype(fractal_) f = this->fractal_ + precision_ - other.fractal_;
    decltype(decimal_) d = this->decimal_ - other.decimal_ - 1;
    return decimal{d, f % precision_};
  }

  friend std::ostream& operator<<(std::ostream& os, const decimal d) {
    os << d.decimal_ << '.' << std::setw(D) << std::setfill('0') << d.fractal_;
    return os;
  }

private:
  int64_t decimal_;
  uint64_t fractal_;
  constexpr static uint64_t precision_ = pow(10, D);

  decimal(int64_t dec, uint64_t frac) : decimal_(dec), fractal_(frac) {}
};
} // namespace sqlinq

#endif /* SQLINQ_TYPES_DECIMAL_HPP_ */
