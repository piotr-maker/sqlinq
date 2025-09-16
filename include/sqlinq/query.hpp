#ifndef SQLINQ_QUERY_HPP_
#define SQLINQ_QUERY_HPP_

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>

#include "query_ast.hpp"
#include "table.hpp"
#include "type_traits.hpp"

namespace sqlinq {

class Database;

inline AggregateResult<int> count() {
  return AggregateResult<int>{AggregateExpr::Function::Count, {}};
}

template <typename Entity, typename T>
inline AggregateResult<int> count(T Entity::*member) {
  constexpr auto table_schema = Table<Entity>::meta();
  std::string_view cname;
  auto offset = reinterpret_cast<std::size_t>(&(((Entity *)0)->*member));
  for (const auto& column : table_schema.columns) {
    if (column.offset() == offset) {
      cname = column.name();
      break;
    }
  }
  return AggregateResult<int>{AggregateExpr::Function::Count, cname};
}

template <class Entity> class InsertQuery {
public:
  using table_t = Table<Entity>;
  InsertQuery(QueryAst &&ast) : ast_(std::move(ast)) {
    ast_.table_name = table_info_.name;
  }

private:
  QueryAst ast_;
  friend class Database;
  static constexpr auto table_info_ = table_t::meta();
};

template <class Entity, typename... Args> class SelectQuery {
public:
  using table_t = Table<Entity>;
  using return_type =
      std::conditional_t<(sizeof...(Args) > 0), std::tuple<Args...>, Entity>;

  SelectQuery(AggregateResult<Args> &&...aggr_fun)
    requires(sizeof...(Args) > 0)
  {
    ast_.op = QueryAst::Operation::Select;
    ast_.table_name = table_info_.name;
    (([&](auto &&fn) { ast_.aggr_expr = fn; })(aggr_fun), ...);
  }

  SelectQuery(ColumnDef<Args> &&...cols) {
    ast_.op = QueryAst::Operation::Select;
    ast_.table_name = table_info_.name;
    (ast_.column_names.push_back(cols.name()), ...);
  }

  SelectQuery fetch(std::size_t n) & {
    ast_.fetch = n;
    return *this;
  }

  SelectQuery fetch(std::size_t n) && {
    ast_.fetch = n;
    return std::move(*this);
  }

  SelectQuery skip(std::size_t n) & {
    ast_.skip = n;
    return *this;
  }

  SelectQuery skip(std::size_t n) && {
    ast_.skip = n;
    return std::move(*this);
  }

  template <typename T, typename... Ts>
  SelectQuery group_by(T Entity::*first, Ts Entity::*...rest) & {
    return group_by_impl(*this, first, rest...);
  }

  template <typename T, typename... Ts>
  SelectQuery group_by(T Entity::*first, Ts Entity::*...rest) && {
    return std::move(group_by_impl(*this, first, rest...));
  }

  template <typename T, typename... Ts>
  SelectQuery &order_by(T Entity::*first, Ts Entity::*...rest) & {
    return order_by_impl(*this, first, rest...);
  }

  template <typename T, typename... Ts>
  SelectQuery order_by(T Entity::*first, Ts Entity::*...rest) && {
    return std::move(order_by_impl(*this, first, rest...));
  }

  SelectQuery where(std::function<FilterChain(table_t)> &&fn) & {
    return where_impl(*this, std::move(fn));
  }

  SelectQuery where(std::function<FilterChain(table_t)> &&fn) && {
    return std::move(where_impl(*this, std::move(fn)));
  }

private:
  QueryAst ast_;
  friend class Database;
  static constexpr auto table_info_ = table_t::meta();

  template <typename Self, typename T, typename... Ts>
  static auto &group_by_impl(Self &&s, T Entity::*first, Ts Entity::*...rest) {
    table_t table;
    auto push_cname = [&](auto member_ptr) {
      auto offset =
          reinterpret_cast<std::size_t>(&(((Entity *)0)->*member_ptr));
      for (const auto &col : table_info_.columns) {
        if (col.offset() == offset) {
          s.ast_.order_expr.push_back(col.name());
        }
      }
    };

    (push_cname(first), ..., push_cname(rest));
    return s;
  }

  template <typename Self, typename T, typename... Ts>
  static auto &order_by_impl(Self &&s, T Entity::*first, Ts Entity::*...rest) {
    table_t table;
    auto push_cname = [&](auto member_ptr) {
      auto offset =
          reinterpret_cast<std::size_t>(&(((Entity *)0)->*member_ptr));
      for (const auto &col : table_info_.columns) {
        if (col.offset() == offset) {
          s.ast_.order_expr.push_back(col.name());
        }
      }
    };

    (push_cname(first), ..., push_cname(rest));
    return s;
  }

  template <typename Self>
  static auto &where_impl(Self &&s, std::function<FilterChain(table_t)> &&fn) {
    table_t table;
    s.ast_.filter_chain = fn(table);
    for (auto &expr : s.ast_.filter_chain) {
      if (expr.kind == FilterExpr::Kind::Leaf) {
        std::size_t idx = expr.condition.index;
        expr.condition.column_name = table_info_.columns[idx].name();
      }
    }
    return s;
  }
};

template <class Entity> class WhereQuery {
public:
  using table_t = Table<Entity>;
  WhereQuery(QueryAst &&ast) : ast_(std::move(ast)) {
    ast_.table_name = table_info_.name;
  }

  WhereQuery where(std::function<FilterChain(table_t)> &&fn) && {
    return std::move(where_impl(*this, std::move(fn)));
  }

  WhereQuery where(std::function<FilterChain(table_t)> &&fn) & {
    return where_impl(*this, std::move(fn));
  }

private:
  QueryAst ast_;
  friend class Database;
  static constexpr auto table_info_ = table_t::meta();

  template <typename Self>
  static auto &where_impl(Self &&s, std::function<FilterChain(table_t)> &&fn) {
    table_t table;
    s.ast_.filter_chain = fn(table);
    for (auto &expr : s.ast_.filter_chain) {
      if (expr.kind == FilterExpr::Kind::Leaf) {
        std::size_t idx = expr.condition.index;
        expr.condition.column_name = table_info_.columns[idx].name();
      }
    }
    return s;
  }
};

// TODO: trzbea określić czym jest ENTITY (concept)
template <class Entity> class Query {
public:
  using table_t = Table<Entity>;
  Query() = default;

  auto insert(std::function<void(table_t &)> fn) {
    table_t table;
    QueryAst ast;
    fn(table);

    zip_apply(structure_to_tuple(table), table_info_.columns,
              [&](auto &col, auto &meta) {
                if (col.is_set()) {
                  ast.column_names.push_back(meta.name());
                  if (col.is_null()) {
                    ast.values.emplace_back(BoundValue{});
                  } else {
                    ast.values.emplace_back(col.value());
                  }
                }
              });

    return InsertQuery<Entity>{std::move(ast)};
  }

  auto remove() -> WhereQuery<Entity> {
    QueryAst ast;
    return WhereQuery<Entity>{std::move(ast)};
  }

  auto select_all() -> SelectQuery<Entity> { return {}; }

  template <typename T>
  auto select(AggregateResult<T> &&aggr) -> SelectQuery<Entity, T> {
    return SelectQuery<Entity, T>{std::move(aggr)};
  }

  template <typename T, typename... Ts>
  auto select(T Entity::*first, Ts Entity::*...rest)
      -> SelectQuery<Entity, T, Ts...> {
    table_t table;

    auto make_col = [&](auto member_ptr) {
      using field_t =
          std::decay_t<decltype(std::declval<Entity>().*member_ptr)>;
      const char *cname = nullptr;

      auto offset =
          reinterpret_cast<std::size_t>(&(((Entity *)0)->*member_ptr));
      for (const auto &col : table_info_.columns) {
        if (col.offset() == offset) {
          cname = col.name();
        }
      }
      return ColumnDef<field_t>{cname};
    };

    return SelectQuery<Entity, T, Ts...>{make_col(first), make_col(rest)...};
  }

  auto update(std::function<void(table_t &)> fn) {
    table_t table;
    QueryAst ast;
    fn(table);

    zip_apply(structure_to_tuple(table), table_info_.columns,
              [&](auto &col, auto &meta) {
                if (col.is_set()) {
                  ast.column_names.push_back(meta.name());
                  if (col.is_null()) {
                    ast.values.emplace_back(BoundValue{});
                  } else {
                    ast.values.emplace_back(col.value());
                  }
                }
              });

    ast.op = QueryAst::Operation::Update;
    return WhereQuery<Entity>{std::move(ast)};
  }

private:
  static constexpr auto table_info_ = table_t::meta();

  template <typename Tuple, std::size_t N, typename Pred>
  void zip_apply(Tuple &&tup, const ColumnSet<N> &info, Pred pred) {
    std::apply(
        [&](auto &...col) {
          [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
            (pred(col, info[Idx]), ...);
          }(std::index_sequence_for<decltype(col)...>{});
        },
        tup);
  }
};

} // namespace sqlinq

#endif /* SQLINQ_QUERY_HPP_ */
