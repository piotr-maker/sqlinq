#ifndef SQLINQ_COLUMN_HPP_
#define SQLINQ_COLUMN_HPP_

#include <array>
#include <cstddef>
#include <utility>
#include <string_view>

namespace sqlinq {
namespace detail {

template <typename T, typename U>
constexpr std::size_t class_member_offset(U T::*member) {
  return reinterpret_cast<std::size_t>(&((T*)nullptr->*member));
}

template <typename T, typename... Args, std::size_t... Is>
constexpr auto class_member_offset_helper(std::index_sequence<Is...>, Args... members) {
  return std::array<std::size_t, sizeof...(Args)>{class_member_offset(members)...};
}

template <typename T, typename... Args>
constexpr auto class_member_offsets(Args... members) {
  return class_member_offset_helper<T>(std::index_sequence_for<Args...>{}, members...);
}

} // namespace detail

template <typename T, typename... Args>
constexpr auto class_member_offsets(Args... members) {
  return detail::class_member_offsets<T>(members...);
};

template <typename T>
struct class_member_offset_holder {
  /*static_assert(false, "member offset holder not defined, define it with macro SQLINQ_COLUMNS first");*/
  constexpr static std::array<std::size_t, 0> value{};
};

template <typename T>
struct class_member_name_holder {
  /*static_assert(false, "member name holder not defined, define it with macro SQLINQ_COLUMNS first");*/
  constexpr static std::array<std::string_view, 0> value{};
};

#define SQLINQ_COL_OFFSET_0(c, m) &c::m
#define SQLINQ_COL_OFFSET_1(c, m, ...) &c::m, SQLINQ_COL_OFFSET_0(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_2(c, m, ...) &c::m, SQLINQ_COL_OFFSET_1(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_3(c, m, ...) &c::m, SQLINQ_COL_OFFSET_2(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_4(c, m, ...) &c::m, SQLINQ_COL_OFFSET_3(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_5(c, m, ...) &c::m, SQLINQ_COL_OFFSET_4(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_6(c, m, ...) &c::m, SQLINQ_COL_OFFSET_5(c, __VA_ARGS__)
#define SQLINQ_COL_OFFSET_7(c, m, ...) &c::m, SQLINQ_COL_OFFSET_6(c, __VA_ARGS__)

#define SQLINQ_COL_OFFSET_N(cname, col, ...) SQLINQ_COL_OFFSET_N_HELPER \
  (, ##__VA_ARGS__, \
  SQLINQ_COL_OFFSET_7, \
  SQLINQ_COL_OFFSET_6, \
  SQLINQ_COL_OFFSET_5, \
  SQLINQ_COL_OFFSET_4, \
  SQLINQ_COL_OFFSET_3, \
  SQLINQ_COL_OFFSET_2, \
  SQLINQ_COL_OFFSET_1  \
)
#define SQLINQ_COL_OFFSET_N_HELPER(z, a, b, c, d, e, f, count, ...) count

#define SQLINQ_COLUMNS(cname, ...) \
template <> struct sqlinq::class_member_offset_holder<cname> { \
  constexpr static auto value = sqlinq::class_member_offsets<cname> \
    (SQLINQ_COL_OFFSET_N(__VA_ARGS__)(cname, __VA_ARGS__)); \
}; \
template<> struct sqlinq::class_member_name_holder<cname> { \
  constexpr static auto value = std::array{"id", "first_name", "age"}; \
};

} // namespace sqlinq

#endif /* SQLINQ_COLUMN_HPP_ */
