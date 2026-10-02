#include "unity.h"
#include "ws169_wire.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- window parameter bytes ---------------------------------------------------------------- */

void test_window_bytes_are_start_then_end_high_byte_first(void)
{
    const WS169_Window_t window = {0x0102U, 0x0304U, 0x0506U, 0x0708U};
    uint8_t columns[WS169_WINDOW_BYTES];
    uint8_t rows[WS169_WINDOW_BYTES];

    WS169_EncodeWindow(&window, columns, rows);
    TEST_ASSERT_EQUAL_HEX8(0x01U, columns[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02U, columns[1]);
    TEST_ASSERT_EQUAL_HEX8(0x05U, columns[2]);
    TEST_ASSERT_EQUAL_HEX8(0x06U, columns[3]);
    TEST_ASSERT_EQUAL_HEX8(0x03U, rows[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04U, rows[1]);
    TEST_ASSERT_EQUAL_HEX8(0x07U, rows[2]);
    TEST_ASSERT_EQUAL_HEX8(0x08U, rows[3]);
}

void test_the_full_landscape_window_is_encoded_with_the_controller_offset(void)
{
    /* rotation 90: x 0..279 maps to RAM columns 20..299 */
    WS169_Window_t window;
    uint8_t columns[WS169_WINDOW_BYTES];
    uint8_t rows[WS169_WINDOW_BYTES];

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_OK,
                          WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, 279U, 239U, &window));
    WS169_EncodeWindow(&window, columns, rows);
    TEST_ASSERT_EQUAL_HEX8(0x00U, columns[0]);
    TEST_ASSERT_EQUAL_HEX8(0x14U, columns[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, columns[2]);
    TEST_ASSERT_EQUAL_HEX8(0x2BU, columns[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, rows[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, rows[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, rows[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEFU, rows[3]);
}

void test_a_null_window_or_output_is_skipped(void)
{
    const WS169_Window_t window = {1U, 2U, 3U, 4U};
    uint8_t columns[WS169_WINDOW_BYTES] = {0xAAU, 0xAAU, 0xAAU, 0xAAU};
    uint8_t rows[WS169_WINDOW_BYTES] = {0xBBU, 0xBBU, 0xBBU, 0xBBU};

    WS169_EncodeWindow(NULL, columns, rows);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, columns[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBBU, rows[0]);

    WS169_EncodeWindow(&window, NULL, rows);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, columns[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, rows[0]);
    WS169_EncodeWindow(&window, columns, NULL);
    TEST_ASSERT_EQUAL_HEX8(0x00U, columns[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, columns[1]);
}

/* ---- one row of a single colour ------------------------------------------------------------ */

void test_a_row_has_two_bytes_per_pixel_high_byte_first(void)
{
    uint8_t row[8] = {0};

    TEST_ASSERT_EQUAL_UINT(6U, WS169_FillRowRGB565(row, sizeof(row), 3U, 0x1234U));
    TEST_ASSERT_EQUAL_HEX8(0x12U, row[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, row[1]);
    TEST_ASSERT_EQUAL_HEX8(0x12U, row[4]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, row[5]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, row[6]);
}

void test_black_and_white_rows(void)
{
    uint8_t row[4];

    TEST_ASSERT_EQUAL_UINT(4U, WS169_FillRowRGB565(row, sizeof(row), 2U, 0x0000U));
    TEST_ASSERT_EQUAL_HEX8(0x00U, row[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, row[3]);
    TEST_ASSERT_EQUAL_UINT(4U, WS169_FillRowRGB565(row, sizeof(row), 2U, 0xFFFFU));
    TEST_ASSERT_EQUAL_HEX8(0xFFU, row[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFFU, row[3]);
}

void test_a_row_that_does_not_fit_or_has_no_buffer_writes_nothing(void)
{
    uint8_t row[4] = {0x55U, 0x55U, 0x55U, 0x55U};

    TEST_ASSERT_EQUAL_UINT(0U, WS169_FillRowRGB565(row, 3U, 2U, 0x1234U));
    TEST_ASSERT_EQUAL_HEX8(0x55U, row[0]);
    TEST_ASSERT_EQUAL_UINT(0U, WS169_FillRowRGB565(NULL, 4U, 2U, 0x1234U));
    TEST_ASSERT_EQUAL_UINT(0U, WS169_FillRowRGB565(row, sizeof(row), 0U, 0x1234U));
    TEST_ASSERT_EQUAL_HEX8(0x55U, row[0]);
}

void test_the_widest_row_of_the_panel_fits_its_buffer(void)
{
    uint8_t row[WS169_LANDSCAPE_WIDTH * 2U];

    TEST_ASSERT_EQUAL_UINT(560U,
                           WS169_FillRowRGB565(row, sizeof(row), WS169_LANDSCAPE_WIDTH, 0xA55AU));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, row[558]);
    TEST_ASSERT_EQUAL_HEX8(0x5AU, row[559]);
}

/* ---- differential tests against the original driver code ----------------------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_window_bytes_match_the_original_expressions(void)
{
    unsigned rng = 55U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const WS169_Window_t window = {(uint16_t)next_random(&rng), (uint16_t)next_random(&rng),
                                       (uint16_t)next_random(&rng), (uint16_t)next_random(&rng)};
        uint8_t data[4];
        uint8_t columns[WS169_WINDOW_BYTES];
        uint8_t rows[WS169_WINDOW_BYTES];

        WS169_EncodeWindow(&window, columns, rows);
        /* the original ws169_set_address_window_unlocked */
        data[0] = (uint8_t)(window.x_start >> 8);
        data[1] = (uint8_t)(window.x_start & 0xFFU);
        data[2] = (uint8_t)(window.x_end >> 8);
        data[3] = (uint8_t)(window.x_end & 0xFFU);
        TEST_ASSERT_EQUAL_MEMORY(data, columns, 4U);
        data[0] = (uint8_t)(window.y_start >> 8);
        data[1] = (uint8_t)(window.y_start & 0xFFU);
        data[2] = (uint8_t)(window.y_end >> 8);
        data[3] = (uint8_t)(window.y_end & 0xFFU);
        TEST_ASSERT_EQUAL_MEMORY(data, rows, 4U);
    }
}

void test_rows_match_the_original_fill_loop(void)
{
    unsigned rng = 66U;

    for (unsigned step = 0U; step < 2000U; step++)
    {
        const uint16_t width = (uint16_t)(1U + (next_random(&rng) % WS169_LANDSCAPE_WIDTH));
        const uint16_t color = (uint16_t)next_random(&rng);
        uint8_t expected[WS169_LANDSCAPE_WIDTH * 2U];
        uint8_t actual[WS169_LANDSCAPE_WIDTH * 2U];

        memset(expected, 0xCC, sizeof(expected));
        memset(actual, 0xCC, sizeof(actual));
        /* the original WS169_FillScreenRGB565 loop */
        for (uint16_t x = 0U; x < width; x++)
        {
            expected[(uint32_t)x * 2U] = (uint8_t)(color >> 8);
            expected[((uint32_t)x * 2U) + 1U] = (uint8_t)(color & 0xFFU);
        }

        TEST_ASSERT_EQUAL_UINT((size_t)width * 2U,
                               WS169_FillRowRGB565(actual, sizeof(actual), width, color));
        TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(expected));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_window_bytes_are_start_then_end_high_byte_first);
    RUN_TEST(test_the_full_landscape_window_is_encoded_with_the_controller_offset);
    RUN_TEST(test_a_null_window_or_output_is_skipped);
    RUN_TEST(test_a_row_has_two_bytes_per_pixel_high_byte_first);
    RUN_TEST(test_black_and_white_rows);
    RUN_TEST(test_a_row_that_does_not_fit_or_has_no_buffer_writes_nothing);
    RUN_TEST(test_the_widest_row_of_the_panel_fits_its_buffer);
    RUN_TEST(test_window_bytes_match_the_original_expressions);
    RUN_TEST(test_rows_match_the_original_fill_loop);
    return UNITY_END();
}
