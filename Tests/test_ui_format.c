#include "ui_format.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- BLE line ------------------------------------------------------------------------------ */

void test_every_ble_status_has_its_name(void)
{
    static const char* const expected[7] = {"BLE: Idle",        "BLE: Init",      "BLE: Ready",
                                            "BLE: Advertising", "BLE: Connected", "BLE: Paired",
                                            "BLE: Error"};
    char out[33];

    for (uint8_t status = 0U; status < 7U; status++)
    {
        (void)UiFormat_BleLine(out, sizeof(out), status);
        TEST_ASSERT_EQUAL_STRING(expected[status], out);
    }
}

void test_a_ble_status_beyond_the_table_is_shown_as_error(void)
{
    char out[33];

    (void)UiFormat_BleLine(out, sizeof(out), 7U);
    TEST_ASSERT_EQUAL_STRING("BLE: Error", out);
    (void)UiFormat_BleLine(out, sizeof(out), 255U);
    TEST_ASSERT_EQUAL_STRING("BLE: Error", out);
}

/* ---- uptime line --------------------------------------------------------------------------- */

void test_uptime_is_hours_minutes_seconds(void)
{
    char out[33];

    (void)UiFormat_UptimeLine(out, sizeof(out), 0U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 00:00:00", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), 59U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 00:00:59", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), 60U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 00:01:00", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), 3599U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 00:59:59", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), 3600U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 01:00:00", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), 45296U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 12:34:56", out);
}

void test_uptime_hours_grow_beyond_two_digits(void)
{
    char out[33];

    (void)UiFormat_UptimeLine(out, sizeof(out), 100U * 3600U);
    TEST_ASSERT_EQUAL_STRING("Uptime: 100:00:00", out);
    (void)UiFormat_UptimeLine(out, sizeof(out), UINT32_MAX);
    TEST_ASSERT_EQUAL_STRING("Uptime: 1193046:28:15", out);
}

/* ---- heartbeat and fps --------------------------------------------------------------------- */

void test_heartbeat_and_fps_lines(void)
{
    char out[33];

    (void)UiFormat_HeartbeatLine(out, sizeof(out), 0U);
    TEST_ASSERT_EQUAL_STRING("Heartbeat: 0", out);
    (void)UiFormat_HeartbeatLine(out, sizeof(out), 123456U);
    TEST_ASSERT_EQUAL_STRING("Heartbeat: 123456", out);
    (void)UiFormat_HeartbeatLine(out, sizeof(out), UINT32_MAX);
    TEST_ASSERT_EQUAL_STRING("Heartbeat: 4294967295", out);
    (void)UiFormat_FpsLine(out, sizeof(out), 0U);
    TEST_ASSERT_EQUAL_STRING("FPS: 0", out);
    (void)UiFormat_FpsLine(out, sizeof(out), 60U);
    TEST_ASSERT_EQUAL_STRING("FPS: 60", out);
    (void)UiFormat_FpsLine(out, sizeof(out), 65535U);
    TEST_ASSERT_EQUAL_STRING("FPS: 65535", out);
}

/* ---- LED labels ---------------------------------------------------------------------------- */

void test_led_labels(void)
{
    TEST_ASSERT_EQUAL_STRING("LD1 ON", UiFormat_LedLabel(0U, true));
    TEST_ASSERT_EQUAL_STRING("LD1 OFF", UiFormat_LedLabel(0U, false));
    TEST_ASSERT_EQUAL_STRING("LD3 ON", UiFormat_LedLabel(1U, true));
    TEST_ASSERT_EQUAL_STRING("LD3 OFF", UiFormat_LedLabel(1U, false));
}

void test_an_led_index_beyond_the_table_gives_an_empty_label_never_null(void)
{
    TEST_ASSERT_NOT_NULL(UiFormat_LedLabel(2U, true));
    TEST_ASSERT_EQUAL_STRING("", UiFormat_LedLabel(2U, true));
    TEST_ASSERT_EQUAL_STRING("", UiFormat_LedLabel(255U, false));
}

void test_the_led_table_covers_exactly_the_declared_led_count(void)
{
    TEST_ASSERT_EQUAL_UINT(2U, UI_FORMAT_LED_COUNT);
    for (uint8_t i = 0U; i < UI_FORMAT_LED_COUNT; i++)
    {
        TEST_ASSERT_NOT_EQUAL('\0', UiFormat_LedLabel(i, true)[0]);
        TEST_ASSERT_NOT_EQUAL('\0', UiFormat_LedLabel(i, false)[0]);
    }
    TEST_ASSERT_EQUAL_STRING("", UiFormat_LedLabel(UI_FORMAT_LED_COUNT, true));
}

