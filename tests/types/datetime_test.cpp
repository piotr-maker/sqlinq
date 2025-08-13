#include "sqlinq/types/datetime.hpp"
#include <gtest/gtest.h>

using namespace sqlinq;

TEST(DateConversion, ToStringAndFromString) {
  Date d{std::chrono::year{2023}, std::chrono::month{12}, std::chrono::day{31}};
  std::string str = to_string(d);
  EXPECT_EQ(str, "2023-12-31");

  Date d2;
  ASSERT_TRUE(from_string(str, d2));
  EXPECT_EQ(int(d2.year()), 2023);
  EXPECT_EQ(unsigned(d2.month()), 12);
  EXPECT_EQ(unsigned(d2.day()), 31);
}

TEST(DateConversion, FromStringInvalidFormat) {
  Date d;
  EXPECT_FALSE(from_string("2023-13-01", d));
  EXPECT_FALSE(from_string("2024-04-31", d));
  EXPECT_FALSE(from_string("2023-02-29", d));
  EXPECT_FALSE(from_string("invalid", d));
}

TEST(TimeConversion, ToStringAndFromString) {
  Time t{std::chrono::hours{9} + std::chrono::minutes{8} +
         std::chrono::seconds{7}};
  std::string str = to_string(t);
  EXPECT_EQ(str, "09:08:07");

  Time t2;
  EXPECT_TRUE(from_string(str, t2));
  EXPECT_EQ(t2.hours().count(), 9);
  EXPECT_EQ(t2.minutes().count(), 8);
  EXPECT_EQ(t2.seconds().count(), 7);
}

TEST(TimeConversion, FromStringInvalidFormat) {
  Time t;
  EXPECT_FALSE(from_string("99:99", t));
  EXPECT_FALSE(from_string("invalid", t));
}

TEST(DatetimeConversion, ToStringAndFromString) {
  // Set date: 2023-03-25 14:30:00
  std::tm tm{};
  tm.tm_year = 2023 - 1900;
  tm.tm_mon = 3 - 1;
  tm.tm_mday = 25;
  tm.tm_hour = 14;
  tm.tm_min = 30;
  tm.tm_sec = 0;

  std::time_t time = timegm(&tm);
  Datetime dt = std::chrono::system_clock::from_time_t(time);
  std::string str = to_string(dt);
  EXPECT_EQ(str, "2023-03-25 14:30:00");

  Datetime dt2;
  ASSERT_TRUE(from_string(str, dt2));
  std::time_t t2 = std::chrono::system_clock::to_time_t(dt2);
  std::tm tm2 = *gmtime(&t2);

  EXPECT_EQ(tm2.tm_year, tm.tm_year);
  EXPECT_EQ(tm2.tm_mon, tm.tm_mon);
  EXPECT_EQ(tm2.tm_mday, tm.tm_mday);
  EXPECT_EQ(tm2.tm_hour, tm.tm_hour);
  EXPECT_EQ(tm2.tm_min, tm.tm_min);
  EXPECT_EQ(tm2.tm_sec, tm.tm_sec);
}

TEST(DatetimeConversion, FromStringInvalidFormat) {
  Datetime dt;
  EXPECT_FALSE(from_string("2023-02-30 25:61:61", dt));
  EXPECT_FALSE(from_string("invalid", dt));
}

TEST(TimestampConversion, ToStringAndFromString) {
  Timestamp ts{123456789};
  std::string str = to_string(ts);
  EXPECT_EQ(str, "123456789");

  Timestamp ts2;
  EXPECT_TRUE(from_string(str, ts2));
  EXPECT_EQ(ts2.count(), 123456789);
}

TEST(TimestampConversion, FromStringInvalidFormat) {
  Timestamp ts;
  EXPECT_FALSE(from_string("abc", ts));
}
