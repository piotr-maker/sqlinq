#ifndef SQLINQ_UTILITY_HPP_
#define SQLINQ_UTILITY_HPP_

#include <string_view>

#include "detail/utility.hpp"

namespace sqlinq {

template <typename T>
constexpr auto template_type_name() -> std::string_view {
  constexpr auto& value = detail::template_type_name_holder<T>::value;
  return std::string_view{value.data(), value.size()};
}

} // namespace sqlinq

#endif /* SQLINQ_UTILITY_HPP_ */
