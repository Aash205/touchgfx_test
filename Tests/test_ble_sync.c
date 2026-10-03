#include "ble_sync.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- active -------------------------------------------------------------------------------- */

void test_sync_is_active_only_when_connected_with_a_service(void)
{
    for (int state = 0; state < 7; state++)
    {
        TEST_ASSERT_EQUAL(state == BLE_FSM_CONNECTED, BleSync_Active((BleFsm_State_t)state, true));
        TEST_ASSERT_FALSE(BleSync_Active((BleFsm_State_t)state, false));
    }
}

/* ---- LED characteristic -------------------------------------------------------------------- */

void test_the_led_characteristic_is_due_when_the_mask_changed(void)
{
    TEST_ASSERT_FALSE(BleSync_LedDue(0x00U, 0x00U));
    TEST_ASSERT_FALSE(BleSync_LedDue(0x03U, 0x03U));
    TEST_ASSERT_TRUE(BleSync_LedDue(0x01U, 0x00U));
    TEST_ASSERT_TRUE(BleSync_LedDue(0x00U, 0x01U));
}

void test_the_forced_first_sync_value_never_equals_a_real_mask(void)
{
    /* the firmware stores UINT8_MAX to force a write: the LED mask has only 2 bits */
    for (unsigned int mask = 0U; mask < 4U; mask++)
    {
        TEST_ASSERT_TRUE(BleSync_LedDue((uint8_t)mask, UINT8_MAX));
    }
}

/* ---- status notification ------------------------------------------------------------------- */

void test_the_status_is_due_after_exactly_one_second(void)
{
    TEST_ASSERT_EQUAL_UINT(1000U, BLE_SYNC_STATUS_PERIOD_MS);
    TEST_ASSERT_FALSE(BleSync_StatusDue(5000U, 5000U));
    TEST_ASSERT_FALSE(BleSync_StatusDue(5999U, 5000U));
    TEST_ASSERT_TRUE(BleSync_StatusDue(6000U, 5000U));
    TEST_ASSERT_TRUE(BleSync_StatusDue(60000U, 5000U));
}

void test_the_first_status_waits_for_a_full_second_after_boot(void)
{
    /* last_status_tick starts at 0 in the firmware */
    TEST_ASSERT_FALSE(BleSync_StatusDue(999U, 0U));
    TEST_ASSERT_TRUE(BleSync_StatusDue(1000U, 0U));
}

void test_the_status_period_is_correct_across_the_counter_wrapping(void)
{
    const uint32_t last = UINT32_MAX - 499U;

    TEST_ASSERT_FALSE(BleSync_StatusDue(400U, last));
    TEST_ASSERT_TRUE(BleSync_StatusDue(500U, last));
}

/* ---- differential tests against the original expressions ----------------------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_status_due_matches_the_original_expression(void)
{
    unsigned rng = 321U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const uint32_t last = (next_random(&rng) << 16) ^ next_random(&rng);
        const uint32_t now = ((step % 2U) == 0U) ? (last + (next_random(&rng) % 3000U))
                                                 : ((next_random(&rng) << 16) ^ next_random(&rng));

        /* the original ble_sync condition */
        TEST_ASSERT_EQUAL((now - last) >= 1000U, BleSync_StatusDue(now, last));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sync_is_active_only_when_connected_with_a_service);
    RUN_TEST(test_the_led_characteristic_is_due_when_the_mask_changed);
    RUN_TEST(test_the_forced_first_sync_value_never_equals_a_real_mask);
    RUN_TEST(test_the_status_is_due_after_exactly_one_second);
    RUN_TEST(test_the_first_status_waits_for_a_full_second_after_boot);
    RUN_TEST(test_the_status_period_is_correct_across_the_counter_wrapping);
    RUN_TEST(test_status_due_matches_the_original_expression);
    return UNITY_END();
}
