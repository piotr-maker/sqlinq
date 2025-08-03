#ifndef SQLINQ_DETAIL_BINDING_HPP_
#define SQLINQ_DETAIL_BINDING_HPP_

#include <string_view>
#include "../type_traits.hpp"

namespace sqlinq::detail {

template <typename T, std::size_t N>
constexpr auto binding_str_array() {
  constexpr std::string_view str =
      "?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?";
  constexpr std::size_t length = N * 2 - 1;
  return substring_as_array(str.substr(0, length),
                            std::make_index_sequence<length>());
}

template <typename T, std::size_t N>
struct binding_str_holder {
  static inline constexpr auto value = binding_str_array<T, N>();
};
} // sqlinq::detail

#endif /* SQLINQ_DETAIL_BINDING_HPP_ */
