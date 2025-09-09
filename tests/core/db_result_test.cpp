#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_backend.hpp"
#include "sqlinq/cursor.hpp"

using ::testing::_;
using ::testing::SaveArg;

TEST(DbResultTest, FetchEachTupleContainerElement) {
  MockBackend backend;
  BindData bind0;
  BindData bind1;
  std::tuple<std::string, Blob> tup;

  constexpr std::size_t N = std::tuple_size_v<decltype(tup)>;
  Result<N> result{backend};

  // Only for fill BindData
  EXPECT_CALL(backend, bind_result(_, N)).Times(1);
  result.bind_result(tup);

  EXPECT_CALL(backend, stmt_fetch_column(_, _))
      .WillOnce(SaveArg<1>(&bind0))
      .WillOnce(SaveArg<1>(&bind1));
  result.fetch_for_each(tup);

  EXPECT_EQ(bind0.type, column::Type::Text);
  EXPECT_EQ(bind0.buffer, (void *)std::get<0>(tup).data());
  EXPECT_EQ(bind0.buffer_length, 0);
  EXPECT_NE(bind0.length, nullptr);
  EXPECT_NE(bind0.is_null, nullptr);
  EXPECT_NE(bind0.error, nullptr);

  EXPECT_EQ(bind1.type, column::Type::Blob);
  EXPECT_EQ(bind1.buffer, (void *)std::get<1>(tup).data());
  EXPECT_EQ(bind1.buffer_length, 0);
  EXPECT_NE(bind1.length, nullptr);
  EXPECT_NE(bind1.is_null, nullptr);
  EXPECT_NE(bind1.error, nullptr);
}

TEST(DbResultTest, PrepareBindEachTupleContainerElement) {
  MockBackend backend;
  const BindData *bind;
  const BindData *captured = nullptr;
  std::tuple<std::string, Blob> tup;

  constexpr std::size_t N = std::tuple_size_v<decltype(tup)>;
  Result<N> result{backend};

  EXPECT_CALL(backend, bind_result(_, N)).WillOnce(SaveArg<0>(&captured));
  result.bind_result(tup);

  ASSERT_NE(captured, nullptr);
  bind = &captured[0];
  EXPECT_EQ(bind->type, column::Type::Text);
  EXPECT_EQ(bind->buffer, nullptr);
  EXPECT_NE(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[1];
  EXPECT_EQ(bind->type, column::Type::Blob);
  EXPECT_EQ(bind->buffer, nullptr);
  EXPECT_NE(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);
}

TEST(DbResultTest, PrepareBindEachTupleOptionalElement) {
  MockBackend backend;
  const BindData *bind;
  const BindData *captured = nullptr;
  std::tuple<std::optional<int>, std::optional<std::string>> tup;

  constexpr std::size_t N = std::tuple_size_v<decltype(tup)>;
  Result<N> result{backend};

  EXPECT_CALL(backend, bind_result(_, N)).WillOnce(SaveArg<0>(&captured));
  result.bind_result(tup);

  ASSERT_NE(captured, nullptr);
  bind = &captured[0];
  EXPECT_EQ(bind->type, column::Type::Int);
  EXPECT_EQ(bind->buffer, (void *)&std::get<0>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[1];
  EXPECT_EQ(bind->type, column::Type::Text);
  EXPECT_EQ(bind->buffer, nullptr);
  EXPECT_NE(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);
}

TEST(DbResultTest, PrepareBindEachTupleNumericElement) {
  MockBackend backend;
  const BindData *bind;
  const BindData *captured = nullptr;
  std::tuple<bool, char, short, int, long long, float, double> tup;

  constexpr std::size_t N = std::tuple_size_v<decltype(tup)>;
  Result<N> result{backend};

  EXPECT_CALL(backend, bind_result(_, N)).WillOnce(SaveArg<0>(&captured));
  result.bind_result(tup);

  ASSERT_NE(captured, nullptr);
  bind = &captured[0];
  EXPECT_EQ(bind->type, column::Type::Bit);
  EXPECT_EQ(bind->buffer, (void *)&std::get<0>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[1];
  EXPECT_EQ(bind->type, column::Type::TinyInt);
  EXPECT_EQ(bind->buffer, (void *)&std::get<1>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[2];
  EXPECT_EQ(bind->type, column::Type::SmallInt);
  EXPECT_EQ(bind->buffer, (void *)&std::get<2>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[3];
  EXPECT_EQ(bind->type, column::Type::Int);
  EXPECT_EQ(bind->buffer, (void *)&std::get<3>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[4];
  EXPECT_EQ(bind->type, column::Type::BigInt);
  EXPECT_EQ(bind->buffer, (void *)&std::get<4>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[5];
  EXPECT_EQ(bind->type, column::Type::Float);
  EXPECT_EQ(bind->buffer, (void *)&std::get<5>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[6];
  EXPECT_EQ(bind->type, column::Type::Double);
  EXPECT_EQ(bind->buffer, (void *)&std::get<6>(tup));
  EXPECT_EQ(bind->length, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);
}
