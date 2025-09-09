#ifndef SQLINQ_CURSOR_HPP_
#define SQLINQ_CURSOR_HPP_

#include <cstddef>
#include <tuple>

#include "backend/backend_iface.hpp"
#include "types/blob.hpp"

namespace sqlinq {

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
    bd_[index].type = column::Type::Bit;
    bd_[index].buffer = &v;
    bd_[index].buffer_length = 0;
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  inline void column(const int index, std::string &) noexcept {
    bd_[index].type = column::Type::Text;
    bd_[index].buffer = nullptr;
    bd_[index].buffer_length = 0;
    bd_[index].length = &length_[index];
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  inline void column(const int index, Blob &) noexcept {
    bd_[index].type = column::Type::Blob;
    bd_[index].buffer = nullptr;
    bd_[index].buffer_length = 0;
    bd_[index].length = &length_[index];
    bd_[index].error = &error_[index];
    bd_[index].is_null = &is_null_[index];
  }

  template <std::integral Int>
  constexpr void column(const int index, Int &value) noexcept {
    column::Type type;
    if constexpr (sizeof(Int) == 1) {
      type = column::Type::TinyInt;
    } else if (sizeof(Int) == 2) {
      type = column::Type::SmallInt;
    } else if (sizeof(Int) == 4) {
      type = column::Type::Int;
    } else if (sizeof(Int) == 8) {
      type = column::Type::BigInt;
    }
    bd_[index].type = type;
    bd_[index].buffer = (void *)&value;
    bd_[index].is_null = &is_null_[index];
    bd_[index].error = &error_[index];
  }

  template <std::floating_point Float>
  void column(const int index, Float &value) noexcept {
    column::Type type;
    if constexpr (std::is_same_v<Float, float>) {
      type = column::Type::Float;
    } else if (std::is_same_v<Float, double>) {
      type = column::Type::Double;
    }
    bd_[index].type = type;
    bd_[index].buffer = (void *)&value;
    bd_[index].is_null = &is_null_[index];
    bd_[index].length = 0;
    bd_[index].error = &error_[index];
  }

  template <std::size_t Precision, std::size_t Scale>
  void column(const int index, Decimal<Precision, Scale> &value) noexcept {
    bd_[index].type = column::Type::Decimal;
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

    if (bd_[index].type == column::Type::Text ||
        bd_[index].type == column::Type::Blob) {
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

template <typename T> class CursorTraits {
public:
  using value_type = T;
  template <typename Derived> static value_type &current(Derived *cursor) {
    return cursor->row_;
  }
};

template <typename Derived, typename T> class CursorBase {
protected:
  CursorBase() = default;

public:
  using traits = CursorTraits<T>;
  using value_type = typename traits::value_type;

  CursorBase(CursorBase &&) = default;
  CursorBase &operator=(CursorBase &&) = default;

  CursorBase(const CursorBase &) = delete;
  CursorBase &operator=(const CursorBase &) = delete;

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    iterator(Derived *cur, bool end = false) : cur_(cur), end_(end) {
      if (!end_)
        ++(*this);
    }

    value_type &operator*() { return traits::current(cur_); }
    value_type *operator->() { return &traits::current(cur_); }

    iterator &operator++() {
      if (!cur_->next())
        end_ = true;
      return *this;
    }

    bool operator==(const iterator &other) const noexcept {
      return end_ == other.end_;
    }

    bool operator!=(const iterator &other) const noexcept {
      return !(*this == other);
    }

  private:
    Derived *cur_;
    bool end_;
  };

  iterator begin() { return iterator(static_cast<Derived *>(this), false); }
  iterator end() { return iterator(static_cast<Derived *>(this), true); }
  value_type &current() {
    return traits::current(static_cast<Derived *>(this));
  }
};

template <typename Entity>
class Cursor : public CursorBase<Cursor<Entity>, Entity> {
public:
  using value_type = Entity;
  using base_type = CursorBase<Cursor<value_type>, value_type>;
  using base_type::base_type;

  Cursor(BackendIface &db) : db_(db), res_(db) {}
  ~Cursor() { db_.stmt_close(); }
  bool has_next() const noexcept { return status_ == ExecStatus::Row; }

  bool next() {
    auto fields = structure_to_tuple(row_);
    res_.bind_result(fields);
    status_ = res_.fetch();
    if (status_ == ExecStatus::Truncated) {
      res_.fetch_for_each(fields);
      row_ = to_struct<Entity>(fields);
      status_ = ExecStatus::Row;
    }
    return status_ == ExecStatus::Row;
  }

private:
  BackendIface &db_;
  ExecStatus status_;
  Entity row_;
  Result<
      std::tuple_size_v<decltype(structure_to_tuple(std::declval<Entity &>()))>>
      res_;
  friend class CursorTraits<value_type>;
};

template <typename... Ts>
class Cursor<std::tuple<Ts...>>
    : public CursorBase<Cursor<std::tuple<Ts...>>, std::tuple<Ts...>> {
public:
  using value_type = std::tuple<Ts...>;
  using base_type = CursorBase<Cursor<value_type>, value_type>;
  using base_type::base_type;

  Cursor(BackendIface &db) : db_(db), res_(db) { res_.bind_result(row_); }
  ~Cursor() { db_.stmt_close(); }
  bool has_next() const noexcept { return status_ == ExecStatus::Row; }

  bool next() {
    status_ = res_.fetch();
    if (status_ == ExecStatus::Truncated) {
      res_.fetch_for_each(row_);
      status_ = ExecStatus::Row;
    }
    return status_ == ExecStatus::Row;
  }

private:
  BackendIface &db_;
  ExecStatus status_;
  std::tuple<Ts...> row_;
  Result<sizeof...(Ts)> res_;
  friend class CursorTraits<value_type>;
};
} // namespace sqlinq

#endif // SQLINQ_CURSOR_HPP_
