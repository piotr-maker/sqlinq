#ifndef TESTS_BACKEND_TEST_MODEL_HPP_
#define TESTS_BACKEND_TEST_MODEL_HPP_

#include <chrono>
#include <gtest/gtest.h>
#include <optional>
#include <sqlinq/backend/backend_iface.hpp>
#include <sqlinq/types.h>
#include <string>

using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;

#define ASSERT_FIELD_EQUAL(field)                                              \
  if (a.field != b.field) {                                                    \
    return AssertionFailure() << #field << " mismatch: expected " << a.field   \
                              << ", got " << b.field;                          \
  }

#define ASSERT_FIELD_EQUAL2(field)                                             \
  if (a.field != b.field) {                                                    \
    return AssertionFailure()                                                  \
           << #field << " mismatch: expected " << sqlinq::to_string(a.field)   \
           << ", got " << sqlinq::to_string(b.field);                          \
  }

struct TestModel {
  std::optional<int64_t> id;
  int8_t tiny_int_v;
  int16_t small_int_v;
  int32_t int_v;
  int64_t big_int_v;

  float float_v;
  double double_v;
  sqlinq::Decimal<8, 2> decimal_v;

  sqlinq::Blob blob_v;
  std::optional<std::string> text_v;

  bool bool_v;
  sqlinq::Date date_v;
  sqlinq::Time time_v;
  sqlinq::Datetime datetime_v;
  sqlinq::Timestamp timestamp_v;

  static constexpr std::size_t column_count = 15;

  static AssertionResult AreEqual(const TestModel &a, const TestModel &b) {
    if (a.id != b.id) {
      std::string text_a =
          (a.id.has_value() ? std::to_string(a.id.value()) : "NULL");
      std::string text_b =
          (b.id.has_value() ? std::to_string(b.id.value()) : "NULL");
      return AssertionFailure()
             << "id_v mismatch: expected " << text_a << ", got " << text_b;
    }
    ASSERT_FIELD_EQUAL(bool_v);
    ASSERT_FIELD_EQUAL(tiny_int_v);
    ASSERT_FIELD_EQUAL(small_int_v);
    ASSERT_FIELD_EQUAL(int_v);
    ASSERT_FIELD_EQUAL(big_int_v);
    ASSERT_FIELD_EQUAL(float_v);
    ASSERT_FIELD_EQUAL(double_v);
    ASSERT_FIELD_EQUAL2(decimal_v);
    ASSERT_FIELD_EQUAL2(blob_v);
    if (a.text_v != b.text_v) {
      std::string text_a = (a.text_v.has_value() ? a.text_v.value() : "NULL");
      std::string text_b = (b.text_v.has_value() ? b.text_v.value() : "NULL");
      return AssertionFailure()
             << "text_v mismatch: expected " << text_a << ", got " << text_b;
    }
    ASSERT_FIELD_EQUAL2(date_v);
    if (a.time_v.to_duration().count() != b.time_v.to_duration().count()) {
      return AssertionFailure()
             << "time_v mismatch: expected " << a.time_v.to_duration().count()
             << ", got " << b.time_v.to_duration().count();
    }
    ASSERT_FIELD_EQUAL2(datetime_v);
    ASSERT_FIELD_EQUAL(timestamp_v);
    return AssertionSuccess();
  }
};

