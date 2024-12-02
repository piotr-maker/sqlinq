#ifndef SQLINQ_DETAIL_UTILITY_HPP_
#define SQLINQ_DETAIL_UTILITY_HPP_

#include <string_view>
#include "type_traits.hpp"

namespace sqlinq::detail {

template <typename T>
constexpr auto template_type_name_array() {
#if defined(__clang__)
  constexpr auto prefix = std::string_view{"[T = "};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto fname = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
  constexpr auto prefix = std::string_view{"with T = "};
  constexpr auto suffix = std::string_view{"]"};
  constexpr auto fname = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
  constexpr auto prefix = std::string_view{"type_name_array<"};
  constexpr auto suffix = std::string_view{">(void)"};
  constexpr auto fname = std::string_view{__FUNSIG__};
#else
  #error Unsupported compiler
#endif

  constexpr auto begin = fname.find(prefix) + prefix.size();
  constexpr auto end = fname.find(suffix, begin);
  constexpr auto name = fname.substr(begin, end - begin);

  return substring_as_array_impl(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct template_type_name_holder {
  static inline constexpr auto value = template_type_name_array<T>();
};
} // namespace sqlinq::detail

#endif /* SQLINQ_DETAIL_UTILITY_HPP_ */
