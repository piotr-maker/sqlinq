#ifndef SQLINQ_TYPES_DECIMAL_HPP_
#define SQLINQ_TYPES_DECIMAL_HPP_

#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ostream>

namespace sqlinq {

template <std::size_t M, std::size_t D> class decimal {
  static_assert(M != 0, "Decimal part length too short");
  static_assert(D != 0, "Fractional part length too short");
  static_assert(M <= 18, "Decimal part length too big");
  static_assert(D <= 19, "Fractional part length too big");

public:
  decimal() : decimal_(0), fractional_(0) {}

  explicit constexpr decimal(double value) {
    double dec;
    double frac = modf(value, &dec);
    fractional_ = round(frac * precision_);
    decimal_ = (decltype(decimal_))(dec);
  }

  constexpr decimal(std::string_view value)
      : decimal_(0), fractional_(0) {
    constexpr const char delimiter = '.';
    std::size_t pos = value.find(delimiter);
    if (pos != value.npos) {
      std::string_view sv = value.substr(pos + sizeof(delimiter));
      auto [ptr, ec] =
          std::from_chars(sv.data(), sv.data() + sv.size(), fractional_);
    }
    auto [ptr, ec] =
        std::from_chars(value.data(), value.data() + value.size(), decimal_);
  }

  int64_t raw() const noexcept {
    return decimal_ * precision_ + fractional_;
  }

  decimal operator+(const decimal &other) const {
    decltype(fractional_) f = this->fractional_ + other.fractional_;
    decltype(decimal_) d = this->decimal_ + other.decimal_ + f / precision_;
    return decimal{d, f % precision_};
  }

  decimal operator-(const decimal &other) const {
    decltype(fractional_) f =
        this->fractional_ + precision_ - other.fractional_;
    decltype(decimal_) d = this->decimal_ - other.decimal_ - 1;
    return decimal{d, f % precision_};
  }

  bool operator==(const decimal &other) const {
    return this->decimal_ == other.decimal_ &&
           this->fractional_ == other.fractional_;
  }

  friend std::ostream &operator<<(std::ostream &os, const decimal d) {
    os << d.decimal_ << '.' << std::setw(D) << std::setfill('0')
       << d.fractional_;
    return os;
  }

private:
  int64_t decimal_;
  uint64_t fractional_;
  constexpr static uint64_t precision_ = pow(10, D);

  decimal(int64_t dec, uint64_t frac) : decimal_(dec), fractional_(frac) {}
};

template <std::size_t M, std::size_t D>
std::string to_string(const decimal<M, D> &d) {
  std::stringstream ss;
  ss << d;
  return ss.str();
}
} // namespace sqlinq

#endif /* SQLINQ_TYPES_DECIMAL_HPP_ */
