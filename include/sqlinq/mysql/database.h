#ifndef SQLINQ_MYSQL_DATABASE_H_
#define SQLINQ_MYSQL_DATABASE_H_

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
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
#include <mysql/mysql.h>

#include "error.h"
#include "sqlinq/type_traits.hpp"

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

  [[nodiscard]]
  inline int execute() {
    if (mysql_stmt_execute(stmt_)) {
      return mysql_stmt_errno(stmt_);
    }
    return 0;
  }

  [[nodiscard]]
  int prepare(std::string_view sql) noexcept {
    if (stmt_ == nullptr) {
      return -1;
    }
    if (mysql_stmt_prepare(stmt_, sql.data(), sql.size())) {
      return mysql_stmt_errno(stmt_);
    }
    memset(bind_, 0, sizeof(bind_));
    return 0;
  }

  [[nodiscard]]
  inline int step() noexcept {
    if (mysql_stmt_bind_param(stmt_, bind_)) {
      return mysql_stmt_errno(stmt_);
    }
    return 0;
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
  result(statement<N> &stmt) : stmt_(stmt) {}

  ~result() { mysql_stmt_free_result(stmt_.stmt_); }

  result(result &&) = default;
  result(const result &) = delete;

  template <class T> void column(int index, std::optional<T> &var) {
    var = T{};
    column(index, var.value());
  }

  /*void column(int index, std::time_t &time) noexcept {*/
  /*  struct std::tm tm;*/
  /*  const char *datetime = (const char *)sqlite3_column_text(stmt_, index);*/
  /*  std::istringstream ss(datetime);*/
  /*  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");*/
  /*  time = mktime(&tm);*/
  /*}*/

  template <std::integral T>
  inline void column(int index, T &value) noexcept {
    if constexpr (sizeof(T) == 1) {
      bind_[index].buffer_type = MYSQL_TYPE_TINY;
    } else if(sizeof(T) == 2) {
      bind_[index].buffer_type = MYSQL_TYPE_SHORT;
    } else if(sizeof(T) == 4) {
      bind_[index].buffer_type = MYSQL_TYPE_LONG;
    } else if(sizeof(T) == 8) {
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

  template <typename Tuple> void column_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    memset(bind_, 0, sizeof(bind_));
    column_for_each_impl(tup, std::make_index_sequence<tup_size>{});
  }

  inline int bind() {
    if (mysql_stmt_bind_result(stmt_.stmt_, bind_)) {
      return mysql_stmt_errno(stmt_.stmt_);
    }
    return 0;
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
  void column_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (column(Idx, std::get<Idx>(tup)), ...);
  }
};

class database {
public:
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
    int rc;
    statement<std::tuple_size_v<std::remove_reference_t<Tuple>>> stmt{*this};
    rc = stmt.prepare(sql);
    if (rc != 0) {
      return mysql::error::make_error_code(rc);
    }
    std::apply(
        [&stmt](auto &&...args) {
          int i = 0;
          (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
        },
        std::forward<Tuple>(tup));

    rc = stmt.step();
    if (rc != 0)
      return mysql::error::make_error_code(rc);
    rc = stmt.execute();
    if (rc != 0)
      return mysql::error::make_error_code(rc);
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

    constexpr std::size_t bind_tuple_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    statement<bind_tuple_size> stmt{*this};
    rc = stmt.prepare(sql);
    if (rc != 0) {
      return std::unexpected(mysql::error::make_error_code(rc));
    }
    /*std::apply(*/
    /*    [&stmt](auto &&...args) {*/
    /*      int i = 0;*/
    /*      (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);*/
    /*    },*/
    /*    std::forward<Tuple>(tup));*/
    /**/
    /*auto params = structure_to_tuple(entity);*/
    /*result<std::tuple_size_v<decltype(params)>, bind_tuple_size> res{stmt};*/
    /*res.column_for_each(params);*/
    /*rc = res.step();*/
    /*if (rc != 0) {*/
    /*  return std::unexpected(mysql::error::make_error_code(rc));*/
    /*}*/
    /*while (res.fetch() != 0) {*/
    /*  entity = to_struct<Entity>(params);*/
    /*  entities.push_back(entity);*/
    /*}*/
    return entities;
  }

  template <class Tuple>
    requires is_tuple_v<Tuple>
  auto exec_res(const char *sql)
      -> std::expected<std::vector<Tuple>, std::error_code> {
    int rc;
    Tuple tup;
    std::vector<Tuple> records;

    statement<0> stmt{*this};
    rc = stmt.prepare(sql);
    if (rc != 0) {
      return std::unexpected(mysql::error::make_error_code(rc));
    }
    result<std::tuple_size_v<Tuple>, 0> res{stmt};
    res.column_for_each(tup);
    rc = res.bind();
    if (rc != 0) {
      return std::unexpected(mysql::error::make_error_code(rc));
    }
    rc = stmt.execute();
    if (rc != 0) {
      return std::unexpected(mysql::error::make_error_code(rc));
    }
    while (res.fetch() == 0) {
      records.push_back(tup);
    }
    return records;
  }

  inline int connect(const char *host, const char *user,
                     const char *passwd, const char* db) noexcept {
    db_ = mysql_init(NULL);
    if (db_ == NULL) {
      return mysql_errno(db_);
    }
    db_ = mysql_real_connect(db_, host, user, passwd, db, 0, NULL, 0);
    if (db_ == NULL) {
      disconnect();
      return mysql_errno(db_);
    }
    return 0;
  }

  inline void disconnect() noexcept {
    if (db_ != nullptr) {
      mysql_close(db_);
      db_ = nullptr;
    }
  }

  inline int64_t last_inserted_rowid() noexcept { return mysql_insert_id(db_); }

  database &operator=(database &&) = default;
  database &operator=(const database &other) = delete;

private:
  template <std::size_t N> friend class statement;

  MYSQL *db_;
};

template <std::size_t N>
statement<N>::statement(database &db) {
  if(db.db_) {
    stmt_ = mysql_stmt_init(db.db_);
  }
}

} // namespace sqlinq::mysql

#endif /* SQLINQ_MYSQL_DATABASE_H_ */
