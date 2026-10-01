#include "counters.h"
#include "unity.h"

#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_everything_starts_at_zero(void)
{
    Counters_t c = {0U, 0U, 0U};

    TEST_ASSERT_EQUAL_UINT16(0U, Counters_Fps(&c));
    TEST_ASSERT_EQUAL_UINT32(0U, Counters_HeartbeatValue(&c));
}

void test_the_tick_publishes_the_flush_count_and_restarts_it(void)
{
    Counters_t c = {0U, 0U, 0U};

    for (unsigned int i = 0U; i < 42U; i++)
    {
        Counters_FrameFlushed(&c);
    }
    TEST_ASSERT_EQUAL_UINT16(0U, Counters_Fps(&c));
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(42U, Counters_Fps(&c));
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(0U, Counters_Fps(&c));
}

void test_the_frame_rate_keeps_sixteen_bits(void)
{
    Counters_t c = {0U, 0U, 0U};

    c.frames = 300U;
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(300U, Counters_Fps(&c));
    c.frames = 65535U;
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(65535U, Counters_Fps(&c));
}

void test_the_frame_rate_wraps_in_sixteen_bits(void)
{
    Counters_t c = {0U, 0U, 0U};

    c.frames = 65536U + 5U;
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(5U, Counters_Fps(&c));
    TEST_ASSERT_EQUAL_UINT32(0U, c.frames);
}

void test_the_heartbeat_returns_the_new_value(void)
{
    Counters_t c = {0U, 0U, 0U};

    TEST_ASSERT_EQUAL_UINT32(1U, Counters_Heartbeat(&c));
    TEST_ASSERT_EQUAL_UINT32(2U, Counters_Heartbeat(&c));
    TEST_ASSERT_EQUAL_UINT32(2U, Counters_HeartbeatValue(&c));
}

void test_the_heartbeat_wraps_to_zero(void)
{
    Counters_t c = {0U, 0U, 0U};

    c.heartbeat = UINT32_MAX;
    TEST_ASSERT_EQUAL_UINT32(0U, Counters_Heartbeat(&c));
    TEST_ASSERT_EQUAL_UINT32(1U, Counters_Heartbeat(&c));
}

void test_the_counters_do_not_disturb_each_other(void)
{
    Counters_t c = {0U, 0U, 0U};

    Counters_FrameFlushed(&c);
    (void)Counters_Heartbeat(&c);
    Counters_Tick1s(&c);
    TEST_ASSERT_EQUAL_UINT16(1U, Counters_Fps(&c));
    TEST_ASSERT_EQUAL_UINT32(1U, Counters_HeartbeatValue(&c));
}

void test_a_null_pointer_is_ignored(void)
{
    Counters_FrameFlushed(NULL);
    Counters_Tick1s(NULL);
    TEST_ASSERT_EQUAL_UINT32(0U, Counters_Heartbeat(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, Counters_Fps(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, Counters_HeartbeatValue(NULL));
}

/* The original app_core.c statics and functions. */
static volatile uint32_t s_frames;
static volatile uint16_t s_fps;
static volatile uint32_t s_heartbeat;

static void original_frame_flushed(void)
{
    s_frames++;
}

static void original_tick1s(void)
{
    s_fps = (uint16_t)s_frames;
    s_frames = 0;
}

static uint32_t original_heartbeat(void)
{
    return ++s_heartbeat;
}

void test_matches_the_original_over_random_calls(void)
{
    Counters_t c = {0U, 0U, 0U};
    unsigned state = 9U;

    s_frames = 0U;
    s_fps = 0U;
    s_heartbeat = UINT32_MAX - 3000U;
    c.heartbeat = UINT32_MAX - 3000U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        state = (state * 1664525U) + 1013904223U;
        switch ((state >> 8) % 8U)
        {
        case 0U:
            original_tick1s();
            Counters_Tick1s(&c);
            break;
        case 1U:
            TEST_ASSERT_EQUAL_UINT32(original_heartbeat(), Counters_Heartbeat(&c));
            break;
        default:
            original_frame_flushed();
            Counters_FrameFlushed(&c);
            break;
        }
        TEST_ASSERT_EQUAL_UINT32(s_frames, c.frames);
        TEST_ASSERT_EQUAL_UINT16(s_fps, Counters_Fps(&c));
        TEST_ASSERT_EQUAL_UINT32(s_heartbeat, Counters_HeartbeatValue(&c));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_everything_starts_at_zero);
    RUN_TEST(test_the_tick_publishes_the_flush_count_and_restarts_it);
    RUN_TEST(test_the_frame_rate_keeps_sixteen_bits);
    RUN_TEST(test_the_frame_rate_wraps_in_sixteen_bits);
    RUN_TEST(test_the_heartbeat_returns_the_new_value);
    RUN_TEST(test_the_heartbeat_wraps_to_zero);
    RUN_TEST(test_the_counters_do_not_disturb_each_other);
    RUN_TEST(test_a_null_pointer_is_ignored);
    RUN_TEST(test_matches_the_original_over_random_calls);
    return UNITY_END();
}
