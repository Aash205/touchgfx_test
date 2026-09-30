/**
  * @file    WS169_driver_tests.c
  * @brief   Non-destructive WS169 driver geometry and validation tests.
  */
#include "WS169_driver_tests.h"
#include "waveshare_driver.h"
#include <stdbool.h>
#include <stddef.h>

static void record_result(WS169_TestReport_t *report, bool passed, uint32_t failure_bit)
{
  report->executed++;
  if (passed) { report->passed++; }
  else { report->failed++; report->failure_mask |= failure_bit; }
}

static bool geometry_matches(WS169_Rotation_t rotation, uint16_t expected_width,
                             uint16_t expected_height, uint8_t expected_madctl)
{
  uint16_t width = 0U;
  uint16_t height = 0U;
  uint8_t madctl = 0U;
  return (WS169_GetGeometry(rotation, &width, &height, &madctl) == WS169_STATUS_OK) &&
         (width == expected_width) && (height == expected_height) &&
         (madctl == expected_madctl);
}

static bool window_matches(WS169_Rotation_t rotation, uint16_t x_end, uint16_t y_end,
                           uint16_t expected_x_start, uint16_t expected_y_start,
                           uint16_t expected_x_end, uint16_t expected_y_end)
{
  WS169_Window_t window = {0U, 0U, 0U, 0U};
  return (WS169_TranslateWindow(rotation, 0U, 0U, x_end, y_end, &window) == WS169_STATUS_OK) &&
         (window.x_start == expected_x_start) && (window.y_start == expected_y_start) &&
         (window.x_end == expected_x_end) && (window.y_end == expected_y_end);
}

void WS169_DriverTests_Run(WS169_TestReport_t *report)
{
  WS169_Window_t window;
  if (report != NULL)
  {
    report->executed = 0U;
    report->passed = 0U;
    report->failed = 0U;
    report->failure_mask = 0U;
    record_result(report, (WS169_PORTRAIT_WIDTH == 240U) &&
                  (WS169_PORTRAIT_HEIGHT == 280U) &&
                  (WS169_CONTROLLER_RAM_OFFSET == 20U), WS169_TEST_PANEL_CONSTANTS);
    record_result(report, geometry_matches(WS169_ROTATION_0, 240U, 280U, 0x00U), WS169_TEST_GEOMETRY_0);
    record_result(report, geometry_matches(WS169_ROTATION_90, 280U, 240U, 0x60U), WS169_TEST_GEOMETRY_90);
    record_result(report, geometry_matches(WS169_ROTATION_180, 240U, 280U, 0xC0U), WS169_TEST_GEOMETRY_180);
    record_result(report, geometry_matches(WS169_ROTATION_270, 280U, 240U, 0xA0U), WS169_TEST_GEOMETRY_270);
    record_result(report, window_matches(WS169_ROTATION_0, 239U, 279U, 0U, 20U, 239U, 299U), WS169_TEST_WINDOW_0);
    record_result(report, window_matches(WS169_ROTATION_90, 279U, 239U, 20U, 0U, 299U, 239U), WS169_TEST_WINDOW_90);
    record_result(report, window_matches(WS169_ROTATION_180, 239U, 279U, 0U, 20U, 239U, 299U), WS169_TEST_WINDOW_180);
    record_result(report, window_matches(WS169_ROTATION_270, 279U, 239U, 20U, 0U, 299U, 239U), WS169_TEST_WINDOW_270);
    record_result(report, WS169_TranslateWindow(WS169_ROTATION_90, 10U, 10U, 9U, 20U, &window) ==
                  WS169_STATUS_INVALID_ARGUMENT, WS169_TEST_REVERSED_WINDOW);
    record_result(report, WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, 280U, 239U, &window) ==
                  WS169_STATUS_INVALID_ARGUMENT, WS169_TEST_RANGE_GUARD);
    record_result(report, WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, 279U, 239U, NULL) ==
                  WS169_STATUS_INVALID_ARGUMENT, WS169_TEST_NULL_GUARD);
  }
}
