#ifndef SQLINQ_DATABASE_HPP_
#define SQLINQ_DATABASE_HPP_

#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "sqlinq/backend/backend_iface.hpp"
#include "type_traits.hpp"
#include <sqlinq/types.h>

#define SQLITE_DATA_TRUNCATED 102

namespace sqlinq {
template <typename T, std::size_t N> struct Buffer {};

template <std::size_t N> class Statement {
public:
  using BindDataStorage =
      std::conditional_t<N == 0, std::nullptr_t, std::array<BindData, N>>;

  Statement(BackendIface &db) : backend_(db) { backend_.stmt_init(); }

  ~Statement() { backend_.stmt_close(); }

  Statement(Statement &&) = default;
  Statement(const Statement &) = delete;

  template <typename T> void bind(const int, T &) {
    static_assert(std::is_same_v<T, void>,
                  "Result::bind(): The specified type is not supported");
  }

  template <string_like T> inline void bind(const int index, T &&text) {
    std::string_view sv = std::forward<T>(text);
    bs_[index].type = BindData::Text;
    bs_[index].buffer = (void *)sv.data();
    bs_[index].buffer_length = sv.size();
    bs_[index].is_null = nullptr;
    bs_[index].length = nullptr;
  }

  template <class T>
  inline void bind(const int index, std::optional<T> &&value) {
    if (value.has_value()) {
      bind(index, std::forward<T>(value.value()));
      return;
    }
    bs_[index].type = BindData::Null;
    bs_[index].buffer = nullptr;
    bs_[index].is_null = nullptr;
    bs_[index].length = nullptr;
  }

  template <std::integral Int>
  constexpr void bind(const int index, const Int &value) noexcept {
    BindData::Type type;
    if constexpr (sizeof(Int) == 1) {
      type = BindData::Tiny;
    } else if (sizeof(Int) == 2) {
      type = BindData::Short;
    } else if (sizeof(Int) == 4) {
      type = BindData::Long;
    } else if (sizeof(Int) == 8) {
      type = BindData::LongLong;
    }
    bs_[index].type = type;
    bs_[index].buffer = (void *)&value;
    bs_[index].is_null = nullptr;
    bs_[index].length = nullptr;
  }

  template <std::floating_point Float>
  void bind(const int index, const Float &value) noexcept {
    BindData::Type type;
    if constexpr (std::is_same_v<Float, float>) {
      type = BindData::Float;
    } else if (std::is_same_v<Float, double>) {
      type = BindData::Double;
    }
    bs_[index].type = type;
    bs_[index].buffer = (void *)&value;
    bs_[index].is_null = nullptr;
    bs_[index].length = nullptr;
  }

  template <std::size_t Precision, std::size_t Scale>
  void bind(const int index, const Decimal<Precision, Scale> &value) noexcept {
    bs_[index].type = BindData::Decimal;
    bs_[index].buffer = (void *)&value;
    bs_[index].is_null = nullptr;
    bs_[index].length = nullptr;
  }

  inline ExecStatus execute() { return backend_.stmt_execute(); }

  inline void prepare(std::string_view sql) { backend_.stmt_prepare(sql); }

  Statement &operator=(Statement &&) = default;
  Statement &operator=(const Statement &) = delete;

private:
  BackendIface &backend_;
  BindDataStorage bs_;

  template <typename Tuple, std::size_t... Idx>
  void bind_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (bind(Idx, std::get<Idx>(tup)), ...);
  }
};

