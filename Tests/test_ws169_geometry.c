#include "unity.h"
#include "ws169_geometry.h"

#include <stddef.h>
#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void assert_geometry(WS169_Rotation_t rotation, uint16_t expected_width,
                            uint16_t expected_height, uint8_t expected_madctl)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_OK, WS169_GetGeometry(rotation, &width, &height, &madctl));
    TEST_ASSERT_EQUAL_UINT16(expected_width, width);
    TEST_ASSERT_EQUAL_UINT16(expected_height, height);
    TEST_ASSERT_EQUAL_HEX8(expected_madctl, madctl);
}

static void assert_window(WS169_Rotation_t rotation, uint16_t x1, uint16_t y1, uint16_t x2,
                          uint16_t y2, uint16_t expected_x1, uint16_t expected_y1,
                          uint16_t expected_x2, uint16_t expected_y2)
{
    WS169_Window_t actual;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_OK,
                          WS169_TranslateWindow(rotation, x1, y1, x2, y2, &actual));
    TEST_ASSERT_EQUAL_UINT16(expected_x1, actual.x_start);
    TEST_ASSERT_EQUAL_UINT16(expected_y1, actual.y_start);
    TEST_ASSERT_EQUAL_UINT16(expected_x2, actual.x_end);
    TEST_ASSERT_EQUAL_UINT16(expected_y2, actual.y_end);
}

static void assert_window_rejected(WS169_Rotation_t rotation, uint16_t x1, uint16_t y1, uint16_t x2,
                                   uint16_t y2)
{
    WS169_Window_t window;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_TranslateWindow(rotation, x1, y1, x2, y2, &window));
}

/* --- geometry per rotation ------------------------------------------------------------- */

void test_rotation_0_geometry(void)
{
    assert_geometry(WS169_ROTATION_0, 240U, 280U, 0x00U);
}

void test_rotation_90_geometry(void)
{
    assert_geometry(WS169_ROTATION_90, 280U, 240U, 0x60U);
}

void test_rotation_180_geometry(void)
{
    assert_geometry(WS169_ROTATION_180, 240U, 280U, 0xC0U);
}

void test_rotation_270_geometry(void)
{
    assert_geometry(WS169_ROTATION_270, 280U, 240U, 0xA0U);
}

/* --- window translation: boundaries ---------------------------------------------------- */

void test_rotation_0_full_bounds(void)
{
    assert_window(WS169_ROTATION_0, 0U, 0U, 239U, 279U, 0U, 20U, 239U, 299U);
}

void test_rotation_90_full_bounds(void)
{
    assert_window(WS169_ROTATION_90, 0U, 0U, 279U, 239U, 20U, 0U, 299U, 239U);
}

void test_rotation_180_full_bounds(void)
{
    assert_window(WS169_ROTATION_180, 0U, 0U, 239U, 279U, 0U, 20U, 239U, 299U);
}

void test_rotation_270_full_bounds(void)
{
    assert_window(WS169_ROTATION_270, 0U, 0U, 279U, 239U, 20U, 0U, 299U, 239U);
}

void test_single_pixel_at_origin(void)
{
    assert_window(WS169_ROTATION_90, 0U, 0U, 0U, 0U, 20U, 0U, 20U, 0U);
}

void test_single_pixel_at_far_corner(void)
{
    assert_window(WS169_ROTATION_90, 279U, 239U, 279U, 239U, 299U, 239U, 299U, 239U);
}

void test_one_pixel_wide_edge_window(void)
{
    assert_window(WS169_ROTATION_0, 239U, 0U, 239U, 279U, 239U, 20U, 239U, 299U);
}

/* --- invalid arguments ----------------------------------------------------------------- */

void test_rotation_count_is_rejected(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_GetGeometry(WS169_ROTATION_COUNT, &width, &height, &madctl));
}

void test_large_invalid_rotation_is_rejected(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_GetGeometry((WS169_Rotation_t)255, &width, &height, &madctl));
}

void test_null_geometry_width_is_rejected(void)
{
    uint16_t height = 0U;
    uint8_t madctl = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_GetGeometry(WS169_ROTATION_0, NULL, &height, &madctl));
}

void test_null_geometry_height_is_rejected(void)
{
    uint16_t width = 0U;
    uint8_t madctl = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_GetGeometry(WS169_ROTATION_0, &width, NULL, &madctl));
}

void test_null_geometry_madctl_is_rejected(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;

    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_GetGeometry(WS169_ROTATION_0, &width, &height, NULL));
}

void test_null_translated_window_is_rejected(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_INVALID_ARGUMENT,
                          WS169_TranslateWindow(WS169_ROTATION_0, 0U, 0U, 0U, 0U, NULL));
}

void test_invalid_window_rotation_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_COUNT, 0U, 0U, 0U, 0U);
}

void test_reversed_x_range_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_90, 10U, 0U, 9U, 0U);
}

void test_reversed_y_range_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_90, 0U, 10U, 0U, 9U);
}

void test_x_exactly_at_width_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_90, 0U, 0U, 280U, 0U);
}

void test_y_exactly_at_height_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_90, 0U, 0U, 0U, 240U);
}

void test_maximum_uint16_coordinate_is_rejected(void)
{
    assert_window_rejected(WS169_ROTATION_90, 0U, 0U, UINT16_MAX, 0U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rotation_0_geometry);
    RUN_TEST(test_rotation_90_geometry);
    RUN_TEST(test_rotation_180_geometry);
    RUN_TEST(test_rotation_270_geometry);
    RUN_TEST(test_rotation_0_full_bounds);
    RUN_TEST(test_rotation_90_full_bounds);
    RUN_TEST(test_rotation_180_full_bounds);
    RUN_TEST(test_rotation_270_full_bounds);
    RUN_TEST(test_single_pixel_at_origin);
    RUN_TEST(test_single_pixel_at_far_corner);
    RUN_TEST(test_one_pixel_wide_edge_window);
    RUN_TEST(test_rotation_count_is_rejected);
    RUN_TEST(test_large_invalid_rotation_is_rejected);
    RUN_TEST(test_null_geometry_width_is_rejected);
    RUN_TEST(test_null_geometry_height_is_rejected);
    RUN_TEST(test_null_geometry_madctl_is_rejected);
    RUN_TEST(test_null_translated_window_is_rejected);
    RUN_TEST(test_invalid_window_rotation_is_rejected);
    RUN_TEST(test_reversed_x_range_is_rejected);
    RUN_TEST(test_reversed_y_range_is_rejected);
    RUN_TEST(test_x_exactly_at_width_is_rejected);
    RUN_TEST(test_y_exactly_at_height_is_rejected);
    RUN_TEST(test_maximum_uint16_coordinate_is_rejected);
    return UNITY_END();
}
