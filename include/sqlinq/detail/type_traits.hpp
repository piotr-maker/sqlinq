#ifndef SQLINQ_DETAIL_TYPE_TRAITS_HPP_
#define SQLINQ_DETAIL_TYPE_TRAITS_HPP_

#include <array>
#include <utility>
#include <type_traits>
#include <string_view>

namespace sqlinq::detail {

struct any_type {
  template <typename T> operator T() const;
};

template <typename T, typename Is, typename = std::void_t<>>
struct is_braces_constructible_n_impl : std::false_type {};

template <typename T, std::size_t... Is>
struct is_braces_constructible_n_impl<
    T, std::index_sequence<Is...>,
    std::void_t<decltype(T{(void(Is), any_type{})...})>> : std::true_type {};

template <typename T, std::size_t N>
  requires std::is_aggregate_v<T>
struct is_aggregate_constructible_n
    : detail::is_braces_constructible_n_impl<T, std::make_index_sequence<N>> {};

template <typename T, std::size_t N>
  requires std::is_aggregate_v<T>
inline constexpr bool is_aggregate_constructible_n_v =
    is_aggregate_constructible_n<T, N>::value;

constexpr char toLower(const char c) {
  return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

template <std::size_t... Is>
constexpr auto substring_as_array_impl(std::string_view substr,
                                  std::index_sequence<Is...>) {
  return std::array{toLower(substr[Is])...};
}

template <class T, std::size_t N>
concept has_tuple_element = requires(T t) {
  typename std::tuple_element_t<N, std::remove_const_t<T>>;
  { get<N>(t) } -> std::convertible_to<const std::tuple_element<N, T>&>;
};
} // namespace sqlinq::detail

#endif /* SQLINQ_DETAIL_TYPE_TRAITS_HPP_ */
