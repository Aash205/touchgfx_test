#include "debounce.h"
#include "unity.h"

#include <stdbool.h>
#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_a_press_is_reported_on_the_third_pressed_poll_with_two_polls(void)
{
    Debounce_t d = {0U, 0U};

    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 2U));
}

void test_a_held_button_reports_only_once(void)
{
    Debounce_t d = {0U, 0U};

    (void)Debounce_Poll(&d, true, 2U);
    (void)Debounce_Poll(&d, true, 2U);
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 2U));
    for (unsigned int i = 0U; i < 50U; i++)
    {
        TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    }
}

void test_release_and_press_again_reports_again(void)
{
    Debounce_t d = {0U, 0U};

    (void)Debounce_Poll(&d, true, 2U);
    (void)Debounce_Poll(&d, true, 2U);
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, false, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, false, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, false, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 2U));
}

void test_a_bounce_restarts_the_count(void)
{
    Debounce_t d = {0U, 0U};

    (void)Debounce_Poll(&d, true, 2U);
    (void)Debounce_Poll(&d, true, 2U);
    TEST_ASSERT_FALSE(Debounce_Poll(&d, false, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 2U));
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 2U));
}

void test_a_released_button_never_reports(void)
{
    Debounce_t d = {0U, 0U};

    for (unsigned int i = 0U; i < 20U; i++)
    {
        TEST_ASSERT_FALSE(Debounce_Poll(&d, false, 2U));
    }
}

void test_zero_polls_and_a_null_state_never_report(void)
{
    Debounce_t d = {0U, 0U};

    for (unsigned int i = 0U; i < 5U; i++)
    {
        TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 0U));
    }
    TEST_ASSERT_FALSE(Debounce_Poll(NULL, true, 2U));
}

void test_one_poll_reports_on_the_second_pressed_poll(void)
{
    Debounce_t d = {0U, 0U};

    TEST_ASSERT_FALSE(Debounce_Poll(&d, true, 1U));
    TEST_ASSERT_TRUE(Debounce_Poll(&d, true, 1U));
}

/* The original poll_button() from app_core.c, with its two statics as a struct. */
static bool original_poll(uint8_t* stable, uint8_t* last, uint8_t now, uint8_t limit)
{
    bool toggled = false;

    if (now == *last)
    {
        if (*stable < limit && ++*stable == limit && now)
        {
            toggled = true;
        }
    }
    else
    {
        *last = now;
        *stable = 0;
    }

    return toggled;
}

void test_matches_the_original_over_random_polls(void)
{
    for (uint8_t limit = 0U; limit <= 4U; limit++)
    {
        unsigned state = 77U + limit;
        uint8_t stable = 0U;
        uint8_t last = 0U;
        Debounce_t d = {0U, 0U};
        uint8_t level = 0U;

        for (unsigned step = 0U; step < 20000U; step++)
        {
            bool expected;
            bool actual;

            state = (state * 1664525U) + 1013904223U;
            /* mostly keep the level, sometimes flip it, so presses of every length occur */
            if (((state >> 8) % 5U) == 0U)
            {
                level = (uint8_t)(1U - level);
            }
            expected = original_poll(&stable, &last, level, limit);
            actual = Debounce_Poll(&d, level != 0U, limit);
            TEST_ASSERT_EQUAL(expected, actual);
            TEST_ASSERT_EQUAL_UINT8(stable, d.stable);
            TEST_ASSERT_EQUAL_UINT8(last, d.last);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_a_press_is_reported_on_the_third_pressed_poll_with_two_polls);
    RUN_TEST(test_a_held_button_reports_only_once);
    RUN_TEST(test_release_and_press_again_reports_again);
    RUN_TEST(test_a_bounce_restarts_the_count);
    RUN_TEST(test_a_released_button_never_reports);
    RUN_TEST(test_zero_polls_and_a_null_state_never_report);
    RUN_TEST(test_one_poll_reports_on_the_second_pressed_poll);
    RUN_TEST(test_matches_the_original_over_random_polls);
    return UNITY_END();
}
