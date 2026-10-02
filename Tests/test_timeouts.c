#include "timeouts.h"
#include "unity.h"

#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_zero_milliseconds_is_zero_ticks(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, Timeout_MsToTicks(0U, 100U));
}

void test_a_non_zero_duration_never_becomes_zero_ticks(void)
{
    TEST_ASSERT_EQUAL_UINT32(1U, Timeout_MsToTicks(1U, 100U));
    TEST_ASSERT_EQUAL_UINT32(1U, Timeout_MsToTicks(1U, 1U));
}

void test_exact_multiples_are_not_rounded_up(void)
{
    TEST_ASSERT_EQUAL_UINT32(1U, Timeout_MsToTicks(10U, 100U));
    TEST_ASSERT_EQUAL_UINT32(10U, Timeout_MsToTicks(100U, 100U));
    TEST_ASSERT_EQUAL_UINT32(100U, Timeout_MsToTicks(1000U, 100U));
}

void test_partial_ticks_round_up(void)
{
    TEST_ASSERT_EQUAL_UINT32(2U, Timeout_MsToTicks(11U, 100U));
    TEST_ASSERT_EQUAL_UINT32(2U, Timeout_MsToTicks(19U, 100U));
    TEST_ASSERT_EQUAL_UINT32(2U, Timeout_MsToTicks(20U, 100U));
    TEST_ASSERT_EQUAL_UINT32(3U, Timeout_MsToTicks(21U, 100U));
}

void test_a_one_kilohertz_tick_is_one_tick_per_millisecond(void)
{
    TEST_ASSERT_EQUAL_UINT32(250U, Timeout_MsToTicks(250U, 1000U));
}

void test_a_zero_tick_rate_gives_zero_ticks(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, Timeout_MsToTicks(100U, 0U));
}

void test_the_result_saturates_instead_of_wrapping(void)
{
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, Timeout_MsToTicks(UINT32_MAX, UINT32_MAX));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, Timeout_MsToTicks(UINT32_MAX, 1000U));
    TEST_ASSERT_EQUAL_UINT32(429496730U, Timeout_MsToTicks(UINT32_MAX, 100U));
}

/* The original formula from usb_logging.c and waveshare_driver.c (before its clamps). */
static uint64_t original_ticks(uint32_t ms, uint32_t tick_hz)
{
    return (((uint64_t)ms * (uint64_t)tick_hz) + 999ULL) / 1000ULL;
}

void test_matches_the_original_formula(void)
{
    uint32_t state = 777U;

    for (unsigned int i = 0U; i < 20000U; i++)
    {
        uint32_t ms;
        uint32_t hz;
        uint64_t expected;

        state = (state * 1664525U) + 1013904223U;
        ms = state;
        state = (state * 1664525U) + 1013904223U;
        hz = state >> ((state >> 28) & 0x1FU);
        expected = original_ticks(ms, hz);
        if (expected > (uint64_t)UINT32_MAX)
        {
            expected = (uint64_t)UINT32_MAX;
        }
        TEST_ASSERT_EQUAL_UINT32((uint32_t)expected, Timeout_MsToTicks(ms, hz));
    }
}

void test_elapsed_is_false_before_the_limit_and_true_at_it(void)
{
    TEST_ASSERT_FALSE(Timeout_Elapsed(1000U, 1999U, 1000U));
    TEST_ASSERT_TRUE(Timeout_Elapsed(1000U, 2000U, 1000U));
    TEST_ASSERT_TRUE(Timeout_Elapsed(1000U, 2001U, 1000U));
}

void test_a_zero_limit_has_always_elapsed(void)
{
    TEST_ASSERT_TRUE(Timeout_Elapsed(5U, 5U, 0U));
}

void test_elapsed_survives_the_tick_counter_wrapping(void)
{
    TEST_ASSERT_FALSE(Timeout_Elapsed(UINT32_MAX - 99U, 899U, 1000U));
    TEST_ASSERT_TRUE(Timeout_Elapsed(UINT32_MAX - 99U, 900U, 1000U));
    TEST_ASSERT_TRUE(Timeout_Elapsed(UINT32_MAX, 0U, 1U));
    TEST_ASSERT_FALSE(Timeout_Elapsed(UINT32_MAX, UINT32_MAX, 1U));
}

void test_elapsed_matches_the_original_comparison(void)
{
    uint32_t state = 99U;

    for (unsigned int i = 0U; i < 20000U; i++)
    {
        uint32_t start;
        uint32_t now;
        uint32_t limit;

        state = (state * 1664525U) + 1013904223U;
        start = state;
        state = (state * 1664525U) + 1013904223U;
        now = ((state & 1U) != 0U) ? (start + (state >> 12)) : state;
        state = (state * 1664525U) + 1013904223U;
        limit = state >> ((state >> 27) & 0x1FU);
        TEST_ASSERT_EQUAL((uint32_t)(now - start) >= limit, Timeout_Elapsed(start, now, limit));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_zero_milliseconds_is_zero_ticks);
    RUN_TEST(test_a_non_zero_duration_never_becomes_zero_ticks);
    RUN_TEST(test_exact_multiples_are_not_rounded_up);
    RUN_TEST(test_partial_ticks_round_up);
    RUN_TEST(test_a_one_kilohertz_tick_is_one_tick_per_millisecond);
    RUN_TEST(test_a_zero_tick_rate_gives_zero_ticks);
    RUN_TEST(test_the_result_saturates_instead_of_wrapping);
    RUN_TEST(test_matches_the_original_formula);
    RUN_TEST(test_elapsed_is_false_before_the_limit_and_true_at_it);
    RUN_TEST(test_a_zero_limit_has_always_elapsed);
    RUN_TEST(test_elapsed_survives_the_tick_counter_wrapping);
    RUN_TEST(test_elapsed_matches_the_original_comparison);
    return UNITY_END();
}
