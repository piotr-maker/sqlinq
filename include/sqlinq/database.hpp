#ifndef SQLINQ_DATABASE_HPP_
#define SQLINQ_DATABASE_HPP_

#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "backend/backend_iface.hpp"
#include "query.hpp"
#include "query_ast.hpp"
#include "sql_generator.hpp"
#include "sqlinq/cursor.hpp"

#define SQLITE_DATA_TRUNCATED 102

namespace sqlinq {
template <std::size_t... Is>
constexpr auto store_string_impl(const std::string &str,
                                 std::index_sequence<Is...>) {
  return std::array{str[Is]...};
}

template <std::size_t N> constexpr auto store_string(const std::string &&str) {
  return store_string_impl(str, std::make_index_sequence<N>{});
}

class Database {
public:
  Database(BackendIface &backend) : backend_(backend) {}

  template <typename Entity> auto create(Entity &entity) {
    static constexpr auto table_schema = Table<Entity>::meta();
    static constexpr auto cols = table_schema.columns.filter(
        [](ColumnInfo const &c) { return !c.is_autoincrement(); });
    std::vector<std::string_view> columns;
    std::vector<BoundValue> params;
    for (auto &col : cols.span()) {
      columns.emplace_back(col.name());
      params.emplace_back(bind_value(entity, col));
    }
    std::string query = SqlGenerator::build_insert(table_schema.name, columns);
    /*static constexpr auto query = store_string<*/
    /*    SqlGenerator::build_insert(table_schema.name, columns).size()>(*/
    /*    SqlGenerator::build_insert(table_schema.name, columns));*/
    std::cout << std::string_view{query.data(), query.size()} << '\n';
    backend_.stmt_init();
    backend_.stmt_prepare(std::string_view{query.data(), query.size()});
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    backend_.stmt_close();
    entity.id = static_cast<int>(backend_.last_inserted_rowid());
    return entity;
  }

  template <typename Entity>
  [[nodiscard]] auto find(auto &&val) -> std::optional<Entity> {
    static constexpr auto table_schema = Table<Entity>::meta();
    using pk_type = typename decltype(table_schema)::pk_type;
    static_assert(std::is_same_v<std::decay_t<decltype(val)>, pk_type>,
                  "find() must be called with pk_type");

    static constexpr auto pk_cols = table_schema.columns.filter(
        [](ColumnInfo const &c) { return c.is_primary_key(); });
    QueryAst ast;
    ast.table_name = table_schema.name;
    ast.filter_chain = FilterChain{
        FilterExpr::Kind::Leaf, ValueCondition{ValueCondition::Operator::Equal,
                                               BoundValue{std::move(val)}, 0}};
    ast.filter_chain.front().condition.column_name = pk_cols.span()[0].name();
    std::vector<BoundValue> params = ast.filter_chain.extract_values();

    std::string query = SqlGenerator::build_select(ast);
    std::cout << std::string_view{query.data(), query.size()} << '\n';
    backend_.stmt_init();
    backend_.stmt_prepare(std::string_view{query.data(), query.size()});
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();

    Cursor<Entity> cursor{backend_};
    if (!cursor.next()) {
      return std::nullopt;
    }
    Entity &e = cursor.current();
    return e;
  }

  template <typename Entity>
  auto get_all(int skip = 0, int fetch = 50) -> Cursor<Entity> {
    static constexpr auto table_schema = Table<Entity>::meta();
    QueryAst ast;
    ast.table_name = table_schema.name;
    ast.skip = skip;
    ast.fetch = fetch;
    std::string sql = SqlGenerator::build_select(ast);
    backend_.stmt_init();
    backend_.stmt_prepare(sql);
    /*backend_.bind_params(std::span{params.data(), params.size()});*/
    backend_.stmt_execute();
    return Cursor<Entity>{backend_};
  }

  template <typename Entity> void remove(auto &&val) {
    static constexpr auto table_schema = Table<Entity>::meta();
    using pk_type = typename decltype(table_schema)::pk_type;
    static_assert(std::is_same_v<std::decay_t<decltype(val)>, pk_type>,
                  "remove() must be called with pk_type");

    static constexpr auto pk_cols = table_schema.columns.filter(
        [](ColumnInfo const &c) { return c.is_primary_key(); });
    QueryAst ast;
    ast.table_name = table_schema.name;
    ast.filter_chain = FilterChain{
        FilterExpr::Kind::Leaf, ValueCondition{ValueCondition::Operator::Equal,
                                               BoundValue{std::move(val)}, 0}};
    ast.filter_chain.front().condition.column_name = pk_cols.span()[0].name();
    std::vector<BoundValue> params = ast.filter_chain.extract_values();

    std::string query = SqlGenerator::build_delete(ast);
    std::cout << std::string_view{query.data(), query.size()} << '\n';
    backend_.stmt_init();
    backend_.stmt_prepare(std::string_view{query.data(), query.size()});
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    backend_.stmt_close();
  }

