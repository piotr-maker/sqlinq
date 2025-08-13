#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_backend.hpp"
#include "sqlinq/database.hpp"

using ::testing::_;
using ::testing::SaveArg;

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
  EXPECT_EQ(bind->type, BindData::Text);
  EXPECT_EQ(bind->buffer, nullptr);
  EXPECT_NE(bind->size, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);

  bind = &captured[1];
  EXPECT_EQ(bind->type, BindData::Blob);
  EXPECT_EQ(bind->buffer, nullptr);
  EXPECT_NE(bind->size, nullptr);
  EXPECT_NE(bind->is_null, nullptr);
  EXPECT_NE(bind->error, nullptr);
}
