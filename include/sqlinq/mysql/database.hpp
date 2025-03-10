#ifndef SQLINQ_MYSQL_DATABASE_HPP_
#define SQLINQ_MYSQL_DATABASE_HPP_

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <mysql/mysql.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "sqlinq/types.h"
#include "sqlinq/type_traits.hpp"

namespace sqlinq::mysql {

class Database;

template <std::size_t N> class Statement {
public:
  Statement(Database &db);

  ~Statement() {
    if (stmt_ != nullptr) {
      mysql_stmt_close(stmt_);
      stmt_ = nullptr;
    }
  }

  Statement(Statement &&) = default;
  Statement(const Statement &) = delete;

  template <class T> void bind(int index, T &value) {
    static_assert(true, "The specified type is not supported by this method");
  }

  template <string_like T> inline void bind(int index, T &&text) noexcept {
    std::string_view sv = std::forward<T>(text);
    std::size_t size = sv.size();
    bind_[index].buffer_type = MYSQL_TYPE_STRING;
    bind_[index].buffer = (char *)sv.data();
    bind_[index].is_null = 0;
    bind_[index].length = &size;
  }

  template <class T>
  inline void bind(int index, std::optional<T> value) noexcept {
    if (value.has_value()) {
      bind(index, value.value());
      return;
    }
    bind_[index].buffer_type = MYSQL_TYPE_NULL;
    bind_[index].buffer = nullptr;
    bind_[index].is_null = 1;
    bind_[index].length = 0;
  }

  template <class T> inline void bind(int index, T &&value) noexcept {
    enum_field_types type;
    if constexpr (std::is_same_v<T, char>) {
      type = MYSQL_TYPE_TINY;
    } else if (std::is_same_v<T, short>) {
      type = MYSQL_TYPE_SHORT;
    } else if (std::is_same_v<T, long>) {
      type = MYSQL_TYPE_LONG;
    } else if (std::is_same_v<T, long long>) {
      type = MYSQL_TYPE_LONGLONG;
    } else if (std::is_same_v<T, float>) {
      type = MYSQL_TYPE_FLOAT;
    } else if (std::is_same_v<T, double>) {
      type = MYSQL_TYPE_DOUBLE;
    } else if (std::is_same_v<T, bool>) {
      type = MYSQL_TYPE_BIT;
    } else if (std::is_same_v<T, std::time_t>) {
      type = MYSQL_TYPE_TIMESTAMP;
    }
    bind_[index].buffer_type = type;
    bind_[index].buffer = (char *)&value;
    bind_[index].is_null = 0;
    bind_[index].length = 0;
  }

  inline void execute() {
    if (mysql_stmt_execute(stmt_)) {
      throw std::runtime_error(mysql_stmt_error(stmt_));
    }
  }

  void prepare(std::string_view sql) {
    if (mysql_stmt_prepare(stmt_, sql.data(), sql.size())) {
      throw std::runtime_error(mysql_stmt_error(stmt_));
    }
    memset(bind_, 0, sizeof(bind_));
  }

  inline void step() {
    if (mysql_stmt_bind_param(stmt_, bind_)) {
      throw std::runtime_error(mysql_stmt_error(stmt_));
    }
  }

  Statement &operator=(Statement &&) = default;
  Statement &operator=(const Statement &other) = delete;

private:
  template <std::size_t, std::size_t> friend class Result;

  MYSQL_STMT *stmt_;
  MYSQL_BIND bind_[N];
};

template <std::size_t K, std::size_t N> class Result {
public:
  Result(Statement<N> &stmt) : stmt_(stmt) {
    result_ = mysql_stmt_result_metadata(stmt_.stmt_);
    if (!result_) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  ~Result() { mysql_stmt_free_result(stmt_.stmt_); }

  Result(Result &&) = default;
  Result(const Result &) = delete;

  template <class T> void column(int index, T &value) {
    static_assert(true, "The specified type is not supported by this method");
  }

  template <class T> void column(int index, std::optional<T> &var) {
    var = T{};
    column(index, var.value());
  }

  template <std::size_t M, std::size_t D>
  inline void column(int index, sqlinq::decimal<M, D> &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_DECIMAL;
    bind_[index].buffer = (void *)&value;
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }
  /*void column(int index, std::time_t &time) noexcept {*/
  /*  struct std::tm tm;*/
  /*  const char *datetime = (const char *)sqlite3_column_text(stmt_, index);*/
  /*  std::istringstream ss(datetime);*/
  /*  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");*/
  /*  time = mktime(&tm);*/
  /*}*/

  template <std::integral T> inline void column(int index, T &value) noexcept {
    if constexpr (sizeof(T) == 1) {
      bind_[index].buffer_type = MYSQL_TYPE_TINY;
    } else if (sizeof(T) == 2) {
      bind_[index].buffer_type = MYSQL_TYPE_SHORT;
    } else if (sizeof(T) == 4) {
      bind_[index].buffer_type = MYSQL_TYPE_LONG;
    } else if (sizeof(T) == 8) {
      bind_[index].buffer_type = MYSQL_TYPE_LONGLONG;
    }
    bind_[index].buffer = (char *)&value;
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }

  inline void column(int index, float &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_FLOAT;
    bind_[index].buffer = (char *)&value;
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }

  inline void column(int index, double &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_DOUBLE;
    bind_[index].buffer = (char *)&value;
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }

  inline void column(int index, Blob &data) {
    bind_[index].buffer_type = MYSQL_TYPE_BLOB;
    bind_[index].buffer = nullptr;
    bind_[index].buffer_length = 0;
    bind_[index].length = &length_[index];
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }

  inline void column(int index, std::string &text) {
    bind_[index].buffer_type = MYSQL_TYPE_STRING;
    bind_[index].buffer = nullptr;
    bind_[index].buffer_length = 0;
    bind_[index].length = &length_[index];
    bind_[index].error = &error_[index];
    bind_[index].is_null = &is_null_[index];
  }

  template <class T> void fetch(int index, T &value) {
    static_assert(true, "The specified type is not supported by this method");
  }

  template <typename T> inline void fetch(int index, std::optional<T> &var) {
    if (is_null_[index]) {
      var.reset();
      return;
    }
    fetch(index, var.value());
  }

  inline void fetch(int index, Blob &data) {
    data.resize(length_[index]);
    bind_[index].buffer = (char *)data.data();
    bind_[index].buffer_length = data.size();
    if (mysql_stmt_fetch_column(stmt_.stmt_, &bind_[index], index, 0)) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  inline void fetch(int index, std::string &text) {
    text.resize(length_[index]);
    bind_[index].buffer = (char *)text.data();
    bind_[index].buffer_length = text.size();
    if (mysql_stmt_fetch_column(stmt_.stmt_, &bind_[index], index, 0)) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  template <typename Tuple> void bind_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    assert(mysql_num_fields(result_) == tup_size &&
           "Invalid column count to bind");
    memset(bind_, 0, sizeof(bind_));
    bind_for_each_impl(tup, std::make_index_sequence<tup_size>{});
    if (mysql_stmt_bind_result(stmt_.stmt_, bind_)) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  template <typename Tuple> void fetch_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    fetch_for_each_impl(tup, std::make_index_sequence<tup_size>{});
  }

  inline int fetch() { return mysql_stmt_fetch(stmt_.stmt_); }

  Result &operator=(Result &&) = default;
  Result &operator=(const Result &other) = delete;

private:
  MYSQL_RES *result_;
  MYSQL_BIND bind_[K];
  Statement<N> &stmt_;
  my_bool error_[K];
  my_bool is_null_[K];
  std::size_t length_[K];

  template <typename Tuple, std::size_t... Idx>
  void bind_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (column(Idx, std::get<Idx>(tup)), ...);
  }

  template <typename Tuple, std::size_t... Idx>
  void fetch_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (fetch(Idx, std::get<Idx>(tup)), ...);
  }
};

class Database {
public:
  Database() {}
  virtual ~Database() { disconnect(); }

