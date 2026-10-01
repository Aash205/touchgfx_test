#include "health_format.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_the_line_has_the_documented_format(void)
{
    char out[160];

    TEST_ASSERT_EQUAL_UINT(strlen("HEALTH ThreadX=OK USBX=ACTIVE TouchGFX_FPS=60 BLE=4 Display=0 "
                                  "DisplayFaults=0 Heartbeat=123"),
                           HealthFormat_Line(out, sizeof(out), true, 60U, 4U, 0U, 0U, 123U));
    TEST_ASSERT_EQUAL_STRING("HEALTH ThreadX=OK USBX=ACTIVE TouchGFX_FPS=60 BLE=4 Display=0 "
                             "DisplayFaults=0 Heartbeat=123",
                             out);
}

void test_usbx_wait_and_large_numbers(void)
{
    char out[160];

    (void)HealthFormat_Line(out, sizeof(out), false, 65535U, 6U, 7U, UINT32_MAX, UINT32_MAX);
    TEST_ASSERT_EQUAL_STRING("HEALTH ThreadX=OK USBX=WAIT TouchGFX_FPS=65535 BLE=6 Display=7 "
                             "DisplayFaults=4294967295 Heartbeat=4294967295",
                             out);
}

void test_single_digit_numbers_are_not_padded(void)
{
    char out[160];

    (void)HealthFormat_Line(out, sizeof(out), true, 0U, 0U, 0U, 0U, 5U);
    TEST_ASSERT_EQUAL_STRING(
        "HEALTH ThreadX=OK USBX=ACTIVE TouchGFX_FPS=0 BLE=0 Display=0 DisplayFaults=0 Heartbeat=5",
        out);
    (void)HealthFormat_Line(out, sizeof(out), false, 7U, 3U, 2U, 9U, 0U);
    TEST_ASSERT_EQUAL_STRING(
        "HEALTH ThreadX=OK USBX=WAIT TouchGFX_FPS=7 BLE=3 Display=2 DisplayFaults=9 Heartbeat=0",
        out);
}

void test_the_longest_line_is_133_characters_and_the_documented_maximum(void)
{
    char big[256];
    char out[HEALTH_FORMAT_MAX_LENGTH + 1U];

    /* the real longest line: every number at UINT32_MAX */
    TEST_ASSERT_EQUAL_UINT(133U, HealthFormat_Line(big, sizeof(big), true, UINT32_MAX, UINT32_MAX,
                                                   UINT32_MAX, UINT32_MAX, UINT32_MAX));
    TEST_ASSERT_EQUAL_UINT(133U, HEALTH_FORMAT_MAX_LENGTH);
    /* a buffer of the documented size plus 1 never cuts it; one byte less cuts the last digit */
    TEST_ASSERT_EQUAL_UINT(133U, HealthFormat_Line(out, sizeof(out), true, UINT32_MAX, UINT32_MAX,
                                                   UINT32_MAX, UINT32_MAX, UINT32_MAX));
    TEST_ASSERT_EQUAL_STRING(big, out);
    TEST_ASSERT_EQUAL_UINT(132U, HealthFormat_Line(out, sizeof(out) - 1U, true, UINT32_MAX,
                                                   UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX));
}

void test_the_line_is_cut_at_capacity_minus_one(void)
{
    char out[10];

    TEST_ASSERT_EQUAL_UINT(9U, HealthFormat_Line(out, sizeof(out), true, 1U, 1U, 1U, 1U, 1U));
    TEST_ASSERT_EQUAL_STRING("HEALTH Th", out);
}

void test_null_output_or_zero_capacity_stores_nothing(void)
{
    char out[4] = {'x', 'x', 'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, HealthFormat_Line(NULL, 8U, true, 1U, 1U, 1U, 1U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, HealthFormat_Line(out, 0U, true, 1U, 1U, 1U, 1U, 1U));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_the_line_matches_the_original_snprintf_code(void)
{
    unsigned rng = 17U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const size_t capacity = 1U + (next_random(&rng) % 130U);
        const bool usbx = (next_random(&rng) % 2U) == 0U;
        const uint32_t fps = (uint16_t)next_random(&rng);
        const uint32_t ble = next_random(&rng) % 8U;
        const uint32_t display = next_random(&rng) % 8U;
        const uint32_t faults = (next_random(&rng) << 8U) ^ next_random(&rng);
        const uint32_t heartbeat = (next_random(&rng) << 8U) ^ next_random(&rng);
        char expected[160];
        char actual[160];

        /* the original USB_Logging_Printf call of the monitor thread */
        snprintf(expected, capacity,
                 "HEALTH ThreadX=OK USBX=%s TouchGFX_FPS=%u BLE=%d Display=%d "
                 "DisplayFaults=%lu Heartbeat=%lu",
                 usbx ? "ACTIVE" : "WAIT", (unsigned)fps, (int)ble, (int)display,
                 (unsigned long)faults, (unsigned long)heartbeat);
        (void)HealthFormat_Line(actual, capacity, usbx, fps, ble, display, faults, heartbeat);
        TEST_ASSERT_EQUAL_STRING(expected, actual);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_line_has_the_documented_format);
    RUN_TEST(test_usbx_wait_and_large_numbers);
    RUN_TEST(test_single_digit_numbers_are_not_padded);
    RUN_TEST(test_the_longest_line_is_133_characters_and_the_documented_maximum);
    RUN_TEST(test_the_line_is_cut_at_capacity_minus_one);
    RUN_TEST(test_null_output_or_zero_capacity_stores_nothing);
    RUN_TEST(test_the_line_matches_the_original_snprintf_code);
    return UNITY_END();
}
