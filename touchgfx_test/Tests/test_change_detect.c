#include "change_detect.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- poll divider -------------------------------------------------------------------------- */

void test_the_poll_is_due_on_every_tenth_tick(void)
{
    ChangeDetect_t d = {0U, false};

    for (unsigned int round = 0U; round < 3U; round++)
    {
        for (unsigned int tick = 1U; tick < 10U; tick++)
        {
            TEST_ASSERT_FALSE(ChangeDetect_PollDue(&d, 10U));
        }
        TEST_ASSERT_TRUE(ChangeDetect_PollDue(&d, 10U));
    }
}

void test_a_period_of_zero_or_one_is_due_on_every_tick(void)
{
    ChangeDetect_t d = {0U, false};

    for (unsigned int i = 0U; i < 5U; i++)
    {
        TEST_ASSERT_TRUE(ChangeDetect_PollDue(&d, 1U));
    }
    for (unsigned int i = 0U; i < 5U; i++)
    {
        TEST_ASSERT_TRUE(ChangeDetect_PollDue(&d, 0U));
    }
}

void test_the_largest_period_is_due_on_the_255th_tick(void)
{
    ChangeDetect_t d = {0U, false};

    for (unsigned int tick = 1U; tick < 255U; tick++)
    {
        TEST_ASSERT_FALSE(ChangeDetect_PollDue(&d, 255U));
    }
    TEST_ASSERT_TRUE(ChangeDetect_PollDue(&d, 255U));
}

void test_a_null_detector_is_never_due(void)
{
    TEST_ASSERT_FALSE(ChangeDetect_PollDue(NULL, 10U));
}

/* ---- change detection ---------------------------------------------------------------------- */

typedef struct
{
    uint8_t a;
    uint32_t b;
    uint16_t c;
} Snapshot_t;

void test_the_first_snapshot_always_notifies_even_if_it_equals_the_zeroed_copy(void)
{
    ChangeDetect_t d = {0U, false};
    uint8_t last[8] = {0};
    const uint8_t now[8] = {0};

    TEST_ASSERT_TRUE(ChangeDetect_Update(&d, last, now, sizeof(now)));
    TEST_ASSERT_TRUE(d.have_last);
}

void test_an_unchanged_snapshot_does_not_notify(void)
{
    ChangeDetect_t d = {0U, false};
    Snapshot_t last;
    Snapshot_t now;

    memset(&last, 0, sizeof(last));
    memset(&now, 0, sizeof(now));
    now.b = 5U;
    TEST_ASSERT_TRUE(ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now)));
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now)));
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now)));
}

void test_a_changed_snapshot_notifies_and_is_stored(void)
{
    ChangeDetect_t d = {0U, false};
    Snapshot_t last;
    Snapshot_t now;

    memset(&last, 0, sizeof(last));
    memset(&now, 0, sizeof(now));
    (void)ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now));
    now.c = 9U;
    TEST_ASSERT_TRUE(ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now)));
    TEST_ASSERT_EQUAL_UINT16(9U, last.c);
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now)));
}

void test_a_change_in_the_last_byte_is_seen(void)
{
    ChangeDetect_t d = {0U, false};
    uint8_t last[16] = {0};
    uint8_t now[16] = {0};

    (void)ChangeDetect_Update(&d, last, now, sizeof(now));
    now[15] = 1U;
    TEST_ASSERT_TRUE(ChangeDetect_Update(&d, last, now, sizeof(now)));
}

void test_only_the_given_size_is_compared_and_stored(void)
{
    ChangeDetect_t d = {0U, false};
    uint8_t last[4] = {1U, 2U, 3U, 4U};
    uint8_t now[4] = {1U, 2U, 9U, 9U};

    (void)ChangeDetect_Update(&d, last, now, 2U);
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, last, now, 2U));
    TEST_ASSERT_EQUAL_UINT8(3U, last[2]);
}

void test_null_arguments_return_false_and_change_nothing(void)
{
    ChangeDetect_t d = {0U, false};
    uint8_t last[2] = {0U, 0U};
    const uint8_t now[2] = {1U, 1U};

    TEST_ASSERT_FALSE(ChangeDetect_Update(NULL, last, now, 2U));
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, NULL, now, 2U));
    TEST_ASSERT_FALSE(ChangeDetect_Update(&d, last, NULL, 2U));
    TEST_ASSERT_FALSE(d.have_last);
    TEST_ASSERT_EQUAL_UINT8(0U, last[0]);
}

/* ---- differential test against the original Model::tick ------------------------------------ */

typedef struct
{
    uint8_t led[2];
    uint8_t ble_status;
    uint32_t uptime_s;
    uint32_t heartbeat;
    uint16_t fps;
} AppState_t;

/* The original Model members and Model::tick logic, with the listener call recorded. */
typedef struct
{
    uint8_t tickCount;
    bool haveLast;
    AppState_t last;
} OriginalModel_t;

static bool original_tick(OriginalModel_t* m, const AppState_t* now)
{
    bool notified = false;

    if (++m->tickCount < 10U)
    {
        return false;
    }
    m->tickCount = 0;

    if (!m->haveLast || memcmp(now, &m->last, sizeof(*now)) != 0)
    {
        m->last = *now;
        m->haveLast = true;
        notified = true;
    }

    return notified;
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_matches_the_original_model_tick_over_random_states(void)
{
    OriginalModel_t original;
    ChangeDetect_t d = {0U, false};
    AppState_t last;
    AppState_t now;
    unsigned state = 606U;

    memset(&original, 0, sizeof(original));
    memset(&last, 0, sizeof(last));
    memset(&now, 0, sizeof(now));

    for (unsigned step = 0U; step < 20000U; step++)
    {
        bool expected;
        bool actual = false;

        /* the state changes now and then; most polls see the same state as before */
        if ((next_random(&state) % 7U) == 0U)
        {
            now.led[next_random(&state) % 2U] = (uint8_t)(next_random(&state) % 2U);
            now.ble_status = (uint8_t)(next_random(&state) % 7U);
            now.heartbeat = next_random(&state) % 5U;
            now.fps = (uint16_t)(next_random(&state) % 3U);
            now.uptime_s = next_random(&state) % 4U;
        }

        expected = original_tick(&original, &now);
        if (ChangeDetect_PollDue(&d, 10U))
        {
            actual = ChangeDetect_Update(&d, (uint8_t*)&last, (const uint8_t*)&now, sizeof(now));
        }

        TEST_ASSERT_EQUAL(expected, actual);
        TEST_ASSERT_EQUAL_UINT8(original.tickCount, d.ticks);
        TEST_ASSERT_EQUAL(original.haveLast, d.have_last);
        TEST_ASSERT_EQUAL_MEMORY(&original.last, &last, sizeof(last));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_poll_is_due_on_every_tenth_tick);
    RUN_TEST(test_a_period_of_zero_or_one_is_due_on_every_tick);
    RUN_TEST(test_the_largest_period_is_due_on_the_255th_tick);
    RUN_TEST(test_a_null_detector_is_never_due);
    RUN_TEST(test_the_first_snapshot_always_notifies_even_if_it_equals_the_zeroed_copy);
    RUN_TEST(test_an_unchanged_snapshot_does_not_notify);
    RUN_TEST(test_a_changed_snapshot_notifies_and_is_stored);
    RUN_TEST(test_a_change_in_the_last_byte_is_seen);
    RUN_TEST(test_only_the_given_size_is_compared_and_stored);
    RUN_TEST(test_null_arguments_return_false_and_change_nothing);
    RUN_TEST(test_matches_the_original_model_tick_over_random_states);
    return UNITY_END();
}
