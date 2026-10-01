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

void test_the_first_state_always_notifies_even_if_it_is_not_different(void)
{
    ChangeDetect_t d = {0U, false};

    TEST_ASSERT_TRUE(ChangeDetect_Changed(&d, false));
    TEST_ASSERT_TRUE(d.have_last);
}

void test_an_unchanged_state_does_not_notify(void)
{
    ChangeDetect_t d = {0U, false};

    TEST_ASSERT_TRUE(ChangeDetect_Changed(&d, true));
    TEST_ASSERT_FALSE(ChangeDetect_Changed(&d, false));
    TEST_ASSERT_FALSE(ChangeDetect_Changed(&d, false));
}

void test_a_changed_state_notifies(void)
{
    ChangeDetect_t d = {0U, false};

    (void)ChangeDetect_Changed(&d, false);
    TEST_ASSERT_TRUE(ChangeDetect_Changed(&d, true));
    TEST_ASSERT_FALSE(ChangeDetect_Changed(&d, false));
}

void test_a_null_detector_never_notifies(void)
{
    TEST_ASSERT_FALSE(ChangeDetect_Changed(NULL, true));
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

static bool same_state(const AppState_t* x, const AppState_t* y)
{
    return (x->led[0] == y->led[0]) && (x->led[1] == y->led[1]) &&
           (x->ble_status == y->ble_status) && (x->uptime_s == y->uptime_s) &&
           (x->heartbeat == y->heartbeat) && (x->fps == y->fps);
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
            /* the firmware compares field by field; here the structs have no stale padding */
            actual = ChangeDetect_Changed(&d, !same_state(&now, &last));
            if (actual)
            {
                last = now;
            }
        }

        TEST_ASSERT_EQUAL(expected, actual);
        TEST_ASSERT_EQUAL_UINT8(original.tickCount, d.ticks);
        TEST_ASSERT_EQUAL(original.haveLast, d.have_last);
        TEST_ASSERT_TRUE(same_state(&original.last, &last));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_poll_is_due_on_every_tenth_tick);
    RUN_TEST(test_a_period_of_zero_or_one_is_due_on_every_tick);
    RUN_TEST(test_the_largest_period_is_due_on_the_255th_tick);
    RUN_TEST(test_a_null_detector_is_never_due);
    RUN_TEST(test_the_first_state_always_notifies_even_if_it_is_not_different);
    RUN_TEST(test_an_unchanged_state_does_not_notify);
    RUN_TEST(test_a_changed_state_notifies);
    RUN_TEST(test_a_null_detector_never_notifies);
    RUN_TEST(test_matches_the_original_model_tick_over_random_states);
    return UNITY_END();
}
