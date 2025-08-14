#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sqlinq/types/decimal.hpp"

using namespace sqlinq;

class DecimalFormatterTest : public ::testing::Test {
protected:
  using Traits = details::DecimalTraits;
  using value_type = Traits::value_type;
  static constexpr value_type nan_sentinel = Traits::nan_sentinel;

  char buf_[Traits::max_str_length + 1];
  char *first_ = buf_;
  char *last_ = buf_ + sizeof(buf_);

  void SetUp() override {
    memset(buf_, '\0', sizeof(buf_));
  }

  void TearDown() override {}
};

TEST_F(DecimalFormatterTest, SimplePositive) {
  value_type v = 12345;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 2);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[6]);
  EXPECT_STREQ("123.45", buf_);
}

TEST_F(DecimalFormatterTest, NegativeValue) {
  value_type v = -12345;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 2);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[7]);
  EXPECT_STREQ("-123.45", buf_);
}

TEST_F(DecimalFormatterTest, ZeroDifferentScales) {
  value_type v = 0;
  std::to_chars_result res = details::to_chars(first_, last_, v, 0);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("0", buf_);

  res = details::to_chars(first_, last_, v, 2);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("0.00", buf_);

  res = details::to_chars(first_, last_, v, 4);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("0.0000", buf_);
}

TEST_F(DecimalFormatterTest, TrailingZeros) {
  value_type v = 42000;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 3);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[6]);
  EXPECT_STREQ("42.000", buf_);
}

TEST_F(DecimalFormatterTest, IntegerAndFraction) {
  value_type v1 = 10001;
  std::to_chars_result res = details::to_chars(first_, last_, v1, 2);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("100.01", buf_);

  value_type v2 = 10000;
  res = details::to_chars(first_, last_, v2, 2);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("100.00", buf_);
}

TEST_F(DecimalFormatterTest, LeadingFractionZeros) {
  value_type v1 = 10001;
  std::to_chars_result res = details::to_chars(first_, last_, v1, 4);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("1.0001", buf_);

  value_type v2 = 10010;
  res = details::to_chars(first_, last_, v2, 4);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("1.0010", buf_);

  value_type v3 = 10100;
  res = details::to_chars(first_, last_, v3, 4);
  ASSERT_EQ(res.ec, std::errc{});
  *res.ptr = '\0';
  EXPECT_STREQ("1.0100", buf_);
}

TEST_F(DecimalFormatterTest, SmallFractionalValue) {
  value_type v = 1;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 6);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[8]);
  EXPECT_STREQ("0.000001", buf_);
}

TEST_F(DecimalFormatterTest, NegativeWithZeros) {
  value_type v = -100;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 5);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[8]);
  EXPECT_STREQ("-0.00100", buf_);
}

TEST_F(DecimalFormatterTest, NaNValue) {
  value_type v = nan_sentinel;
  auto [ptr, ec] = details::to_chars(first_, last_, v, 5);
  ASSERT_EQ(ec, std::errc{});
  EXPECT_EQ(ptr, &buf_[3]);
  EXPECT_STREQ("NaN", buf_);
}