template <std::size_t N> class Result {
public:
  Result(BackendIface &db) : backend_(db) {
    memset(length_, 0, sizeof(length_));
    memset(is_null_, 0, sizeof(is_null_));
    memset(error_, 0, sizeof(error_));
  }

  Result(Result &&) = default;
  Result(const Result &) = delete;

  template <typename T> void column(const int, T &) {
    static_assert(std::is_same_v<T, void>,
                  "Result::column(): The specified type is not supported");
  }

  template <class T>
  inline void column(const int index, std::optional<T> &val) noexcept {
    T &ref = val.emplace();
    column(index, ref);
  }

  inline void column(const int index, bool &v) noexcept {
    bd_[index].type = BindData::Bit;
    bd_[index].buffer = &v;
    bd_[index].buffer_length = 0;
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  inline void column(const int index, std::string &) noexcept {
    bd_[index].type = BindData::Text;
    bd_[index].buffer = nullptr;
    bd_[index].buffer_length = 0;
    bd_[index].length = &length_[index];
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  inline void column(const int index, Blob &) noexcept {
    bd_[index].type = BindData::Blob;
    bd_[index].buffer = nullptr;
    bd_[index].buffer_length = 0;
    bd_[index].length = &length_[index];
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  template <std::integral Int>
  constexpr void column(const int index, Int &value) noexcept {
    BindData::Type type;
    if constexpr (sizeof(Int) == 1) {
      type = BindData::Tiny;
    } else if (sizeof(Int) == 2) {
      type = BindData::Short;
    } else if (sizeof(Int) == 4) {
      type = BindData::Long;
    } else if (sizeof(Int) == 8) {
      type = BindData::LongLong;
    }
    bd_[index].type = type;
    bd_[index].buffer = (void *)&value;
    bd_[index].is_null = &is_null_[index];
    bd_[index].error = &error_[index];
  }

  template <std::floating_point Float>
  void column(const int index, Float &value) noexcept {
    BindData::Type type;
    if constexpr (std::is_same_v<Float, float>) {
      type = BindData::Float;
    } else if (std::is_same_v<Float, double>) {
      type = BindData::Double;
    }
    bd_[index].type = type;
    bd_[index].buffer = (void *)&value;
    bd_[index].is_null = &is_null_[index];
    bd_[index].length = 0;
    bd_[index].error = &error_[index];
  }

  template <std::size_t Precision, std::size_t Scale>
  void column(const int index, Decimal<Precision, Scale> &value) noexcept {
    bd_[index].type = BindData::Decimal;
    bd_[index].buffer = (void *)&value;
    bd_[index].is_null = &is_null_[index];
    bd_[index].length = 0;
    bd_[index].error = &error_[index];
  }

  template <typename T>
  inline void fetch(const int index, std::optional<T> &val) noexcept {
    if (is_null_[index]) {
      val.reset();
      return;
    }

    if (bd_[index].type == BindData::Text || bd_[index].type == BindData::Blob) {
      T &ref = val.emplace();
      fetch(index, ref);
    }
  }

  inline void fetch(const int index, Blob &blob) noexcept {
    BindData bind = bd_[index];
    blob.resize(length_[index]);
    bind.buffer = (void *)blob.data();
    bind.buffer_length = blob.size();
    backend_.stmt_fetch_column(index, bind);
  }

  inline void fetch(const int index, std::string &text) noexcept {
    BindData bind = bd_[index];
    text.resize(length_[index]);
    bind.buffer = (void *)text.data();
    bind.buffer_length = text.size();
    backend_.stmt_fetch_column(index, bind);
  }

  template <typename T> inline void fetch(const int, T &) noexcept {}

  template <typename Tuple> void bind_result(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    memset(bd_, 0, sizeof(bd_));
    column_for_each_impl(tup, std::make_index_sequence<tup_size>{});
    backend_.bind_result(bd_, N);
  }

  inline ExecStatus fetch() { return backend_.stmt_fetch(); }

  template <typename Tuple> void fetch_for_each(Tuple &tup) {
    constexpr std::size_t tup_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    fetch_for_each_impl(tup, std::make_index_sequence<tup_size>{});
  }

  Result &operator=(Result &&) = default;
  Result &operator=(const Result &) = delete;

private:
  BackendIface &backend_;
  BindData bd_[N];
  bool error_[N];
  bool is_null_[N];
  std::size_t length_[N];

  template <typename Tuple, std::size_t... Idx>
  void column_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (column(Idx, std::get<Idx>(tup)), ...);
  }

  template <typename Tuple, std::size_t... Idx>
  void fetch_for_each_impl(Tuple &tup, std::index_sequence<Idx...>) {
    (fetch(Idx, std::get<Idx>(tup)), ...);
  }
};

class Database {
public:
  Database(BackendIface &backend) : backend_(backend) {}

  void exec(const char *sql) {
    Statement<0> stmt{backend_};
    stmt.prepare(sql);
    stmt.execute();
  }

  template <class Tuple> void exec(const char *sql, Tuple &&tup) {
    static_assert(is_tuple_v<std::remove_reference_t<Tuple>>,
                  "Database::exec(): Type is not std::tuple");
    constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    Statement<N> stmt{backend_};
    stmt.prepare(sql);
    std::apply(
        [&stmt](auto &&...args) {
          int i = 0;
          (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
        },
        std::forward<Tuple>(tup));
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
                  "Database::exec_res(): Type is not std::tuple");
    Entity entity;
    std::vector<Entity> entities;

    constexpr std::size_t bind_tuple_size =
        std::tuple_size_v<std::remove_reference_t<Tuple>>;
    Statement<bind_tuple_size> stmt{backend_};
    stmt.prepare(sql);
    std::apply(
        [&stmt](auto &&...args) {
          int i = 0;
          (stmt.bind(i++, std::forward<decltype(args)>(args)), ...);
        },
        std::forward<Tuple>(tup));
    ExecStatus status = stmt.execute();

    auto params = structure_to_tuple(entity);
    Result<std::tuple_size_v<decltype(params)>> res{backend_};
    res.bind_result(params);
    while (1) {
      status = res.fetch();
      if (status == ExecStatus::NoData || status == ExecStatus::Error) {
        break;
      }
      if (status == ExecStatus::Truncated) {
        res.fetch_for_each(params);
      }
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

    Statement<0> stmt{backend_};
    stmt.prepare(sql);
    ExecStatus status = stmt.execute();

    Result<std::tuple_size_v<Tuple>> res{backend_};
    res.bind_result(tup);
    while (1) {
      status = res.fetch();
      if (status == ExecStatus::NoData || status == ExecStatus::Error) {
        break;
      }
      if (status == ExecStatus::Truncated) {
        res.fetch_for_each(tup);
      }
      records.push_back(tup);
    }
    return records;
  }

  uint64_t last_inserted_rowid() const noexcept {
    return backend_.last_inserted_rowid();
  }

private:
  BackendIface &backend_;
};
} // namespace sqlinq

#endif // SQLINQ_DATABASE_HPP_