inline std::vector<TestModel> db_rows = {
    {.id = 1,
     .tiny_int_v = 127,
     .small_int_v = 32767,
     .int_v = 123456,
     .big_int_v = 9223372036854775807,
     .float_v = 3.14f,
     .double_v = 2.71828,
     .decimal_v{"2400.59"},
     .blob_v = {std::byte{0xCA}, std::byte{0xFE}, std::byte{0xBA},
                std::byte{0xBE}},
     .text_v = std::nullopt,
     .bool_v = true,
     .date_v = sqlinq::Date{std::chrono::year{2023}, std::chrono::month{8},
                            std::chrono::day{8}},
     .time_v = sqlinq::Time{std::chrono::hours{14} + std::chrono::minutes{30} +
                            std::chrono::seconds{0}},
     .datetime_v = sqlinq::Datetime{std::chrono::seconds{1691505000}},
     .timestamp_v = sqlinq::Timestamp{1691505000}},
    {.id = 2,
     .tiny_int_v = -128,
     .small_int_v = -32768,
     .int_v = -123456,
     .big_int_v = static_cast<long long>(-9223372036854775807LL - 1),
     .float_v = -3.14f,
     .double_v = -2.71828,
     .decimal_v{"1800.50"},
     .blob_v = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE},
                std::byte{0xEF}},
     .text_v = "Text field",
     .bool_v = false,
     .date_v = sqlinq::Date{std::chrono::year{1999}, std::chrono::month{12},
                            std::chrono::day{31}},
     .time_v = sqlinq::Time{std::chrono::hours{23} + std::chrono::minutes{59} +
                            std::chrono::seconds{59}},
     .datetime_v = sqlinq::Datetime{std::chrono::seconds{946684799}},
     .timestamp_v = sqlinq::Timestamp{946684799}}};

namespace helper {
template <typename T>
sqlinq::BoundValue bind_value(const T& val) {
  using namespace sqlinq;
  return BoundValue{&val, 0, details::column_type_of<T>()};
}

template <typename T>
sqlinq::BoundValue bind_value(std::optional<T>& val) {
  using namespace sqlinq;
  if (val.has_value()) {
    return bind_value(val.value());
  }
  return BoundValue{};
}

template <std::size_t P, std::size_t S>
inline sqlinq::BoundValue bind_value(const sqlinq::Decimal<P, S>& d) {
  using namespace sqlinq;
  return BoundValue{d};
}

template <>
inline sqlinq::BoundValue bind_value<sqlinq::Blob>(const sqlinq::Blob& b) {
  using namespace sqlinq;
  return BoundValue{b.data(), b.size(), details::column_type_of<Blob>()};
}

template <>
inline sqlinq::BoundValue bind_value<std::string>(const std::string& s) {
  using namespace sqlinq;
  return BoundValue{s.data(), s.size(), details::column_type_of<std::string>()};
}
}

class TestBindData {
public:
  static constexpr std::size_t size = TestModel::column_count;

  TestBindData() {
    memset(length_, 0, sizeof(std::size_t) * size);
    memset(is_null_, 0, sizeof(bool) * size);
    memset(error_, 0, sizeof(bool) * size);
  }

  std::vector<sqlinq::BoundValue> bind_param(TestModel &model) {
    using namespace sqlinq;
    std::vector<BoundValue> result;
    result.emplace_back(helper::bind_value(model.id));
    result.emplace_back(helper::bind_value(model.tiny_int_v));
    result.emplace_back(helper::bind_value(model.small_int_v));
    result.emplace_back(helper::bind_value(model.int_v));
    result.emplace_back(helper::bind_value(model.big_int_v));
    result.emplace_back(helper::bind_value(model.float_v));
    result.emplace_back(helper::bind_value(model.double_v));
    result.emplace_back(helper::bind_value(model.decimal_v));
    result.emplace_back(helper::bind_value(model.blob_v));
    result.emplace_back(helper::bind_value(model.text_v));
    result.emplace_back(helper::bind_value(model.bool_v));
    result.emplace_back(helper::bind_value(model.date_v));
    result.emplace_back(helper::bind_value(model.time_v));
    result.emplace_back(helper::bind_value(model.datetime_v));
    result.emplace_back(helper::bind_value(model.timestamp_v));
    return result;
  }

