#ifndef SQLINQ_QUERY_HPP_
#define SQLINQ_QUERY_HPP_

#include <tuple>
#include <vector>
#include <utility>
#include <sstream>
#include <iostream>
#include <string_view>

#include "sqlinq/utility.hpp"
#include "sqlite/binding.hpp"

namespace sqlinq {

template <typename T, typename U, typename... Args>
concept Database = requires(T t, U u, const char *sql) {
  { t.exec(sql) } -> std::same_as<void>;
  { t.exec(sql, u) } -> std::same_as<void>;
  { t.template exec_res<U>(sql) } -> std::same_as<std::vector<U>>;
  { t.template exec_res<std::tuple<Args...>>(sql)}
    -> std::same_as<std::vector<std::tuple<Args...>>>;
};

template <class Database, class Entity> class Query {
public:
  Query(Database &db) : db_(&db) {}

  auto count(const std::string_view column = "*") -> std::size_t {
    std::stringstream sql;
    sql << "SELECT COUNT(" << column;
    sql << ") FROM " << tableName_ << ss_.str();
    ss_.str(std::string{});
    auto result = db_->template exec_res<std::tuple<int>>(sql.str().c_str());
    return std::get<0>(result[0]);
  }

  auto insert_into(Entity &entity) -> int {
    std::stringstream sql;
    sql << "INSERT INTO ";
    sql << tableName_;
    sql << " VALUES(" << binding_string() << ')';
    auto tup = structure_to_tuple(std::forward<Entity>(entity));
    db_->exec(sql.str().c_str(), tup);
    return db_->last_inserted_rowid();
  }

  auto select() -> std::vector<Entity> {
    std::string sql{"SELECT * FROM " + std::string{tableName_} + ' ' +
                    ss_.str()};
    ss_.str(std::string{});
    return db_->template exec_res<Entity>(sql.c_str());
  }

  template <typename... Args, std::size_t N>
  auto select(const std::string_view (&column_names)[N])
      -> std::vector<std::tuple<Args...>> {
    static_assert(sizeof...(Args) != 0,
                  "Result tuple parameter list count cannot be 0");
    static_assert(
        sizeof...(Args) == N,
        "Result tuple parameter list count mismatch column names count");
    std::stringstream sql;
    sql << "SELECT ";
    for (std::size_t i = 0; i < N - 1; i++) {
      sql << column_names[i] << ',';
    }
    sql << column_names[N - 1];
    sql << " FROM " << tableName_ << ss_.str();
    ss_.str(std::string{});
    return db_->template exec_res<std::tuple<Args...>>(sql.str().c_str());
  }

  auto group_by(const char *column) -> Query & {
    ss_ << " GROUP BY " << column;
    return *this;
  }

  auto limit(int n) -> Query & {
    ss_ << " LIMIT " << n;
    return *this;
  }

  auto order_by(const char *column) -> Query & {
    ss_ << " ORDER BY " << column;
    return *this;
  }

  auto where(const char *condition) -> Query & {
    ss_ << " WHERE " << condition;
    return *this;
  }

private:
  Database *db_;
  std::stringstream ss_;
  static constexpr std::string_view tableName_ = template_type_name<Entity>();

  constexpr auto binding_string() -> std::string_view {
    Entity entity;
    auto tup = structure_to_tuple(entity);
    constexpr std::size_t N = std::tuple_size_v<decltype(tup)>;
    constexpr auto &value = sqlite::binding_str_holder<Entity, N>::value;
    return std::string_view{value.data(), value.size()};
  }
};

} // namespace sqlinq

#endif /* SQLINQ_QUERY_HPP_ */
