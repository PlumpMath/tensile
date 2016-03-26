#include "assertions.h"

TEST_SPEC(trivial, "Trivial", false, ASSERT(true));
TEST_SPEC(skip, "Skipped", true, ASSERT(false));

TEST_SPEC(iterate_bool, "Iterate boolean", false,
          TEST_ITERATE(x, bool,
                       {
                           ASSERT(x || !x);
                       }));

TEST_SPEC(iterate_char, "Iterate char", false,
          TEST_ITERATE(x, char,
                       {
                           ASSERT_GREATER(int, x, 0);
                       }));

TEST_SPEC(iterate_smallint, "Iterate small int", false,
          TEST_ITERATE(x, testval_small_uint_t,
                       {
                           ASSERT_LE(int, x, TESTVAL_SMALL_UINT_MAX);
                       }));

TEST_SPEC(iterate_uint64, "Iterate 64-but", false,
          TEST_ITERATE(x, uint64_t,
                       {
                           ASSERT(true);
                       }));

RUN_TESTSUITE("Self-test");
