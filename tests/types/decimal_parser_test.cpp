#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sqlinq/types/decimal.hpp"

using namespace sqlinq;

TEST(DecimalParserTest, ParsesSimpleInteger) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "123";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 12300);
}

TEST(DecimalParserTest, ParsesDecimalWithCorrectScale) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "12.34";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 1234);
}

TEST(DecimalParserTest, ParsesNegativeNumber) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "-45.67";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, -4567);
}

TEST(DecimalParserTest, HandlesMissingFractionalPart) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "78.";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 78000);
}

TEST(DecimalParserTest, PadsFractionalZeros) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "9.1";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 9100);
}

TEST(DecimalParserTest, RejectsTooManyFractionalDigits) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "1.2345";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc::result_out_of_range);
  EXPECT_EQ(res.ptr, &sv[4]);
}

TEST(DecimalParserTest, RejectsInvalidCharacters) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "12.3x";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc::invalid_argument);
  EXPECT_EQ(res.ptr, &sv[4]);
}

TEST(DecimalParserTest, AcceptsLeadingPlusSign) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "+4.56";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 456);
}

TEST(DecimalParserTest, ParsesLeadingZeros) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "0000123.45";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 12345);
}

TEST(DecimalParserTest, ParsesZeroExactly) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "0.00";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 0);
}

TEST(DecimalParserTest, ParsesZeroWithScaleThree) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "0.0";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 0);
}

TEST(DecimalParserTest, ParsesZeroWithNoFractional) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "0";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 0);
}

TEST(DecimalParserTest, RejectsEmptyString) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc::invalid_argument);
  EXPECT_EQ(res.ptr, end);
}

TEST(DecimalParserTest, RejectsJustDot) {
  details::DecimalTraits::value_type value;
  std::string_view sv = ".";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc::invalid_argument);
  EXPECT_EQ(res.ptr, begin);
}

TEST(DecimalParserTest, RejectsOnlySign) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "-";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc::invalid_argument);
  EXPECT_EQ(res.ptr, end);
}

TEST(DecimalParserTest, RejectsDoubleSign) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "+-0.05";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 2);
  ASSERT_EQ(res.ec, std::errc::invalid_argument);
  EXPECT_EQ(res.ptr, &sv[1]);
}

TEST(DecimalParserTest, HandlesScaleZeroProperly) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "123";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 0);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 123);
}

TEST(DecimalParserTest, RejectsFractionWhenScaleZero) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "123.45";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 0);
  ASSERT_EQ(res.ec, std::errc::result_out_of_range);
  EXPECT_EQ(res.ptr, &sv[4]);
}

TEST(DecimalParserTest, AcceptsExactlyFittingFraction) {
  details::DecimalTraits::value_type value;
  std::string_view sv = "0.234";
  const char *begin = sv.data();
  const char *end = sv.data() + sv.size();
  auto res = details::from_chars(begin, end, value, 3);
  ASSERT_EQ(res.ec, std::errc{});
  EXPECT_EQ(res.ptr, end);
  EXPECT_EQ(value, 234);
}
