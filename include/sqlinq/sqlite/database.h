#ifndef SQLINQ_SQLITE_DATABASE_H_
#define SQLINQ_SQLITE_DATABASE_H_

#include <cassert>
#include <concepts>
#include <ctime>
#include <expected>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "error.h"
#include "sqlinq/type_traits.hpp"
#include "sqlinq/types/decimal.hpp"
#include <sqlite3.h>

namespace sqlinq::sqlite {

class database;

class statement {
public:
  statement(database &db, std::string_view sql);

  ~statement() { sqlite3_finalize(stmt_); }

  template <class T> inline void bind(int index, std::optional<T> value) {
    if (value.has_value()) {
      bind(index, value.value());
      return;
    }
    int rc = sqlite3_bind_null(stmt_, index);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  inline void bind(int index, std::time_t time) {
    int rc = sqlite3_bind_int64(stmt_, index, time);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  template <std::size_t M, std::size_t D>
  inline void bind(int index, sqlinq::decimal<M, D> value) {
    static_assert(true, "Need to implement this");
    int rc = sqlite3_bind_double(stmt_, index, value);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  template <std::integral T> inline void bind(int index, T value) {
    int rc = sqlite3_bind_int64(stmt_, index, value);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  template <std::floating_point T> inline void bind(int index, T value) {
    int rc = sqlite3_bind_double(stmt_, index, value);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  template <string_like T> inline void bind(int index, T &&text) {
    int rc;
    std::string_view sv = std::forward<T>(text);
    rc = sqlite3_bind_text(stmt_, index, sv.data(), sv.size(), nullptr);
    if (rc != SQLITE_OK) {
      throw rc;
    }
  }

  template <class T> void column(int index, std::optional<T> &value) {
    if (sqlite3_column_type(stmt_, index) == SQLITE_NULL) {
      value.reset();
    } else {
      T var;
      column(index, var);
      value = var;
    }
  }

  void column(int index, std::time_t &time) noexcept {
    struct std::tm tm;
    const char *datetime = (const char *)sqlite3_column_text(stmt_, index);
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    time = mktime(&tm);
  }

  template <std::size_t M, std::size_t D>
  inline void column(int index, sqlinq::decimal<M, D> &value) noexcept {
    assert(sqlite3_column_type(stmt_, index) == SQLITE_FLOAT &&
           "Incorrect column type");
    double var = sqlite3_column_double(stmt_, index);
    value = decimal<M, D>{var};
  }

  template <std::integral T> inline void column(int index, T &value) noexcept {
    assert(sqlite3_column_type(stmt_, index) == SQLITE_INTEGER &&
           "Incorrect column type");
    value = sqlite3_column_int(stmt_, index);
  }

  template <std::floating_point T>
  inline void column(int index, T &value) noexcept {
    assert(sqlite3_column_type(stmt_, index) == SQLITE_FLOAT &&
           "Incorrect column type");
    value = sqlite3_column_double(stmt_, index);
  }

  inline void column(int index, std::string &text) {
    assert(sqlite3_column_type(stmt_, index) == SQLITE_TEXT &&
           "Incorrect column type");
    text = std::string{(const char *)sqlite3_column_text(stmt_, index)};
  }

  template <typename Tuple> void column_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    column_for_each_impl(tup, std::make_index_sequence<tup_size>{});
  }

  inline int step() { return sqlite3_step(stmt_); }

private:
  sqlite3_stmt *stmt_;

  template <typename Tuple, std::size_t... Idx>
  void column_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (column(Idx, std::get<Idx>(tup)), ...);
  }
};

class database {
public:
  friend class statement;

  database() {}
  virtual ~database() { disconnect(); }

  database(database &&) = default;
  database(const database &) = delete;

  auto exec(const char *sql) -> std::error_code {
    return exec(sql, std::make_tuple());
  }

  template <class Tuple>
  auto exec(const char *sql, Tuple &&tup) -> std::error_code {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Template type have to be std::tuple");
    int rc = SQLITE_OK;
    statement stmt{*this, sql};
    try {
      std::apply(
          [&stmt](auto &&...args) {
            int i = 1;
            (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
          },
          std::forward<Tuple>(tup));
    } catch (int e) {
      return sqlite::error::make_error_code(e);
    }
    rc = stmt.step();
    if (rc != SQLITE_DONE)
      return sqlite::error::make_error_code(rc);
    return std::error_code{};
  }

  template <class Entity>
  auto exec_res(const char *sql)
      -> std::expected<std::vector<Entity>, std::error_code> {
    auto tup = std::make_tuple();
    return exec_res<Entity>(sql, tup);
  }

  template <class Entity, class Tuple>
  auto exec_res(const char *sql, Tuple &&tup)
      -> std::expected<std::vector<Entity>, std::error_code> {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Template type have to be std::tuple");
    int rc;
    Entity entity;
    std::vector<Entity> entities;

    statement stmt{*this, sql};
    try {
      std::apply(
          [&stmt](auto &&...args) {
            int i = 1;
            (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
          },
          std::forward<Tuple>(tup));
    } catch (int e) {
      return std::unexpected(sqlite::error::make_error_code(e));
    }

    auto params = structure_to_tuple(entity);
    while ((rc = stmt.step()) == SQLITE_ROW) {
      stmt.column_for_each(params);
      entity = to_struct<Entity>(params);
      entities.push_back(entity);
    }
    if (rc != SQLITE_DONE)
      return std::unexpected(sqlite::error::make_error_code(rc));
    return entities;
  }

  template <class Tuple>
    requires is_tuple_v<Tuple>
  auto exec_res(const char *sql)
      -> std::expected<std::vector<Tuple>, std::error_code> {
    int rc;
    Tuple tup;
    std::vector<Tuple> records;

    statement stmt{*this, sql};
    while ((rc = stmt.step()) == SQLITE_ROW) {
      stmt.column_for_each(tup);
      records.push_back(tup);
    }
    if (rc != SQLITE_DONE)
      return std::unexpected(sqlite::error::make_error_code(rc));
    return records;
  }

  int backup(database &db);
  int restore(database &db);

  inline int connect(const char *fname) noexcept {
    int rc = sqlite3_open(fname, &db_);
    sqlite3_extended_result_codes(db_, 1);
    return rc;
  }

  inline void disconnect() noexcept {
    if (db_ != nullptr) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
  }

  inline int64_t last_inserted_rowid() noexcept {
    return sqlite3_last_insert_rowid(db_);
  }

  database &operator=(database &&) = default;
  database &operator=(const database &other) = delete;

private:
  sqlite3 *db_;
};
} // namespace sqlinq::sqlite

#endif /* SQLINQ_SQLITE_DATABASE_H_ */