  void bind_result(TestModel &model) {
    model.id.emplace();
    bind_[0].type = sqlinq::column::Type::BigInt;
    bind_[0].buffer = &model.id.value();
    bind_[0].buffer_length = 0;
    bind_[0].length = &length_[0];
    bind_[0].is_null = &is_null_[0];
    bind_[0].error = &error_[0];

    bind_[1].type = sqlinq::column::Type::TinyInt;
    bind_[1].buffer = &model.tiny_int_v;
    bind_[1].buffer_length = 0;
    bind_[1].length = &length_[1];
    bind_[1].is_null = &is_null_[1];
    bind_[1].error = &error_[1];

    bind_[2].type = sqlinq::column::Type::SmallInt;
    bind_[2].buffer = &model.small_int_v;
    bind_[2].buffer_length = 0;
    bind_[2].length = &length_[2];
    bind_[2].is_null = &is_null_[2];
    bind_[2].error = &error_[2];

    bind_[3].type = sqlinq::column::Type::Int;
    bind_[3].buffer = &model.int_v;
    bind_[3].buffer_length = 0;
    bind_[3].length = &length_[3];
    bind_[3].is_null = &is_null_[3];
    bind_[3].error = &error_[3];

    bind_[4].type = sqlinq::column::Type::BigInt;
    bind_[4].buffer = &model.big_int_v;
    bind_[4].buffer_length = 0;
    bind_[4].length = &length_[4];
    bind_[4].is_null = &is_null_[4];
    bind_[4].error = &error_[4];

    bind_[5].type = sqlinq::column::Type::Float;
    bind_[5].buffer = &model.float_v;
    bind_[5].buffer_length = 0;
    bind_[5].length = &length_[5];
    bind_[5].is_null = &is_null_[5];
    bind_[5].error = &error_[5];

    bind_[6].type = sqlinq::column::Type::Double;
    bind_[6].buffer = &model.double_v;
    bind_[6].buffer_length = 0;
    bind_[6].length = &length_[6];
    bind_[6].is_null = &is_null_[6];
    bind_[6].error = &error_[6];

    bind_[7].type = sqlinq::column::Type::Decimal;
    bind_[7].buffer = &model.decimal_v;
    bind_[7].buffer_length = 0;
    bind_[7].length = &length_[7];
    bind_[7].is_null = &is_null_[7];
    bind_[7].error = &error_[7];

    bind_[8].type = sqlinq::column::Type::Blob;
    bind_[8].buffer = nullptr;
    bind_[8].buffer_length = 0;
    bind_[8].length = &length_[8];
    bind_[8].is_null = &is_null_[8];
    bind_[8].error = &error_[8];

    bind_[9].type = sqlinq::column::Type::Text;
    bind_[9].buffer = nullptr;
    bind_[9].buffer_length = 0;
    bind_[9].length = &length_[9];
    bind_[9].is_null = &is_null_[9];
    bind_[9].error = &error_[9];

    bind_[10].type = sqlinq::column::Type::Bit;
    bind_[10].buffer = &model.bool_v;
    bind_[10].buffer_length = 0;
    bind_[10].length = &length_[10];
    bind_[10].is_null = &is_null_[10];
    bind_[10].error = &error_[10];

    bind_[11].type = sqlinq::column::Type::Date;
    bind_[11].buffer = &model.date_v;
    bind_[11].buffer_length = 0;
    bind_[11].length = &length_[11];
    bind_[11].is_null = &is_null_[11];
    bind_[11].error = &error_[11];

    bind_[12].type = sqlinq::column::Type::Time;
    bind_[12].buffer = &model.time_v;
    bind_[12].buffer_length = 0;
    bind_[12].length = &length_[12];
    bind_[12].is_null = &is_null_[12];
    bind_[12].error = &error_[12];

    bind_[13].type = sqlinq::column::Type::Datetime;
    bind_[13].buffer = &model.datetime_v;
    bind_[13].buffer_length = 0;
    bind_[13].length = &length_[13];
    bind_[13].is_null = &is_null_[13];
    bind_[13].error = &error_[13];

    bind_[14].type = sqlinq::column::Type::Timestamp;
    bind_[14].buffer = &model.timestamp_v;
    bind_[14].buffer_length = 0;
    bind_[14].length = &length_[14];
    bind_[14].is_null = &is_null_[14];
    bind_[14].error = &error_[14];
  }

  sqlinq::BindData *get() noexcept { return bind_; }

private:
  sqlinq::BindData bind_[size];
  std::size_t length_[size];
  bool is_null_[size];
  bool error_[size];
};

#endif // TESTS_BACKEND_TEST_MODEL_HPP_
