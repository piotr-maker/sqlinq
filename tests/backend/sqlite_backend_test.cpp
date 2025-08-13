#include <gtest/gtest.h>
#include <sqlite_backend.hpp>

#include <cstring>

#include "test_model.hpp"

using namespace sqlinq;

class SQLiteBackendTest : public ::testing::Test {
protected:
  SQLiteBackend backend_;
  const char *create_query_ = R"(CREATE TABLE test (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  tiny_int_v INTEGER NOT NULL,
  small_int_v INTEGER NOT NULL,
  int_v INTEGER NOT NULL,
  big_int_v INTEGER NOT NULL,

  float_v DOUBLE NOT NULL,
  double_v DOUBLE NOT NULL,
  decimal_v INTEGER NOT NULL,

  blob_v BLOB NOT NULL,
  text_v TEXT,

  bool_v INTEGER NOT NULL,
  date_v TEXT NOT NULL,
  time_v TEXT NOT NULL,
  datetime_v TEXT NOT NULL,
  timestamp_v INTEGER NOT NULL
);)";

  const char *insert_query_ = R"(INSERT INTO test
  (tiny_int_v, small_int_v, int_v, big_int_v,
   float_v, double_v, decimal_v,
   blob_v, text_v,
   bool_v, date_v, time_v, datetime_v, timestamp_v)
  VALUES
  (
    127, 32767, 123456, 9223372036854775807,
    3.14, 2.71828, 240059,
    X'CAFEBABE', NULL,
    TRUE, '2023-08-08', '14:30:00', '2023-08-08 14:30:00', 1691505000
  ),
  (
    -128, -32768, -123456, -9223372036854775808,
    -3.14, -2.71828, 180050,
    X'DEADBEEF', 'Text field',
    FALSE, '1999-12-31', '23:59:59', '1999-12-31 23:59:59', 946684799
  )
;)";

  void SetUp() override {
    backend_.connect(":memory:");
    /*backend_.connect("test.sqlite3");*/
    ASSERT_TRUE(backend_.is_connected());

    backend_.stmt_init();
    backend_.stmt_prepare(create_query_);
    ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);
    backend_.stmt_close();

    backend_.stmt_init();
    backend_.stmt_prepare(insert_query_);
    ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);
    backend_.stmt_close();
  }

  void TearDown() override { backend_.disconnect(); }
};

TEST_F(SQLiteBackendTest, SelectRows) {
  TestModel model;
  TestBindData bind;
  std::vector<TestModel> records;

  backend_.stmt_init();
  backend_.stmt_prepare("SELECT * FROM test ORDER BY id");
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);

  bind.bind_result(model);
  model.blob_v.resize(12);
  bind.get()[8].buffer = model.blob_v.data();
  bind.get()[8].buffer_length = model.blob_v.size();

  model.text_v = std::string(12, '\0');
  bind.get()[9].buffer = model.text_v->data();
  bind.get()[9].buffer_length = model.text_v->size();

  backend_.bind_result(bind.get(), TestBindData::size);
  while (backend_.stmt_fetch() == ExecStatus::Row) {
    records.push_back(model);
    records.back().blob_v.resize(*bind.get()[8].length);
    if (*bind.get()[9].is_null) {
      records.back().text_v.reset();
    } else {
      records.back().text_v->resize(*bind.get()[9].length);
    }
  }
  backend_.stmt_close();

  ASSERT_EQ(records.size(), 2);
  EXPECT_TRUE(TestModel::AreEqual(db_rows[0], records[0]));
  EXPECT_TRUE(TestModel::AreEqual(db_rows[1], records[1]));
}

TEST_F(SQLiteBackendTest, SelectRowsWithBindedParams) {
  using Result = std::tuple<int8_t, Timestamp>;
  bool is_null[2];
  BindData bind[2];
  std::size_t length[2];

  short greater_than = 0;
  Decimal<8, 2> d{"2000.00"};
  Result result;
  std::vector<Result> records;

  memset(&bind, 0, sizeof(bind));
  memset(&length, 0, sizeof(length));
  memset(&is_null, 0, sizeof(is_null));

  bind[0].type = BindData::Decimal;
  bind[0].buffer = &d;
  bind[0].length = &length[0];
  bind[0].is_null = &is_null[0];

  bind[1].type = BindData::Short;
  bind[1].buffer = &greater_than;
  bind[1].length = &length[1];
  bind[1].is_null = &is_null[1];

  backend_.stmt_init();
  backend_.stmt_prepare("SELECT tiny_int_v, timestamp_v FROM test WHERE "
                        "decimal_v > ? AND small_int_v > ?");
  backend_.bind_param(bind, 2);
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);

  is_null[0] = false;
  bind[0].type = BindData::Tiny;
  bind[0].buffer = &std::get<0>(result);
  bind[1].type = BindData::Timestamp;
  bind[1].buffer = &std::get<1>(result);
  backend_.bind_result(bind, 2);
  while (backend_.stmt_fetch() == ExecStatus::Row) {
    records.push_back(result);
  }
  backend_.stmt_close();

  ASSERT_EQ(records.size(), 1);
  EXPECT_EQ(127, std::get<0>(records[0]));
  EXPECT_EQ(Timestamp{1691505000}, std::get<1>(records[0]));
}

