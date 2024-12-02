#ifndef SQLINQ_DETAIL_STRUCTURE_HPP_
#define SQLINQ_DETAIL_STRUCTURE_HPP_

#include <cstddef>
#include <tuple>
#include "type_traits.hpp"

constexpr std::size_t max_struct_size = 32;

#define PARAM_NAME(X) p##X

#define REPEAT_1(FN) FN(1)
#define REPEAT_2(FN) FN(2), REPEAT_1(FN)
#define REPEAT_3(FN) FN(3), REPEAT_2(FN)
#define REPEAT_4(FN) FN(4), REPEAT_3(FN)
#define REPEAT_5(FN) FN(5), REPEAT_4(FN)
#define REPEAT_6(FN) FN(6), REPEAT_5(FN)
#define REPEAT_7(FN) FN(7), REPEAT_6(FN)
#define REPEAT_8(FN) FN(8), REPEAT_7(FN)
#define REPEAT_9(FN) FN(9), REPEAT_8(FN)
#define REPEAT_10(FN) FN(10), REPEAT_9(FN)
#define REPEAT_11(FN) FN(11), REPEAT_10(FN)
#define REPEAT_12(FN) FN(12), REPEAT_11(FN)
#define REPEAT_13(FN) FN(13), REPEAT_12(FN)
#define REPEAT_14(FN) FN(14), REPEAT_13(FN)
#define REPEAT_15(FN) FN(15), REPEAT_14(FN)
#define REPEAT_16(FN) FN(16), REPEAT_15(FN)
#define REPEAT_17(FN) FN(17), REPEAT_16(FN)
#define REPEAT_18(FN) FN(18), REPEAT_17(FN)
#define REPEAT_19(FN) FN(19), REPEAT_18(FN)
#define REPEAT_20(FN) FN(20), REPEAT_19(FN)
#define REPEAT_21(FN) FN(21), REPEAT_20(FN)
#define REPEAT_22(FN) FN(22), REPEAT_21(FN)
#define REPEAT_23(FN) FN(23), REPEAT_22(FN)
#define REPEAT_24(FN) FN(24), REPEAT_23(FN)
#define REPEAT_25(FN) FN(25), REPEAT_24(FN)
#define REPEAT_26(FN) FN(26), REPEAT_25(FN)
#define REPEAT_27(FN) FN(27), REPEAT_26(FN)
#define REPEAT_28(FN) FN(28), REPEAT_27(FN)
#define REPEAT_29(FN) FN(29), REPEAT_28(FN)
#define REPEAT_30(FN) FN(30), REPEAT_29(FN)
#define REPEAT_31(FN) FN(31), REPEAT_30(FN)
#define REPEAT_32(FN) FN(32), REPEAT_31(FN)

#define REPEAT(FN, N) REPEAT_##N(FN)

#define UNPACK(S, N)                                                   \
    do {                                                               \
        auto&& [REPEAT(PARAM_NAME, N)] = std::forward<decltype(S)>(S); \
        return std::make_tuple(REPEAT(PARAM_NAME, N));                 \
    } while (0);

namespace sqlinq::detail {

    template <class S, std::size_t N>
    constexpr auto structure_size_impl() -> std::size_t
    {
        static_assert(N <= max_struct_size, "Library is limited to 32 struct members");
        return N;
    }

    template <class S, std::size_t N>
        requires(!is_aggregate_constructible_n_v<S, N>)
    constexpr auto structure_size_impl() -> std::size_t
    {
        return structure_size_impl<S, N - 1>();
    }

    template <class C, std::size_t N>
    constexpr auto structure_to_tuple_impl(C&& c)
    {
        if constexpr (N == 32) { UNPACK(c, 32); }
        else if constexpr (N == 31) { UNPACK(c, 31); }
        else if constexpr (N == 30) { UNPACK(c, 30); }
        else if constexpr (N == 29) { UNPACK(c, 29); }
        else if constexpr (N == 28) { UNPACK(c, 28); }
        else if constexpr (N == 27) { UNPACK(c, 27); }
        else if constexpr (N == 26) { UNPACK(c, 26); }
        else if constexpr (N == 25) { UNPACK(c, 25); }
        else if constexpr (N == 24) { UNPACK(c, 24); }
        else if constexpr (N == 23) { UNPACK(c, 23); }
        else if constexpr (N == 22) { UNPACK(c, 22); }
        else if constexpr (N == 21) { UNPACK(c, 21); }
        else if constexpr (N == 20) { UNPACK(c, 20); }
        else if constexpr (N == 19) { UNPACK(c, 19); }
        else if constexpr (N == 18) { UNPACK(c, 18); }
        else if constexpr (N == 17) { UNPACK(c, 17); }
        else if constexpr (N == 16) { UNPACK(c, 16); }
        else if constexpr (N == 15) { UNPACK(c, 15); }
        else if constexpr (N == 14) { UNPACK(c, 14); }
        else if constexpr (N == 13) { UNPACK(c, 13); }
        else if constexpr (N == 12) { UNPACK(c, 12); }
        else if constexpr (N == 11) { UNPACK(c, 11); }
        else if constexpr (N == 10) { UNPACK(c, 10); }
        else if constexpr (N == 9) { UNPACK(c, 9); }
        else if constexpr (N == 8) { UNPACK(c, 8); }
        else if constexpr (N == 7) { UNPACK(c, 7); }
        else if constexpr (N == 6) { UNPACK(c, 6); }
        else if constexpr (N == 5) { UNPACK(c, 5); }
        else if constexpr (N == 4) { UNPACK(c, 4); }
        else if constexpr (N == 3) { UNPACK(c, 3); }
        else if constexpr (N == 2) { UNPACK(c, 2); }
        else if constexpr (N == 1) { UNPACK(c, 1); }
        else return std::make_tuple();
    }

} // namespace sqlinq::detail

#undef PARAM_NAME

#undef REPEAT_1
#undef REPEAT_2
#undef REPEAT_3
#undef REPEAT_4
#undef REPEAT_5
#undef REPEAT_6
#undef REPEAT_7
#undef REPEAT_8
#undef REPEAT_9
#undef REPEAT_10
#undef REPEAT_11
#undef REPEAT_12
#undef REPEAT_13
#undef REPEAT_14
#undef REPEAT_15
#undef REPEAT_16
#undef REPEAT_17
#undef REPEAT_18
#undef REPEAT_19
#undef REPEAT_20
#undef REPEAT_21
#undef REPEAT_22
#undef REPEAT_23
#undef REPEAT_24
#undef REPEAT_25
#undef REPEAT_26
#undef REPEAT_27
#undef REPEAT_28
#undef REPEAT_29
#undef REPEAT_30
#undef REPEAT_31
#undef REPEAT_32

#undef REPEAT

#undef UNPACK

#endif /* SQLINQ_DETAIL_STRUCTURE_HPP_ */