  template <typename Entity> void update(const Entity &entity) {
    static constexpr auto table_schema = Table<Entity>::meta();
    static constexpr auto pk_cols = table_schema.columns.filter(
        [](ColumnInfo const &c) { return c.is_primary_key(); });
    static constexpr auto cols =
        table_schema.columns.filter([](ColumnInfo const &c) {
          return !c.is_primary_key() && !c.is_autoincrement();
        });

    QueryAst ast;
    ast.table_name = table_schema.name;
    ast.filter_chain =
        FilterChain{FilterExpr::Kind::Leaf,
                    ValueCondition{ValueCondition::Operator::Equal,
                                   bind_value(entity, pk_cols.span()[0]), 0}};
    ast.filter_chain.front().condition.column_name = pk_cols.span()[0].name();
    std::vector<BoundValue> params;

    std::vector<std::string_view> columns;
    for (auto &col : cols.span()) {
      ast.column_names.push_back(col.name());
      params.emplace_back(bind_value(entity, col));
    }
    params.emplace_back(std::move(ast.filter_chain.front().condition.value));
    std::string query = SqlGenerator::build_update(ast);
    std::cout << std::string_view{query.data(), query.size()} << '\n';
    backend_.stmt_init();
    backend_.stmt_prepare(std::string_view{query.data(), query.size()});
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    backend_.stmt_close();
  }

  uint64_t last_inserted_rowid() const noexcept {
    return backend_.last_inserted_rowid();
  }

  template <typename Entity> void execute(InsertQuery<Entity> &q) {
    std::string sql =
        SqlGenerator::build_insert(q.ast_.table_name, q.ast_.column_names);
    std::cout << sql << '\n';
    std::vector<BoundValue> params = std::move(q.ast_.values);
    backend_.stmt_init();
    backend_.stmt_prepare(sql);
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    backend_.stmt_close();
  }

  template <typename Entity> void execute(WhereQuery<Entity> &q) {
    std::string sql = (q.ast_.op == QueryAst::Operation::Update)
                          ? SqlGenerator::build_update(q.ast_)
                          : SqlGenerator::build_delete(q.ast_);
    std::cout << sql << '\n';
    std::vector<BoundValue> params = std::move(q.ast_.values);
    for (auto &&v : q.ast_.filter_chain.extract_values()) {
      params.emplace_back(std::move(v));
    }
    backend_.stmt_init();
    backend_.stmt_prepare(sql);
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    backend_.stmt_close();
  }

  template <typename Entity, typename... Ts>
  [[nodiscard]] auto execute(SelectQuery<Entity, Ts...> &q) {
    using return_type =
        std::conditional_t<(sizeof...(Ts) > 0), std::tuple<Ts...>, Entity>;
    std::string sql = SqlGenerator::build_select(q.ast_);
    std::cout << sql << '\n';
    std::vector<BoundValue> params = q.ast_.filter_chain.extract_values();
    backend_.stmt_init();
    backend_.stmt_prepare(sql);
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();
    return Cursor<return_type>{backend_};
  }

  template <typename Entity, typename... Ts>
  [[nodiscard]] auto to_vector(SelectQuery<Entity, Ts...> &q) {
    using return_type =
        std::conditional_t<(sizeof...(Ts) > 0), std::tuple<Ts...>, Entity>;
    std::string sql = SqlGenerator::build_select(q.ast_);
    std::cout << sql << '\n';
    std::vector<BoundValue> params = q.ast_.filter_chain.extract_values();
    backend_.stmt_init();
    backend_.stmt_prepare(sql);
    backend_.bind_params(std::span{params.data(), params.size()});
    backend_.stmt_execute();

    std::vector<return_type> entities;
    Cursor<return_type> cursor{backend_};
    while (cursor.next()) {
      auto &row = cursor.current();
      entities.emplace_back(std::move(row));
    }
    return entities;
  }

private:
  BackendIface &backend_;

  template <typename Entity>
  BoundValue bind_value(const Entity &entity, const ColumnInfo &info) {
    std::size_t size = 0;
    const void *data_ptr = nullptr;
    const char *field_addr =
        reinterpret_cast<const char *>(&entity) + info.offset();
    if (info.type() == column::Type::Text) {
      std::string *s = (std::string *)(field_addr);
      size = s->size();
      data_ptr = s->data();
    } else if (info.type() == column::Type::Blob) {
      Blob *b = (Blob *)(field_addr);
      size = b->size();
      data_ptr = b->data();
    } else {
      data_ptr = (void *)field_addr;
    }
    return BoundValue{data_ptr, size, info.type()};
  }
};
} // namespace sqlinq

#endif // SQLINQ_DATABASE_HPP_
