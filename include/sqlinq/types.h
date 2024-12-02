#ifndef SQLINQ_TYPES_H_
#define SQLINQ_TYPES_H_

#include <array>
#include <sqlinq/types/decimal.hpp>
#include <sqlinq/types/varchar.hpp>

template <std::size_t N>
class varchar : std::array<char, N> {};


#endif /* SQLINQ_TYPES_H_ */
