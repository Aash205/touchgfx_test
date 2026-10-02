#include "ble_fsm.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- the state values match BLE_StatusTypeDef ---------------------------------------------- */

void test_state_values_are_the_ble_status_codes(void)
{
    TEST_ASSERT_EQUAL_INT(0, BLE_FSM_IDLE);
    TEST_ASSERT_EQUAL_INT(1, BLE_FSM_INITIALIZING);
    TEST_ASSERT_EQUAL_INT(2, BLE_FSM_INITIALIZED);
    TEST_ASSERT_EQUAL_INT(3, BLE_FSM_ADVERTISING);
    TEST_ASSERT_EQUAL_INT(4, BLE_FSM_CONNECTED);
    TEST_ASSERT_EQUAL_INT(5, BLE_FSM_PAIRED);
    TEST_ASSERT_EQUAL_INT(6, BLE_FSM_ERROR);
}

/* ---- transitions --------------------------------------------------------------------------- */

#define STATE_COUNT 7
#define EVENT_COUNT 9
#define KEEP (-1)

/* expected next state per event; KEEP = the state does not change */
static const int expected_next[EVENT_COUNT] = {
    BLE_FSM_INITIALIZING, /* INIT_STARTED */
    BLE_FSM_INITIALIZED,  /* INIT_SUCCEEDED */
    BLE_FSM_ERROR,        /* INIT_FAILED */
    BLE_FSM_ADVERTISING,  /* ADVERTISE_SUCCEEDED */
    BLE_FSM_ERROR,        /* ADVERTISE_FAILED */
    BLE_FSM_INITIALIZED,  /* STOP_SUCCEEDED */
    BLE_FSM_ERROR,        /* STOP_FAILED */
    BLE_FSM_CONNECTED,    /* CONNECT_SUCCEEDED */
    KEEP                  /* CONNECT_FAILED */
};

void test_every_event_from_every_state_gives_the_expected_state(void)
{
    for (int state = 0; state < STATE_COUNT; state++)
    {
        for (int event = 0; event < EVENT_COUNT; event++)
        {
            const int want = (expected_next[event] == KEEP) ? state : expected_next[event];

            TEST_ASSERT_EQUAL_INT_MESSAGE(want,
                                          BleFsm_Next((BleFsm_State_t)state, (BleFsm_Event_t)event),
                                          "state/event pair");
        }
    }
}

void test_an_event_outside_the_enum_leaves_the_state_unchanged(void)
{
    TEST_ASSERT_EQUAL_INT(BLE_FSM_CONNECTED, BleFsm_Next(BLE_FSM_CONNECTED, (BleFsm_Event_t)9));
    TEST_ASSERT_EQUAL_INT(BLE_FSM_IDLE, BleFsm_Next(BLE_FSM_IDLE, (BleFsm_Event_t)255));
}

void test_a_state_outside_the_enum_survives_a_failed_connect(void)
{
    TEST_ASSERT_EQUAL_INT(42, BleFsm_Next((BleFsm_State_t)42, BLE_FSM_EVENT_CONNECT_FAILED));
}

void test_the_firmware_start_up_and_reconnect_story(void)
{
    BleFsm_State_t s = BLE_FSM_IDLE;

    s = BleFsm_Next(s, BLE_FSM_EVENT_INIT_STARTED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_INITIALIZING, s);
    s = BleFsm_Next(s, BLE_FSM_EVENT_INIT_SUCCEEDED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_INITIALIZED, s);
    s = BleFsm_Next(s, BLE_FSM_EVENT_ADVERTISE_SUCCEEDED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_ADVERTISING, s);
    s = BleFsm_Next(s, BLE_FSM_EVENT_CONNECT_SUCCEEDED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_CONNECTED, s);
    /* disconnect: the firmware re-advertises, and the result of that call moves the state */
    s = BleFsm_Next(s, BLE_FSM_EVENT_ADVERTISE_SUCCEEDED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_ADVERTISING, s);
    s = BleFsm_Next(s, BLE_FSM_EVENT_STOP_SUCCEEDED);
    TEST_ASSERT_EQUAL_INT(BLE_FSM_INITIALIZED, s);
}

/* ---- advertising allowed ------------------------------------------------------------------- */

void test_advertising_may_start_from_every_state_except_error(void)
{
    for (int state = 0; state < STATE_COUNT; state++)
    {
        TEST_ASSERT_EQUAL(state != BLE_FSM_ERROR, BleFsm_MayAdvertise((BleFsm_State_t)state));
    }
}

/* ---- advertising time limit ---------------------------------------------------------------- */

void test_advertising_expires_after_exactly_the_limit(void)
{
    TEST_ASSERT_EQUAL_UINT(120000U, BLE_FSM_ADVERTISING_LIMIT_MS);
    TEST_ASSERT_FALSE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, 1000U, 1000U));
    TEST_ASSERT_FALSE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, 1000U, 120999U));
    TEST_ASSERT_TRUE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, 1000U, 121000U));
    TEST_ASSERT_TRUE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, 1000U, 500000U));
}

void test_advertising_expiry_is_correct_across_the_counter_wrapping(void)
{
    const uint32_t started = UINT32_MAX - 1000U;

    TEST_ASSERT_FALSE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, started, 5000U));
    TEST_ASSERT_TRUE(BleFsm_AdvertisingExpired(BLE_FSM_ADVERTISING, started, 119000U));
}

void test_only_the_advertising_state_can_expire(void)
{
    for (int state = 0; state < STATE_COUNT; state++)
    {
        if (state != BLE_FSM_ADVERTISING)
        {
            TEST_ASSERT_FALSE(BleFsm_AdvertisingExpired((BleFsm_State_t)state, 0U, 500000U));
        }
    }
}

/* ---- differential test against the original expression ------------------------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_expiry_matches_the_original_expression(void)
{
    unsigned rng = 909U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const uint32_t started = (next_random(&rng) << 16) ^ next_random(&rng);
        const uint32_t now = ((step % 3U) == 0U) ? (started + (next_random(&rng) % 240000U))
                                                 : ((next_random(&rng) << 16) ^ next_random(&rng));
        const BleFsm_State_t state = (BleFsm_State_t)(next_random(&rng) % 7U);
        /* the original BLE_App_Process condition */
        const bool original =
            (state == BLE_FSM_ADVERTISING) && ((uint32_t)(now - started) >= 120000UL);

        TEST_ASSERT_EQUAL(original, BleFsm_AdvertisingExpired(state, started, now));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_state_values_are_the_ble_status_codes);
    RUN_TEST(test_every_event_from_every_state_gives_the_expected_state);
    RUN_TEST(test_an_event_outside_the_enum_leaves_the_state_unchanged);
    RUN_TEST(test_a_state_outside_the_enum_survives_a_failed_connect);
    RUN_TEST(test_the_firmware_start_up_and_reconnect_story);
    RUN_TEST(test_advertising_may_start_from_every_state_except_error);
    RUN_TEST(test_advertising_expires_after_exactly_the_limit);
    RUN_TEST(test_advertising_expiry_is_correct_across_the_counter_wrapping);
    RUN_TEST(test_only_the_advertising_state_can_expire);
    RUN_TEST(test_expiry_matches_the_original_expression);
    return UNITY_END();
}