TEST_F(SQLiteBackendTest, SelectTruncatedRows) {
  TestModel model;
  TestBindData bind;
  std::vector<TestModel> records;

  backend_.stmt_init();
  backend_.stmt_prepare("SELECT * FROM test ORDER BY id");
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);

  ExecStatus status;
  bind.bind_result(model);
  model.text_v = std::string{};
  backend_.bind_result(bind.get(), TestBindData::size);
  while (1) {
    status = backend_.stmt_fetch();
    if (status == ExecStatus::Truncated) {
      if (*bind.get()[8].is_null == false) {
        BindData bd = bind.get()[8];
        model.blob_v.resize(*bd.length);
        bd.buffer = model.blob_v.data();
        bd.buffer_length = model.blob_v.size();
        backend_.stmt_fetch_column(8, bd);
      }
      if (*bind.get()[9].is_null == false) {
        BindData bd = bind.get()[9];
        model.text_v = std::string{};
        model.text_v->resize(*bd.length);
        bd.buffer = model.text_v->data();
        bd.buffer_length = model.text_v->size();
        backend_.stmt_fetch_column(9, bd);
      }
    } else {
      break;
    }
    records.emplace_back(std::move(model));
    if (*bind.get()[9].is_null) {
      records.back().text_v.reset();
    }
  }
  backend_.stmt_close();

  ASSERT_EQ(records.size(), 2);
  EXPECT_TRUE(TestModel::AreEqual(db_rows[0], records[0]));
  EXPECT_TRUE(TestModel::AreEqual(db_rows[1], records[1]));
}

TEST_F(SQLiteBackendTest, InsertRows) {
  TestModel rows[] = {
      {.id = std::nullopt,
       .tiny_int_v = 12,
       .small_int_v = 32,
       .int_v = 100000,
       .big_int_v = 9775807,
       .float_v = 0.1f,
       .double_v = 1.23456,
       .decimal_v{"7999.99"},
       .blob_v = {std::byte{0xAA}, std::byte{0xEE}, std::byte{0xBB},
                  std::byte{0xCC}},
       .text_v = std::nullopt,
       .bool_v = true,
       .date_v = sqlinq::Date{std::chrono::year{2025}, std::chrono::month{8},
                              std::chrono::day{8}},
       .time_v = sqlinq::Time{std::chrono::hours{8} + std::chrono::minutes{53} +
                              std::chrono::seconds{29}},
       .datetime_v = sqlinq::Datetime{std::chrono::seconds{1754643209}},
       .timestamp_v = sqlinq::Timestamp{1754643209}},
      {.id = std::nullopt,
       .tiny_int_v = -12,
       .small_int_v = -32,
       .int_v = -100000,
       .big_int_v = -9775807,
       .float_v = -0.1f,
       .double_v = -1.23456,
       .decimal_v{"559.99"},
       .blob_v = {std::byte{0xCC}, std::byte{0xBB}, std::byte{0xEE},
                  std::byte{0xAA}},
       .text_v = "Not empty string",
       .bool_v = false,
       .date_v = sqlinq::Date{std::chrono::year{2025}, std::chrono::month{8},
                              std::chrono::day{8}},
       .time_v = sqlinq::Time{std::chrono::hours{8} + std::chrono::minutes{53} +
                              std::chrono::seconds{29}},
       .datetime_v = sqlinq::Datetime{std::chrono::seconds{1754643209}},
       .timestamp_v = sqlinq::Timestamp{1754643209}}};

  TestBindData bind;
  const char *insert_query =
      "INSERT INTO test VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

  backend_.stmt_init();
  backend_.stmt_prepare(insert_query);
  bind.bind_param(rows[0]);
  backend_.bind_param(bind.get(), TestModel::column_count);
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);

  bind.bind_param(rows[1]);
  backend_.bind_param(bind.get(), TestModel::column_count);
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);
  backend_.stmt_close();

  TestModel model;
  std::vector<TestModel> records;

  backend_.stmt_init();
  backend_.stmt_prepare("SELECT * FROM test ORDER BY id");
  ASSERT_EQ(backend_.stmt_execute(), ExecStatus::Ok);

  ExecStatus status;
  bind.bind_result(model);
  model.text_v = std::string{};
  backend_.bind_result(bind.get(), TestBindData::size);
  while (1) {
    status = backend_.stmt_fetch();
    if (status == ExecStatus::Truncated) {
      if (*bind.get()[8].is_null == false) {
        BindData bd = bind.get()[8];
        model.blob_v.resize(*bd.length);
        bd.buffer = model.blob_v.data();
        bd.buffer_length = model.blob_v.size();
        backend_.stmt_fetch_column(8, bd);
      }
      if (*bind.get()[9].is_null == false) {
        BindData bd = bind.get()[9];
        model.text_v = std::string{};
        model.text_v->resize(*bd.length);
        bd.buffer = model.text_v->data();
        bd.buffer_length = model.text_v->size();
        backend_.stmt_fetch_column(9, bd);
      }
    } else {
      break;
    }
    records.emplace_back(std::move(model));
    if (*bind.get()[9].is_null) {
      records.back().text_v.reset();
    }
  }
  backend_.stmt_close();

  ASSERT_EQ(records.size(), 4);
  rows[0].id = 3;
  rows[1].id = 4;
  EXPECT_TRUE(TestModel::AreEqual(rows[0], records[2]));
  EXPECT_TRUE(TestModel::AreEqual(rows[1], records[3]));
}
