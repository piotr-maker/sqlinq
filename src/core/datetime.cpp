#include "sqlinq/types/datetime.hpp"
#include <charconv>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>

#ifdef _WIN32
  #define timegm _mkgmtime
#endif

namespace sqlinq {

using namespace std;

std::string to_string(const Date &d) {
  std::ostringstream oss;
  oss << std::setw(4) << int(d.year()) << '-'
    << std::setw(2) << std::setfill('0') << unsigned(d.month()) << '-'
    << std::setw(2) << std::setfill('0') << unsigned(d.day());
  return oss.str();
};

std::string to_string(const Time &t) {
  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << t.hours().count() << ':'
    << std::setw(2) << std::setfill('0') << t.minutes().count() << ':'
    << std::setw(2) << std::setfill('0') << t.seconds().count();
  return oss.str();
};

std::string to_string(const Datetime &dt) {
  std::time_t tt = std::chrono::system_clock::to_time_t(dt);
  std::tm tm = *std::gmtime(&tt);
  char buf[20];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
  return std::string{buf};
}

std::string to_string(const Timestamp &ts) {
  return std::to_string(ts.count());
}

bool from_string(std::string_view str, Date &date) {
  int y;
  unsigned int m, d;
  if (std::sscanf(str.data(), "%d-%u-%u", &y, &m, &d) != 3)
    return false;
  date = Date{chrono::year{y}, chrono::month{m}, chrono::day{d}};
  return date.ok();
}

bool from_string(std::string_view str, Time &t) {
  int h, m, s;
  if (std::sscanf(str.data(), "%d:%d:%d", &h, &m, &s) != 3)
    return false;
  t = Time{chrono::hours{h} + chrono::minutes{m} + chrono::seconds{s}};
  return true;
}

bool from_string(std::string_view str, Datetime &dt) {
  std::tm tm{};
  if (str.size() != 19 || !strchr(str.data(), ' '))
    return false;
  std::istringstream iss{std::string{str}};
  iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  if (iss.fail())
    return false;
  dt = std::chrono::system_clock::from_time_t(timegm(&tm));
  return true;
}

bool from_string(std::string_view str, Timestamp &ts) {
  long long v;
  const char *first = str.data();
  const char *last = first + str.size();
  auto [ptr, ec] = std::from_chars(first, last, v);
  if (ec != std::errc{} || ptr != last)
    return false;
  ts = Timestamp{v};
  return true;
}

std::ostream &operator<<(std::ostream &os, const Date &d) {
  return os << to_string(d);
}

std::ostream &operator<<(std::ostream &os, const Time &t) {
  return os << to_string(t);
}

std::ostream &operator<<(std::ostream &os, const Datetime &dt) {
  return os << to_string(dt);
}

std::ostream &operator<<(std::ostream &os, const Timestamp &ts) {
  return os << to_string(ts);
}
}
