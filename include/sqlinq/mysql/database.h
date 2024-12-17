#ifndef SQLINQ_MYSQL_DATABASE_H_
#define SQLINQ_MYSQL_DATABASE_H_

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <expected>
#include <mysql/mysql.h>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "sqlinq/type_traits.hpp"
#include "sqlinq/types.h"

namespace sqlinq::mysql {

class database;

template <std::size_t N> class statement {
public:
  statement(database &db);

  ~statement() {
    if (stmt_ != nullptr) {
      mysql_stmt_close(stmt_);
      stmt_ = nullptr;
    }
  }

  statement(statement &&) = default;
  statement(const statement &) = delete;

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

  statement &operator=(statement &&) = default;
  statement &operator=(const statement &other) = delete;

private:
  template <std::size_t, std::size_t> friend class result;

  MYSQL_STMT *stmt_;
  MYSQL_BIND bind_[N];
};

template <std::size_t K, std::size_t N> class result {
public:
  result(statement<N> &stmt) : stmt_(stmt) {
    result_ = mysql_stmt_result_metadata(stmt_.stmt_);
    if (!result_) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  ~result() { mysql_stmt_free_result(stmt_.stmt_); }

  result(result &&) = default;
  result(const result &) = delete;

  template <class T> void column(int index, std::optional<T> &var) {
    var = T{};
    column(index, var.value());
  }

  template <std::size_t M, std::size_t D>
  inline void column(int index, sqlinq::decimal<M, D> &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_DECIMAL;
    bind_[index].buffer = (char *)&value;
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
  }

  inline void column(int index, float &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_FLOAT;
    bind_[index].buffer = (char *)&value;
  }

  inline void column(int index, double &value) noexcept {
    bind_[index].buffer_type = MYSQL_TYPE_DOUBLE;
    bind_[index].buffer = (char *)&value;
  }

  inline void column(int index, std::string &text) {
    text.resize(255);
    bind_[index].buffer_type = MYSQL_TYPE_STRING;
    bind_[index].buffer = (char *)text.data();
    bind_[index].buffer_length = text.size();
  }

  template <typename Tuple> void bind_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    assert(mysql_num_fields(result_) == tup_size && "Invalid column count to bind");
    memset(bind_, 0, sizeof(bind_));
    bind_for_each_impl(tup, std::make_index_sequence<tup_size>{});
    if (mysql_stmt_bind_result(stmt_.stmt_, bind_)) {
      throw std::runtime_error(mysql_stmt_error(stmt_.stmt_));
    }
  }

  inline int fetch() { return mysql_stmt_fetch(stmt_.stmt_); }

  result &operator=(result &&) = default;
  result &operator=(const result &other) = delete;

private:
  MYSQL_RES *result_;
  MYSQL_FIELD *fields_;
  MYSQL_BIND bind_[K];
  statement<N> &stmt_;

  template <typename Tuple, std::size_t... Idx>
  void bind_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (column(Idx, std::get<Idx>(tup)), ...);
  }
};

class database {
public:
  database() {}
  virtual ~database() { disconnect(); }

  database(database &&) = default;
  database(const database &) = delete;

  void exec(const char *sql) { exec(sql, std::make_tuple()); }

  template <class Tuple> void exec(const char *sql, Tuple &&tup) {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Template type have to be std::tuple");
    int rc;
    statement<std::tuple_size_v<std::remove_reference_t<Tuple>>> stmt{*this};
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
    statement<bind_tuple_size> stmt{*this};
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
    result<std::tuple_size_v<decltype(params)>, bind_tuple_size> res{stmt};
    res.bind_for_each(params);
    while(1) {
      int status = res.fetch();
      if (status == 1 || status == MYSQL_NO_DATA)
        break;
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

    statement<0> stmt{*this};
    stmt.prepare(sql);
    stmt.execute();
    result<std::tuple_size_v<Tuple>, 0> res{stmt};
    res.bind_for_each(tup);
    while(1) {
      int status = res.fetch();
      if (status == 1 || status == MYSQL_NO_DATA)
        break;
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

  database &operator=(database &&) = default;
  database &operator=(const database &other) = delete;

private:
  template <std::size_t N> friend class statement;

  MYSQL *db_;
};

template <std::size_t N>
statement<N>::statement(database &db) : stmt_(nullptr) {
  if (db.db_) {
    stmt_ = mysql_stmt_init(db.db_);
  }

  if (stmt_ == nullptr) {
    throw std::bad_alloc();
  }
}

} // namespace sqlinq::mysql

#endif /* SQLINQ_MYSQL_DATABASE_H_ */
