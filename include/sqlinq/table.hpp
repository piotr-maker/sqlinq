#ifndef SQLINQ_TABLE_HPP_
#define SQLINQ_TABLE_HPP_

#include <array>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>

#include "detail/type_traits.hpp"
#include "types/datetime.hpp"
#include "types/decimal.hpp"

#define SQLINQ_COLUMN_META(CLASS, MEMBER, NAME)                                \
  sqlinq::make_column(&CLASS::MEMBER, NAME, offsetof(CLASS, MEMBER))

namespace sqlinq {
template <typename Entity> struct Table {};

namespace column {
enum class Type : short {
  Null,
  Bit,
  TinyInt,
  SmallInt,
  Int,
  BigInt,
  Float,
  Double,
  Decimal,
  Blob,
  Text,
  Date,
  Time,
  Datetime,
  Timestamp
};

enum class Options : char {
  None = 0,
  AutoIncrement = 1 << 0,
  Optional = 1 << 1,
  PrimaryKey = 1 << 2,
  Unique = 1 << 3,
  ForeignKey = 1 << 4
};

constexpr Options operator|(const Options lhs, const Options rhs) noexcept {
  return static_cast<Options>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr Options operator&(const Options lhs, const Options rhs) noexcept {
  return static_cast<Options>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr Options &operator|=(Options &lhs, const Options &rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

constexpr Options &operator&=(Options &lhs, const Options &rhs) noexcept {
  lhs = lhs & rhs;
  return lhs;
}
} // namespace column

class ColumnInfo {
public:
  constexpr ColumnInfo() = default;
  constexpr ColumnInfo(const char *name, uint32_t offset, column::Type type,
                       column::Options opts)
      : name_(name), offset_(offset), type_(type), options_(opts) {}

  constexpr auto is_autoincrement() const noexcept -> bool {
    using namespace column;
    return (options_ & Options::AutoIncrement) == Options::AutoIncrement;
  }

  constexpr auto is_primary_key() const noexcept -> bool {
    using namespace column;
    return (options_ & Options::PrimaryKey) == Options::PrimaryKey;
  }

  constexpr auto is_required() const noexcept -> bool {
    using namespace column;
    return (options_ & Options::Optional) == Options::None;
  }

  constexpr auto is_unique() const noexcept -> bool {
    using namespace column;
    return (options_ & Options::Unique) == Options::Unique;
  }

  constexpr auto offset() const noexcept -> uint32_t { return offset_; }
  constexpr auto name() const noexcept -> const char * { return name_; }
  constexpr auto type() const noexcept -> column::Type { return type_; }

private:
  const char *name_;
  uint32_t offset_;
  column::Type type_;
  column::Options options_;
};

namespace details {
template <typename T> struct column_type_map {
  static consteval column::Type value() {
    static_assert(sizeof(T) == 0,
                  "column_type_map: The specified type is not supported");
    return column::Type::Bit;
  }
};

template <> struct column_type_map<bool> {
  static consteval column::Type value() { return column::Type::Bit; }
};
template <> struct column_type_map<int8_t> {
  static consteval column::Type value() { return column::Type::TinyInt; }
};
template <> struct column_type_map<int16_t> {
  static consteval column::Type value() { return column::Type::SmallInt; }
};
template <> struct column_type_map<int32_t> {
  static consteval column::Type value() { return column::Type::Int; }
};
template <> struct column_type_map<int64_t> {
  static consteval column::Type value() { return column::Type::BigInt; }
};
template <> struct column_type_map<float> {
  static consteval column::Type value() { return column::Type::Float; }
};
template <> struct column_type_map<double> {
  static consteval column::Type value() { return column::Type::Double; }
};
template <std::size_t P, std::size_t S> struct column_type_map<Decimal<P, S>> {
  static consteval column::Type value() { return column::Type::Decimal; }
};
template <> struct column_type_map<std::vector<std::byte>> {
  static consteval column::Type value() { return column::Type::Blob; }
};
template <> struct column_type_map<std::string> {
  static consteval column::Type value() { return column::Type::Text; }
};
template <> struct column_type_map<Date> {
  static consteval column::Type value() { return column::Type::Date; }
};
template <> struct column_type_map<Time> {
  static consteval column::Type value() { return column::Type::Time; }
};
template <> struct column_type_map<Datetime> {
  static consteval column::Type value() { return column::Type::Datetime; }
};
template <> struct column_type_map<Timestamp> {
  static consteval column::Type value() { return column::Type::Timestamp; }
};

template <typename T> static constexpr column::Type column_type_of() {
  return column_type_map<T>::value();
}
} // namespace details

template <typename Class, typename T, bool IsPk = false> class ColumnMeta {
public:
  using class_type = Class;
  static constexpr bool is_primary_key_v = IsPk;
  using value_t = std::conditional_t<detail::is_optional_v<T>,
                                     detail::optional_value_t<T>, T>;

  constexpr ColumnMeta(T Class::*mem, const char *name, uint32_t offset,
                       column::Options opts = column::Options::None)
      : name_(name), member_(mem), offset_(offset), opts_(opts) {}

  constexpr auto name() const noexcept -> const char * { return name_; }
  constexpr auto offset() const noexcept -> uint32_t { return offset_; }

  [[nodiscard]] constexpr ColumnMeta autoincrement() noexcept {
    return ColumnMeta<Class, T, IsPk>{member_, name_, offset_,
                                      opts_ | column::Options::AutoIncrement};
  }

  [[nodiscard]] constexpr auto foreign_key() noexcept {
    return ColumnMeta<Class, T, IsPk>{member_, name_, offset_,
                                      opts_ | column::Options::ForeignKey};
  }

  [[nodiscard]] constexpr auto primary_key() noexcept {
    return ColumnMeta<Class, T, true>{member_, name_, offset_,
                                      opts_ | column::Options::PrimaryKey};
  }

  [[nodiscard]] constexpr ColumnMeta unique() noexcept {
    return ColumnMeta<Class, T, IsPk>{member_, name_, offset_,
                                      opts_ | column::Options::Unique};
  }

  consteval ColumnInfo info() const noexcept {
    column::Options opts{opts_};
    if constexpr (detail::is_optional_v<value_t>) {
      opts |= column::Options::Optional;
    }
    return ColumnInfo{name_, offset_, details::column_type_of<value_t>(), opts};
  }

private:
  const char *name_;
  T Class::*member_;
  uint32_t offset_;
  column::Options opts_;
};

template <std::size_t N> class ColumnSet {
public:
  using iterator = std::array<ColumnInfo, N>::iterator;
  using const_iterator = std::array<ColumnInfo, N>::const_iterator;

  constexpr ColumnSet(std::array<ColumnInfo, N> cols) : cols_(cols) {}

  constexpr iterator begin() { return cols_.begin(); }
  constexpr const_iterator begin() const { return cols_.begin(); }
  constexpr iterator end() { return cols_.end(); }
  constexpr const_iterator end() const { return cols_.end(); }

  constexpr ColumnInfo *data() { return cols_.data(); }
  constexpr const ColumnInfo *data() const { return cols_.data(); }
  constexpr bool empty() const noexcept { return cols_.empty(); }
  constexpr std::size_t size() const { return cols_.size(); }

  constexpr const ColumnInfo &operator[](std::size_t idx) const {
    return cols_[idx];
  }

  template <typename Pred> consteval auto filter(Pred p) const noexcept {
    auto [arr, size] = filter_impl(p, std::make_index_sequence<N>{});
    struct View {
      std::array<ColumnInfo, N> storage;
      std::size_t size;
      consteval auto span() const { return std::span{storage.data(), size}; }
    };
    return View{arr, size};
  }

private:
  std::array<ColumnInfo, N> cols_;

  template <typename Pred, std::size_t... Is>
  consteval auto filter_impl(Pred p, std::index_sequence<Is...>) const {
    std::size_t M = ((p(cols_[Is]) ? 1 : 0) + ... + 0);
    std::array<ColumnInfo, N> result{};
    std::size_t i = 0;
    ((p(cols_[Is]) ? (result[i++] = cols_[Is], 0) : 0), ...);
    return std::tuple{result, M};
  }
};

template <typename Class, std::size_t N, typename T> struct TableSchema {
  using pk_type = T;
  static constexpr std::size_t column_count = N;
  const char *name;
  ColumnSet<N> columns{};
  ColumnMeta<Class, T, true> pk_column;

  consteval explicit TableSchema(const char *tname,
                                 std::array<ColumnInfo, N> cols,
                                 ColumnMeta<Class, T, true> pk_col)
      : name(tname), columns(cols), pk_column(pk_col) {
    static_assert(N > 0, "Table must have at least one column");
  }
};

template <typename> struct pk_value_type;

template <typename C, typename T> struct pk_value_type<ColumnMeta<C, T, true>> {
  using type = T;
};

template <typename Col> constexpr auto extract_pk(Col &&col, std::true_type) {
  return std::tuple<std::decay_t<Col>>(std::forward<Col>(col));
}

template <typename Col> constexpr auto extract_pk(Col &&, std::false_type) {
  return std::tuple<>();
}

template <typename Class, typename Type, std::size_t N>
consteval auto make_column(Type Class::*m, const char (&n)[N],
                           uint32_t offset) {
  static_assert(N > 1, "Column name not defined");
  return ColumnMeta<Class, Type, false>(m, n, offset);
}

template <typename Class, std::size_t N, typename... Cols>
consteval auto make_table(const char (&n)[N], Cols... cols) {
  static_assert(N > 1, "Table name not defined");
  constexpr std::size_t M = sizeof...(Cols);

  auto pk_tuple = std::tuple_cat(extract_pk(
      cols, std::bool_constant<std::decay_t<Cols>::is_primary_key_v>{})...);

  static_assert(std::tuple_size_v<decltype(pk_tuple)> == 1,
                "Table must have exactly one primary key");

  using PkCol = decltype(std::get<0>(pk_tuple));
  using PkType = typename pk_value_type<std::remove_reference_t<PkCol>>::type;

  return TableSchema<Class, M, PkType>{
      n, std::array<ColumnInfo, M>{cols.info()...}, std::get<0>(pk_tuple)};
}
} // namespace sqlinq

#endif // SQLINQ_TABLE_HPP_
