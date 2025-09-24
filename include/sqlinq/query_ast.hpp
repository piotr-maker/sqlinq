#ifndef SQLINQ_QUERY_AST_HPP_
#define SQLINQ_QUERY_AST_HPP_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "table.hpp"
#include "types/datetime.hpp"

namespace sqlinq {
struct AggregateExpr {
  enum class Function { None, Avg, Count, Min, Max, Sum };

  AggregateExpr() : fn_(Function::None), distinct_(false), col_name_() {}

  AggregateExpr(Function fn, std::string_view col_name)
      : fn_(fn), distinct_(false), col_name_(col_name) {}

  Function fn() const noexcept { return fn_; }
  bool is_distinct() const noexcept { return distinct_; }
  std::string_view column_name() const noexcept { return col_name_; }

protected:
  Function fn_;
  bool distinct_;
  std::string_view col_name_;
};

template <typename T> class AggregateResult : public AggregateExpr {
public:
  using value_type = T;

  AggregateResult(Function fn, std::string_view col_name)
      : AggregateExpr(fn, col_name) {}

  AggregateResult distinct() && {
    distinct_ = true;
    return std::move(*this);
  }

private:
  using AggregateExpr ::column_name;
  using AggregateExpr ::fn;
  using AggregateExpr ::is_distinct;
};

class BoundValue {
public:
  constexpr BoundValue() : type_(column::Type::Null), ptr_(nullptr), size_(0) {}

  constexpr BoundValue(const void *ptr, std::size_t size, column::Type type)
      : type_(type), ptr_(ptr), size_(size) {}

  constexpr BoundValue(int8_t v)
      : type_(column::Type::TinyInt), tiny_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(int16_t v)
      : type_(column::Type::SmallInt), short_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(int32_t v)
      : type_(column::Type::Int), long_(v), ptr_(nullptr), size_(0) {
  }

  constexpr BoundValue(int64_t v)
      : type_(column::Type::BigInt), longlong_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(float v)
      : type_(column::Type::Float), float_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(double v)
      : type_(column::Type::Double), double_(v), ptr_(nullptr),
        size_(0) {}

  template <std::size_t P, std::size_t S>
  constexpr BoundValue(Decimal<P, S> v)
      : type_(column::Type::Decimal), longlong_(static_cast<int64_t>(v)),
        ptr_(nullptr), size_(S) {}

  constexpr BoundValue(Date v)
      : type_(column::Type::Date), date_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(Time v)
      : type_(column::Type::Time), time_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(Datetime v)
      : type_(column::Type::Datetime), datetime_(v), ptr_(nullptr),
        size_(0) {}

  constexpr BoundValue(Timestamp v)
      : type_(column::Type::Timestamp), timestamp_(v), ptr_(nullptr),
        size_(0) {}

  BoundValue(const std::string &str) noexcept
      : type_(column::Type::Text), ptr_(nullptr), size_(str.size()) {
    owned_data_ = std::make_unique<char[]>(size_);
    std::memcpy(owned_data_.get(), str.data(), str.size());
  }

  BoundValue(BoundValue &&) noexcept = default;
  BoundValue &operator=(BoundValue &&other) noexcept = default;

  BoundValue(const BoundValue &) = delete;
  BoundValue &operator=(const BoundValue &) = delete;

  constexpr bool has_value() const noexcept {
    return type_ != column::Type::Null;
  }
  constexpr column::Type type() const noexcept { return type_; }
  constexpr bool is_null() const noexcept {
    return type_ == column::Type::Null;
  }
  constexpr std::size_t size() const noexcept { return size_; }
  constexpr const void *ptr() const noexcept {
    if (ptr_ != nullptr) {
      return ptr_;
    }
    if (owned_data_) {
      return owned_data_.get();
    }
    switch (type_) {
    case column::Type::Bit:
      return &tiny_;
    case column::Type::TinyInt:
      return &tiny_;
    case column::Type::SmallInt:
      return &short_;
    case column::Type::Int:
      return &long_;
    case column::Type::BigInt:
      return &longlong_;
    case column::Type::Float:
      return &float_;
    case column::Type::Double:
      return &double_;
    case column::Type::Decimal:
      return &longlong_;
    case column::Type::Date:
      return &date_;
    case column::Type::Time:
      return &time_;
    case column::Type::Datetime:
      return &datetime_;
    case column::Type::Timestamp:
      return &timestamp_;
    default:
      break;
    }
    return nullptr;
  }

private:
  column::Type type_;
  union {
    int8_t tiny_;
    int16_t short_;
    int32_t long_;
    int64_t longlong_;
    float float_;
    double double_;
    Date date_;
    Time time_;
    Datetime datetime_;
    Timestamp timestamp_;
  };