  Database(Database &&) = default;
  Database(const Database &) = delete;

  void exec(const char *sql) { exec(sql, std::make_tuple()); }

  template <class Tuple> void exec(const char *sql, Tuple &&tup) {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Template type have to be std::tuple");
    int rc;
    Statement<std::tuple_size_v<std::remove_reference_t<Tuple>>> stmt{*this};
    stmt.prepare(sql);
    std::apply(
        [&stmt](auto &&...args) {
          int i = 0;
          (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
        },
        std::forward<Tuple>(tup));

    stmt.step();
    stmt.execute();
  }

  template <class Entity>
  auto exec_res(const char *sql) -> std::vector<Entity> {
    auto tup = std::make_tuple();
    return exec_res<Entity>(sql, tup);
  }

  template <class Entity, class Tuple>
  auto exec_res(const char *sql, Tuple &&tup) -> std::vector<Entity> {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Template type have to be std::tuple");
    Entity entity;
    std::vector<Entity> entities;

    constexpr std::size_t bind_tuple_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    Statement<bind_tuple_size> stmt{*this};
    stmt.prepare(sql);
    std::apply(
        [&stmt](auto &&...args) {
          int i = 0;
          (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
        },
        std::forward<Tuple>(tup));

    stmt.step();
    stmt.execute();
    auto params = structure_to_tuple(entity);
    Result<std::tuple_size_v<decltype(params)>, bind_tuple_size> res{stmt};
    res.bind_for_each(params);
    while (1) {
      int status = res.fetch();
      if (status == 1 || status == MYSQL_NO_DATA)
        break;
      res.fetch_for_each(params);
      entity = to_struct<Entity>(params);
      entities.push_back(entity);
    }
    return entities;
  }

  template <class Tuple>
    requires is_tuple_v<Tuple>
  auto exec_res(const char *sql) -> std::vector<Tuple> {
    Tuple tup;
    std::vector<Tuple> records;

    Statement<0> stmt{*this};
    stmt.prepare(sql);
    stmt.execute();
    Result<std::tuple_size_v<Tuple>, 0> res{stmt};
    res.bind_for_each(tup);
    while (1) {
      int status = res.fetch();
      if (status == 1 || status == MYSQL_NO_DATA)
        break;
      res.fetch_for_each(tup);
      records.push_back(tup);
    }
    return records;
  }

  inline void connect(const char *host, const char *user, const char *passwd,
                      const char *db) {
    db_ = mysql_init(NULL);
    if (db_ == NULL) {
      throw std::bad_alloc();
    }
    db_ = mysql_real_connect(db_, host, user, passwd, db, 0, NULL, 0);
    if (db_ == NULL) {
      disconnect();
      throw std::runtime_error(mysql_error(db_));
    }
  }

  inline void disconnect() noexcept {
    if (db_ != nullptr) {
      mysql_close(db_);
      db_ = nullptr;
    }
  }

  inline bool is_connected() const noexcept { return db_ != nullptr; }

  inline int64_t last_inserted_rowid() noexcept { return mysql_insert_id(db_); }

  Database &operator=(Database &&) = default;
  Database &operator=(const Database &other) = delete;

private:
  template <std::size_t N> friend class Statement;

  MYSQL *db_;
};

template <std::size_t N>
Statement<N>::Statement(Database &db) : stmt_(nullptr) {
  if (db.db_) {
    stmt_ = mysql_stmt_init(db.db_);
  }

  if (stmt_ == nullptr) {
    throw std::bad_alloc();
  }
}

} // namespace sqlinq::mysql

#endif /* SQLINQ_MYSQL_DATABASE_HPP_ */
