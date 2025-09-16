#ifndef SQLINQ_COLUMN_
#define SQLINQ_COLUMN_

#include <cstddef>
#include <type_traits>

#include "detail/type_traits.hpp"
#include "query_ast.hpp"

#define SQLINQ_COLUMN(INDEX, CLASS, MEMBER)                                    \
  sqlinq::Column<INDEX, CLASS, decltype(CLASS::MEMBER)> MEMBER;

namespace sqlinq {
template <std::size_t Idx, typename Class, typename T> class Column {
public:
  enum ValueState : char { Uninitialized, Set, Null };
  using class_type = Class;
  using value_t = std::conditional_t<detail::is_optional_v<T>,
                                     detail::optional_value_t<T>, T>;

  constexpr Column() : state_(Uninitialized) {}

  constexpr bool is_set() const noexcept { return state_ != Uninitialized; }
  constexpr bool is_null() const noexcept { return state_ == Null; }
  constexpr auto value() const noexcept { return value_; }

  template <typename U = T, typename = std::enable_if<detail::is_optional_v<U>>>
  void operator=(std::nullopt_t) noexcept {
    state_ = Null;
  }

  void operator=(value_t &&v) noexcept {
    value_ = std::move(v);
    state_ = Set;
  }

  template <typename U = T, typename = std::enable_if<detail::is_optional_v<U>>>
  FilterChain operator==(std::nullopt_t) const noexcept {
    return {FilterExpr::Kind::Leaf,
            ValueCondition{ValueCondition::Operator::Equal, BoundValue{}, Idx}};
  }

  template <typename U = T, typename = std::enable_if<detail::is_optional_v<U>>>
  FilterChain operator!=(std::nullopt_t) const noexcept {
    return {
        FilterExpr::Kind::Leaf,
        ValueCondition{ValueCondition::Operator::NotEqual, BoundValue{}, Idx}};
  }

  FilterChain operator==(value_t &&v) const noexcept {
    return {FilterExpr::Kind::Leaf,
            ValueCondition{ValueCondition::Operator::Equal, std::move(v), Idx}};
  }

  FilterChain operator!=(value_t &&v) const noexcept {
    return {
        FilterExpr::Kind::Leaf,
        ValueCondition{ValueCondition::Operator::NotEqual, std::move(v), Idx}};
  }

  FilterChain operator<(value_t &&v) const noexcept {
    return {FilterExpr::Kind::Leaf,
            ValueCondition{ValueCondition::Operator::Less, std::move(v), Idx}};
  }

  FilterChain operator<=(value_t &&v) const noexcept {
    return {
        FilterExpr::Kind::Leaf,
        ValueCondition{ValueCondition::Operator::LessEqual, std::move(v), Idx}};
  }

  FilterChain operator>(value_t &&v) const noexcept {
    return {
        FilterExpr::Kind::Leaf,
        ValueCondition{ValueCondition::Operator::Greater, std::move(v), Idx}};
  }

  FilterChain operator>=(value_t &&v) const noexcept {
    return {FilterExpr::Kind::Leaf,
            ValueCondition{ValueCondition::Operator::GreaterEqual, std::move(v),
                           Idx}};
  }

private:
  ValueState state_;
  value_t value_;
};
} // namespace sqlinq

#endif // SQLINQ_COLUMN_