  std::unique_ptr<char[]> owned_data_;
  const void *ptr_;
  std::size_t size_;
};

struct ValueCondition {
  enum class Operator : int {
    Less,
    LessEqual,
    Equal,
    NotEqual,
    GreaterEqual,
    Greater
  };

  Operator value_op;
  BoundValue value;
  const char *column_name;
  const std::size_t index;

  explicit ValueCondition()
      : value_op(Operator::Less), value(), column_name(nullptr), index(0) {}

  explicit ValueCondition(Operator op, BoundValue &&v, std::size_t idx)
      : value_op(op), value(std::move(v)), column_name(nullptr), index(idx) {}
};

struct FilterExpr {
  enum class Kind { None, Leaf, And, Not, Or };
  Kind kind;
  ValueCondition condition;

  FilterExpr(Kind k, ValueCondition &&cond)
      : kind(k), condition(std::move(cond)) {}
};

class FilterChain {
public:
  using iterator = std::vector<FilterExpr>::iterator;
  using const_iterator = std::vector<FilterExpr>::const_iterator;

  FilterChain() noexcept = default;
  FilterChain(FilterExpr::Kind kind, ValueCondition &&cond) {
    exprs_.emplace_back(FilterExpr{kind, std::move(cond)});
  }

  iterator begin() noexcept { return exprs_.begin(); }
  const_iterator begin() const noexcept { return exprs_.begin(); }
  iterator end() noexcept { return exprs_.end(); }
  const_iterator end() const noexcept { return exprs_.end(); }

  void clear() noexcept { exprs_.clear(); }
  FilterExpr *data() { return exprs_.data(); }
  const FilterExpr *data() const noexcept { return exprs_.data(); }
  constexpr bool empty() const { return exprs_.empty(); }
  FilterExpr &front() { return exprs_.front(); }
  const FilterExpr &front() const { return exprs_.front(); }
  std::size_t size() const { return exprs_.size(); }

  auto extract_values() {
    std::vector<BoundValue> result{};
    for (FilterExpr &e : exprs_) {
      if (e.condition.value.has_value()) {
        result.emplace_back(std::move(e.condition.value));
      }
    }
    return result;
  }

  friend FilterChain operator&&(FilterChain &&lhs, FilterChain &&rhs) {
    lhs.exprs_.push_back(FilterExpr{FilterExpr::Kind::And, ValueCondition()});
    lhs.exprs_.emplace_back(std::move(rhs.front()));
    return std::move(lhs);
  }

  friend FilterChain operator||(FilterChain &&lhs, FilterChain &&rhs) {
    lhs.exprs_.push_back(FilterExpr{FilterExpr::Kind::Or, ValueCondition()});
    lhs.exprs_.emplace_back(std::move(rhs.front()));
    return std::move(lhs);
  }

private:
  std::vector<FilterExpr> exprs_;
};

template <typename T> class ColumnDef {
public:
  constexpr ColumnDef(const char *name) noexcept : name_(name) {}
  constexpr const char *name() const noexcept { return name_; }

private:
  const char *name_;
};

struct QueryAst {
  enum class Operation { None, Delete, Insert, Select, Update };
  Operation op;
  std::string table_name;
  FilterChain filter_chain;
  AggregateExpr aggr_expr;
  std::vector<std::string_view> group_expr;
  std::vector<std::string_view> order_expr;
  std::vector<std::string_view> column_names;
  std::vector<BoundValue> values;

  std::optional<std::size_t> skip;  // OFFSET
  std::optional<std::size_t> fetch; // LIMIT
};
} // namespace sqlinq

#endif // SQLINQ_QUERY_AST_HPP_
