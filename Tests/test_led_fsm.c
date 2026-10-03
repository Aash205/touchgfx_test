#include "led_fsm.h"
#include "unity.h"

#include <stdbool.h>
#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- init ---------------------------------------------------------------------------------- */

void test_init_gives_an_off_led_with_the_default_period(void)
{
    Led_t led = {LED_BLINK_FAST, 99U, 99U};

    Led_Init(&led);
    TEST_ASSERT_EQUAL_INT(LED_OFF, led.state);
    TEST_ASSERT_EQUAL_UINT32(0U, led.last_toggle_ms);
    TEST_ASSERT_EQUAL_UINT32(100U, led.half_period_ms);
}

void test_a_null_led_is_ignored_everywhere(void)
{
    Led_Init(NULL);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Set(NULL, LED_ON, 0U, false));
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(NULL, 0U));
}

/* ---- set ----------------------------------------------------------------------------------- */

void test_set_on_and_off_drive_the_pin(void)
{
    Led_t led;

    Led_Init(&led);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HIGH, Led_Set(&led, LED_ON, 5U, false));
    TEST_ASSERT_EQUAL_INT(LED_ON, led.state);
    TEST_ASSERT_EQUAL_INT(LED_PIN_LOW, Led_Set(&led, LED_OFF, 5U, true));
    TEST_ASSERT_EQUAL_INT(LED_OFF, led.state);
}

void test_set_toggle_settles_on_the_opposite_of_the_pin_level(void)
{
    Led_t led;

    Led_Init(&led);
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Set(&led, LED_TOGGLE, 0U, false));
    TEST_ASSERT_EQUAL_INT(LED_ON, led.state);
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Set(&led, LED_TOGGLE, 0U, true));
    TEST_ASSERT_EQUAL_INT(LED_OFF, led.state);
}

void test_set_blink_states_set_the_period_and_start_the_timer(void)
{
    Led_t led;

    Led_Init(&led);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Set(&led, LED_BLINK_SLOW, 1234U, true));
    TEST_ASSERT_EQUAL_INT(LED_BLINK_SLOW, led.state);
    TEST_ASSERT_EQUAL_UINT32(500U, led.half_period_ms);
    TEST_ASSERT_EQUAL_UINT32(1234U, led.last_toggle_ms);

    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Set(&led, LED_BLINK_FAST, 2000U, false));
    TEST_ASSERT_EQUAL_INT(LED_BLINK_FAST, led.state);
    TEST_ASSERT_EQUAL_UINT32(100U, led.half_period_ms);
    TEST_ASSERT_EQUAL_UINT32(2000U, led.last_toggle_ms);
}

void test_set_with_a_value_outside_the_enum_only_stores_it(void)
{
    Led_t led;

    Led_Init(&led);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Set(&led, (LED_StateTypeDef)42, 7U, true));
    TEST_ASSERT_EQUAL_INT(42, (int)led.state);
    TEST_ASSERT_EQUAL_UINT32(0U, led.last_toggle_ms);
    TEST_ASSERT_EQUAL_UINT32(100U, led.half_period_ms);
}

/* ---- update -------------------------------------------------------------------------------- */

void test_update_re_asserts_the_pin_level_for_on_and_off(void)
{
    Led_t led;

    Led_Init(&led);
    (void)Led_Set(&led, LED_ON, 0U, false);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HIGH, Led_Update(&led, 10U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_HIGH, Led_Update(&led, 20U));
    (void)Led_Set(&led, LED_OFF, 0U, true);
    TEST_ASSERT_EQUAL_INT(LED_PIN_LOW, Led_Update(&led, 30U));
}

void test_update_toggles_once_and_becomes_off_for_a_pending_toggle(void)
{
    Led_t led = {LED_TOGGLE, 0U, 100U};

    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 0U));
    TEST_ASSERT_EQUAL_INT(LED_OFF, led.state);
    TEST_ASSERT_EQUAL_INT(LED_PIN_LOW, Led_Update(&led, 1U));
}

void test_a_blinking_led_toggles_every_half_period(void)
{
    Led_t led;

    Led_Init(&led);
    (void)Led_Set(&led, LED_BLINK_FAST, 1000U, false);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 1000U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 1099U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 1100U));
    TEST_ASSERT_EQUAL_UINT32(1100U, led.last_toggle_ms);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 1199U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 1250U));
    TEST_ASSERT_EQUAL_UINT32(1250U, led.last_toggle_ms);
}

void test_slow_blink_uses_five_hundred_milliseconds(void)
{
    Led_t led;

    Led_Init(&led);
    (void)Led_Set(&led, LED_BLINK_SLOW, 0U, false);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 499U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 500U));
}

void test_blinking_survives_the_tick_counter_wrapping(void)
{
    Led_t led;

    Led_Init(&led);
    (void)Led_Set(&led, LED_BLINK_FAST, UINT32_MAX - 49U, false);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 49U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 50U));
}

void test_a_blink_started_just_before_the_wrap_does_not_toggle_early(void)
{
    Led_t led;

    Led_Init(&led);
    (void)Led_Set(&led, LED_BLINK_FAST, UINT32_MAX - 10U, false);
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, UINT32_MAX - 5U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 20U));
    TEST_ASSERT_EQUAL_INT(LED_PIN_TOGGLE, Led_Update(&led, 89U));
}

