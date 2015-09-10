#include "support.h"

#suite Supporting routines

#test CLZ
    ck_assert_uint_eq(count_leading_zeroes(0u), sizeof(unsigned) * CHAR_BIT);
    ck_assert_uint_eq(count_leading_zeroes(1u), sizeof(unsigned) * CHAR_BIT - 1);
    ck_assert_uint_eq(count_leading_zeroes(0x12340u), sizeof(unsigned) * CHAR_BIT - 17);
    ck_assert_uint_eq(count_leading_zeroes(UINT_MAX), 0);
    ck_assert_uint_eq(count_leading_zeroes(UINT_MAX >> 1), 1);
    ck_assert_uint_eq(count_leading_zeroes(UINT_MAX >> 2), 2);

