#ifndef SQLINQ_TYPES_DATETIME_HPP_
#define SQLINQ_TYPES_DATETIME_HPP_

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

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
} // namespace sqlinq

#endif // SQLINQ_TYPES_DATETIME_HPP_
