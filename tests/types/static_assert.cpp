#include <sqlinq/types/decimal.hpp>

constexpr int vi = 123;
constexpr sqlinq::Decimal<6, 2> d{vi};
static_assert(12300 == static_cast<int64_t>(d));

static_assert(static_cast<int64_t>(sqlinq::Decimal<6, 2>::max()) == 999999);
static_assert(static_cast<int64_t>(sqlinq::Decimal<6, 2>::min()) == -999999);
