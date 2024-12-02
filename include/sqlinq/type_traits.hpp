#ifndef UTILITY_TYPE_TRAITS_HPP_
#define UTILITY_TYPE_TRAITS_HPP_

#include <type_traits>
#include <string_view>
#include "detail/structure.hpp"
#include "detail/type_traits.hpp"

namespace sqlinq {
template <typename, typename = std::void_t<>> struct is_container {
  static inline constexpr bool value = false;
};

template <typename T>
struct is_container<T, std::void_t<decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end())>> {
  static inline constexpr bool value = true;
};

template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;

template <typename, typename = std::void_t<>> struct is_tuple {
  static inline constexpr bool value = false;
};

template <typename... Args>
struct is_tuple<std::tuple<Args...>> {
  static inline constexpr bool value = true;
};

template <typename T>
inline constexpr bool is_tuple_v = is_tuple<T>::value;

template <class T>
concept string_like = std::is_convertible_v<T, std::string_view>;

template <std::size_t... Is>
constexpr auto substring_as_array(std::string_view substr,
                                  std::index_sequence<Is...> idx) {
  return detail::substring_as_array_impl(substr, idx);
}

// tuple
template <class T>
concept tuple_like =// !std::is_reference_v<T> &&
  requires(T t) {
    typename std::tuple_size<std::remove_reference_t<T>>::type;
    requires std::derived_from<
      std::tuple_size<std::remove_reference_t<T>>,
      std::integral_constant<std::size_t, std::tuple_size_v<T>>
  >;
} && []<std::size_t... N>(std::index_sequence<N...>) {
  return (detail::has_tuple_element<T, N> && ...);
}(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>());

// struct
template <class S> struct structure_size {
  inline static constexpr auto value =
      detail::structure_size_impl<S, max_struct_size + 1>();
};

template <typename C>
concept AggregateClass =
    std::is_aggregate_v<std::decay_t<C>> && std::is_class_v<std::decay_t<C>>;

template <AggregateClass C, std::size_t... Is, class Tuple>
C to_struct(std::index_sequence<Is...>, Tuple&& tup) {
  return {std::get<Is>(std::forward<Tuple>(tup))...};
}

template <AggregateClass C, class Tuple>
C to_struct(Tuple&& tup) {
  using T = std::remove_reference_t<Tuple>;
  return to_struct<C>(
    std::make_index_sequence<std::tuple_size_v<T>>{},
    std::forward<Tuple>(tup)
  );
}

template <AggregateClass C> constexpr auto structure_to_tuple(C &&c) {
  constexpr std::size_t struct_size = structure_size<std::decay_t<C>>::value;
  static_assert(struct_size != 0, "Struct does not have any members");
  return detail::structure_to_tuple_impl<C, struct_size>(std::forward<C>(c));
}
} // namespace utility

#endif /* UTILITY_TYPE_TRAITS_HPP_ */