/* ---- truncation and bad arguments ---------------------------------------------------------- */

void test_lines_are_cut_at_capacity_minus_one(void)
{
    char out[8];

    TEST_ASSERT_EQUAL_UINT(7U, UiFormat_BleLine(out, sizeof(out), 3U));
    TEST_ASSERT_EQUAL_STRING("BLE: Ad", out);
    TEST_ASSERT_EQUAL_UINT(7U, UiFormat_UptimeLine(out, sizeof(out), 1U));
    TEST_ASSERT_EQUAL_STRING("Uptime:", out);
    TEST_ASSERT_EQUAL_UINT(7U, UiFormat_HeartbeatLine(out, sizeof(out), 1U));
    TEST_ASSERT_EQUAL_STRING("Heartbe", out);
}

void test_null_output_or_zero_capacity_stores_nothing(void)
{
    char out[4] = {'x', 'x', 'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, UiFormat_BleLine(NULL, 8U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, UiFormat_UptimeLine(NULL, 8U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, UiFormat_HeartbeatLine(NULL, 8U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, UiFormat_FpsLine(NULL, 8U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, UiFormat_FpsLine(out, 0U, 1U));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
}

/* ---- differential tests against the original Screen1View::updateState --------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_lines_match_the_original_snprintf_code(void)
{
    static const char* const bleNames[] = {"Idle",      "Init",   "Ready", "Advertising",
                                           "Connected", "Paired", "Error"};
    unsigned state = 808U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const size_t capacity = 1U + (next_random(&state) % 40U);
        const uint8_t ble_status = (uint8_t)next_random(&state);
        uint32_t uptime_s = next_random(&state) << 8U;
        const uint32_t heartbeat = (next_random(&state) << 8U) ^ next_random(&state);
        const uint16_t fps = (uint16_t)next_random(&state);
        char expected[64];
        char actual[64];

        if ((step % 4U) == 0U)
        {
            uptime_s >>= (next_random(&state) % 24U);
        }

        /* the original: char line[DynText::MAX_CHARS + 1] and snprintf into it */
        {
            const uint8_t bleIdx = (ble_status < 7) ? ble_status : 6;

            snprintf(expected, capacity, "BLE: %s", bleNames[bleIdx]);
            (void)UiFormat_BleLine(actual, capacity, ble_status);
            TEST_ASSERT_EQUAL_STRING(expected, actual);
        }
        snprintf(expected, capacity, "Uptime: %02lu:%02lu:%02lu",
                 (unsigned long)(uptime_s / 3600UL), (unsigned long)((uptime_s / 60UL) % 60UL),
                 (unsigned long)(uptime_s % 60UL));
        (void)UiFormat_UptimeLine(actual, capacity, uptime_s);
        TEST_ASSERT_EQUAL_STRING(expected, actual);

        snprintf(expected, capacity, "Heartbeat: %lu", (unsigned long)heartbeat);
        (void)UiFormat_HeartbeatLine(actual, capacity, heartbeat);
        TEST_ASSERT_EQUAL_STRING(expected, actual);

        snprintf(expected, capacity, "FPS: %u", (unsigned)fps);
        (void)UiFormat_FpsLine(actual, capacity, fps);
        TEST_ASSERT_EQUAL_STRING(expected, actual);
    }
}

void test_led_labels_match_the_original_tables(void)
{
    static const char* const onLabels[2] = {"LD1 ON", "LD3 ON"};
    static const char* const offLabels[2] = {"LD1 OFF", "LD3 OFF"};

    for (uint8_t i = 0U; i < 2U; i++)
    {
        TEST_ASSERT_EQUAL_STRING(onLabels[i], UiFormat_LedLabel(i, true));
        TEST_ASSERT_EQUAL_STRING(offLabels[i], UiFormat_LedLabel(i, false));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_ble_status_has_its_name);
    RUN_TEST(test_a_ble_status_beyond_the_table_is_shown_as_error);
    RUN_TEST(test_uptime_is_hours_minutes_seconds);
    RUN_TEST(test_uptime_hours_grow_beyond_two_digits);
    RUN_TEST(test_heartbeat_and_fps_lines);
    RUN_TEST(test_led_labels);
    RUN_TEST(test_an_led_index_beyond_the_table_gives_an_empty_label_never_null);
    RUN_TEST(test_the_led_table_covers_exactly_the_declared_led_count);
    RUN_TEST(test_lines_are_cut_at_capacity_minus_one);
    RUN_TEST(test_null_output_or_zero_capacity_stores_nothing);
    RUN_TEST(test_lines_match_the_original_snprintf_code);
    RUN_TEST(test_led_labels_match_the_original_tables);
    return UNITY_END();
}
