#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "sqlinq/types/decimal.hpp"

using namespace sqlinq;

TEST(DecimalTest, ConstructsFromDouble) {
  Decimal<6, 2> d(1.237);
  EXPECT_EQ(124, static_cast<int64_t>(d));
  EXPECT_EQ(to_string(d), "1.24");
}

TEST(DecimalTest, ParsesCorrectlyFromString) {
  Decimal<6, 2> d("12.34");
  EXPECT_EQ(1234, static_cast<int64_t>(d));
  EXPECT_EQ(to_string(d), "12.34");
}

TEST(DecimalTest, ParsesNegativeDecimal) {
  Decimal<6, 3> d("-0.123");
  EXPECT_EQ(-123, static_cast<int64_t>(d));
  EXPECT_EQ(to_string(d), "-0.123");
}

TEST(DecimalTest, ZeroPaddingAfterDot) {
  Decimal<6, 3> d("7.1");
  EXPECT_EQ(7100, static_cast<int64_t>(d));
  EXPECT_EQ(to_string(d), "7.100");
}

TEST(DecimalTest, EqualityAndInequality) {
  Decimal<6, 2> a("1.50");
  Decimal<6, 2> b("1.50");
  Decimal<6, 2> c("1.51");

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);
}

TEST(DecimalTest, RejectsTooMuchPrecision) {
  Decimal<6, 2> d("1.234");
  EXPECT_TRUE(d.is_nan());
  EXPECT_EQ(to_string(d), "NaN");
}

TEST(DecimalTest, HandlesZeroProperly) {
  Decimal<6, 4> d(0);
  EXPECT_EQ(0, static_cast<int64_t>(d));
  EXPECT_EQ(to_string(d), "0.0000");
}

/*TEST(DecimalMixedScaleTest, CompareEqualAcrossScales) {*/
/*  Decimal<6, 2> a("1.20");*/
/*  Decimal<6, 3> b("1.200");*/
/**/
/*  EXPECT_TRUE((decimal_cast<6, 3>(a)) == b);*/
/*  EXPECT_FALSE((decimal_cast<6, 3>(a)) != b);*/
/*}*/
/**/
/*TEST(DecimalMixedScaleTest, CompareLessGreaterAcrossScales) {*/
/*  Decimal<6, 2> a("1.19");*/
/*  Decimal<6, 3> b("1.190");*/
/**/
/*  EXPECT_TRUE((decimal_cast<6, 3>(a)) == b);*/
/**/
/*  Decimal<6, 2> c("1.18");*/
/*  EXPECT_TRUE((decimal_cast<6, 3>(c)) < b);*/
/*  EXPECT_TRUE(b > (decimal_cast<6, 3>(c)));*/
/*}*/
