#ifndef SQLINQ_TYPES_DATETIME_HPP_
#define SQLINQ_TYPES_DATETIME_HPP_

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

#if defined(__cpp_lib_format)
#include <format>
#define SQLINQ_HAS_FORMAT
#elif defined(FMT_VERSION)
#include <fmt/format.h>
#define SQLINQ_HAS_FORMAT
#endif

namespace sqlinq {
using Date = std::chrono::year_month_day;
using Time = std::chrono::hh_mm_ss<std::chrono::seconds>;
using Datetime = std::chrono::time_point<std::chrono::system_clock>;
using Timestamp = std::chrono::seconds;

std::string to_string(const Date &);
std::string to_string(const Time &);
std::string to_string(const Datetime &);
std::string to_string(const Timestamp &);

bool from_string(std::string_view, Date &);
bool from_string(std::string_view, Time &);
bool from_string(std::string_view, Datetime &);
bool from_string(std::string_view, Timestamp &);

std::ostream &operator<<(std::ostream &, const Date &);
std::ostream &operator<<(std::ostream &, const Time &);
std::ostream &operator<<(std::ostream &, const Datetime &);
std::ostream &operator<<(std::ostream &, const Timestamp &);

#ifdef SQLINQ_HAS_FORMAT
template <> struct std::formatter<Date> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }
  auto format(const sqlinq::Date &d, auto &ctx) {
    return std::format_to(ctx.out(), "{}", to_string(d));
  }
} template <> struct std::formatter<Time> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }
  auto format(const sqlinq::Time &t, auto &ctx) {
    return std::format_to(ctx.out(), "{}", to_string(t));
  }
} template <> struct std::formatter<Datetime> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }
  auto format(const sqlinq::Datetime &dt, auto &ctx) {
    return std::format_to(ctx.out(), "{}", to_string(dt));
  }
} template <> struct std::formatter<Timestamp> {
  constexpr auto parse(auto &ctx) { return ctx.begin(); }
  auto format(const sqlinq::Timestamp &d, auto &ctx) {
    return std::format_to(ctx.out(), "{}", to_string(d));
  }
}
#endif
} // namespace sqlinq

#endif // SQLINQ_TYPES_DATETIME_HPP_
