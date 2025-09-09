#ifndef SQLINQ_DETAIL_TYPE_TRAITS_HPP_
#define SQLINQ_DETAIL_TYPE_TRAITS_HPP_

#include <array>
#include <concepts>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sqlinq::detail {
template <typename T> struct is_optional : std::false_type {};

template <typename U> struct is_optional<std::optional<U>> : std::true_type {
  using value_type = U;
};

template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T>
using optional_value_t = typename is_optional<T>::value_type;

template <class, template <class...> class>
inline constexpr bool is_specialization_of_v = false;

template <class... Args, template <class...> class T>
inline constexpr bool is_specialization_of_v<T<Args...>, T> = true;

struct any_type {
  template <typename T, typename = std::enable_if_t<!is_specialization_of_v<
                            std::remove_cvref_t<T>, std::optional>>>
  operator T() const;
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
  { get<N>(t) } -> std::convertible_to<const std::tuple_element<N, T> &>;
};
} // namespace sqlinq::detail

#endif /* SQLINQ_DETAIL_TYPE_TRAITS_HPP_ */