void test_update_of_a_value_outside_the_enum_does_nothing(void)
{
    Led_t led = {(LED_StateTypeDef)42, 5U, 100U};

    TEST_ASSERT_EQUAL_INT(LED_PIN_HOLD, Led_Update(&led, 99999U));
    TEST_ASSERT_EQUAL_INT(42, (int)led.state);
    TEST_ASSERT_EQUAL_UINT32(5U, led.last_toggle_ms);
}

/* ---- differential test against the original uart_commands.c code --------------------------- */

/* The original LED fields. The GPIO calls act on a simulated pin passed by pointer. */
typedef struct
{
    LED_StateTypeDef state;
    uint32_t blink_count;
    uint32_t blink_period;
} OriginalLed_t;

/* UART_CMD_UpdateLEDs for one LED, as it was (HAL_GetTick is the tick argument). */
static void original_update(OriginalLed_t* led, bool* pin, uint32_t tick)
{
    if (led->state == LED_ON)
    {
        *pin = true;
    }
    else if (led->state == LED_OFF)
    {
        *pin = false;
    }
    else if (led->state == LED_TOGGLE)
    {
        *pin = !*pin;
        led->state = LED_OFF;
    }
    else if (led->state == LED_BLINK_SLOW || led->state == LED_BLINK_FAST)
    {
        if ((tick - led->blink_count) >= led->blink_period)
        {
            *pin = !*pin;
            led->blink_count = tick;
        }
    }
}

/* UART_CMD_SetLEDState for one LED, as it was. */
static void original_set(OriginalLed_t* led, LED_StateTypeDef state, bool* pin, uint32_t tick)
{
    led->state = state;

    if (state == LED_ON)
    {
        *pin = true;
    }
    else if (state == LED_OFF)
    {
        *pin = false;
    }
    else if (state == LED_TOGGLE)
    {
        *pin = !*pin;
        led->state = *pin ? LED_ON : LED_OFF;
    }
    else if (state == LED_BLINK_SLOW)
    {
        led->blink_period = 500U;
        led->blink_count = tick;
    }
    else if (state == LED_BLINK_FAST)
    {
        led->blink_period = 100U;
        led->blink_count = tick;
    }
}

static void apply(LedPinAction_t action, bool* pin)
{
    if (action == LED_PIN_HIGH)
    {
        *pin = true;
    }
    else if (action == LED_PIN_LOW)
    {
        *pin = false;
    }
    else if (action == LED_PIN_TOGGLE)
    {
        *pin = !*pin;
    }
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

static void run_differential(uint32_t first_tick)
{
    unsigned state = 321U;
    OriginalLed_t original = {LED_OFF, 0U, 100U};
    Led_t led;
    bool original_pin = false;
    bool new_pin = false;
    uint32_t tick = first_tick;

    Led_Init(&led);

    for (unsigned step = 0U; step < 20000U; step++)
    {
        tick += next_random(&state) % 120U;

        if ((next_random(&state) % 4U) == 0U)
        {
            /* values 0..5 include one outside the enum */
            const LED_StateTypeDef wanted = (LED_StateTypeDef)(next_random(&state) % 6U);
            const bool pin_before = new_pin;

            original_set(&original, wanted, &original_pin, tick);
            apply(Led_Set(&led, wanted, tick, pin_before), &new_pin);
        }
        else
        {
            original_update(&original, &original_pin, tick);
            apply(Led_Update(&led, tick), &new_pin);
        }

        TEST_ASSERT_EQUAL_INT(original.state, led.state);
        TEST_ASSERT_EQUAL_UINT32(original.blink_count, led.last_toggle_ms);
        TEST_ASSERT_EQUAL_UINT32(original.blink_period, led.half_period_ms);
        TEST_ASSERT_EQUAL(original_pin, new_pin);
    }
}

void test_matches_the_original_over_random_operations(void)
{
    run_differential(0U);
}

void test_matches_the_original_across_the_tick_wrap(void)
{
    run_differential(UINT32_MAX - 2000U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_gives_an_off_led_with_the_default_period);
    RUN_TEST(test_a_null_led_is_ignored_everywhere);
    RUN_TEST(test_set_on_and_off_drive_the_pin);
    RUN_TEST(test_set_toggle_settles_on_the_opposite_of_the_pin_level);
    RUN_TEST(test_set_blink_states_set_the_period_and_start_the_timer);
    RUN_TEST(test_set_with_a_value_outside_the_enum_only_stores_it);
    RUN_TEST(test_update_re_asserts_the_pin_level_for_on_and_off);
    RUN_TEST(test_update_toggles_once_and_becomes_off_for_a_pending_toggle);
    RUN_TEST(test_a_blinking_led_toggles_every_half_period);
    RUN_TEST(test_slow_blink_uses_five_hundred_milliseconds);
    RUN_TEST(test_blinking_survives_the_tick_counter_wrapping);
    RUN_TEST(test_a_blink_started_just_before_the_wrap_does_not_toggle_early);
    RUN_TEST(test_update_of_a_value_outside_the_enum_does_nothing);
    RUN_TEST(test_matches_the_original_over_random_operations);
    RUN_TEST(test_matches_the_original_across_the_tick_wrap);
    return UNITY_END();
}
