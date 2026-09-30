/**
  * @file    app_tests.c
  * @brief   On-target board bring-up tests. Run with TEST or TEST <1..5>.
  */
#include "app_tests.h"
#include "app_state.h"
#include "waveshare_driver.h"
#include "WS169_driver_tests.h"
#include "ble_app.h"
#include "uart_commands.h"
#include "usb_logging.h"
#include <string.h>

#define PASS(name) do { USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: %s", name); return TEST_PASS; } while (0)
#define FAIL(...)  do { USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: " __VA_ARGS__); return TEST_FAIL; } while (0)

static volatile uint8_t s_last_case;
static volatile uint8_t s_last_status = APP_TEST_IDLE;
static volatile uint16_t s_total;
static volatile uint16_t s_passed;
static volatile uint16_t s_failed;

TestStatusTypeDef Test_Display(void)
{
  WS169_TestReport_t report;
  WS169_Diagnostics_t diagnostics;
  WS169_DriverTests_Run(&report);
  if (report.failed != 0U) {
    FAIL("WS169 unit tests: %u/%u failed, mask=0x%08lx",
         (unsigned)report.failed, (unsigned)report.executed,
         (unsigned long)report.failure_mask);
  }
  if (WS169_GetWidth() != WS169_LANDSCAPE_WIDTH ||
      WS169_GetHeight() != WS169_LANDSCAPE_HEIGHT) {
    FAIL("Display size %ux%u (expected %ux%u)", WS169_GetWidth(), WS169_GetHeight(),
         WS169_LANDSCAPE_WIDTH, WS169_LANDSCAPE_HEIGHT);
  }
  WS169_GetDiagnostics(&diagnostics);
  if (!diagnostics.initialized || !diagnostics.rtos_ready) {
    FAIL("WS169 state initialized=%u rtos=%u", diagnostics.initialized ? 1U : 0U,
         diagnostics.rtos_ready ? 1U : 0U);
  }
  if ((diagnostics.spi_error_count != 0U) || (diagnostics.dma_error_count != 0U) ||
      (diagnostics.timeout_count != 0U)) {
    FAIL("WS169 faults spi=%lu dma=%lu timeout=%lu", (unsigned long)diagnostics.spi_error_count,
         (unsigned long)diagnostics.dma_error_count, (unsigned long)diagnostics.timeout_count);
  }
  USB_Logging_Printf(LOG_LEVEL_INFO, "WS169 unit tests: %u/%u passed",
                     (unsigned)report.passed, (unsigned)report.executed);
  PASS("Display driver and geometry");
}

TestStatusTypeDef Test_UART_Commands(void)
{
  UART_CommandTypeDef h;
  const char *line = "LED0 ON\r";
  memset(&h, 0, sizeof(h));
  h.led_count = 1U;
  for (const char *p = line; *p != '\0'; p++) {
    if (*p == '\r') { h.rx_buffer[h.rx_index] = '\0'; h.command_ready = 1U; }
    else { h.rx_buffer[h.rx_index++] = *p; }
  }
  if (!h.command_ready || strcmp(h.rx_buffer, "LED0 ON") != 0) { FAIL("UART line assembly"); }
  PASS("UART command parsing");
}

TestStatusTypeDef Test_LED_Control(void)
{
  UART_CommandTypeDef h;
  memset(&h, 0, sizeof(h));
  h.led_count = 1U;
  UART_CMD_SetLEDState(&h, 0U, LED_BLINK_FAST);
  if (h.leds[0].state != LED_BLINK_FAST || h.leds[0].blink_period != 100U) { FAIL("LED fast blink"); }
  UART_CMD_SetLEDState(&h, 0U, LED_BLINK_SLOW);
  if (h.leds[0].blink_period != 500U) { FAIL("LED slow blink"); }
  PASS("LED control");
}

TestStatusTypeDef Test_BLE_Status(void)
{
  BLE_StatusTypeDef st = BLE_App_GetStatus();
  if ((st != BLE_APP_STATUS_INITIALIZED) && (st != BLE_APP_STATUS_ADVERTISING) &&
      (st != BLE_APP_STATUS_CONNECTED)) { FAIL("BLE status = %d", (int)st); }
  PASS("BLE ready/advertising/connected");
}

TestStatusTypeDef Test_USB_Logging(void)
{
  if (USB_Logging_Printf(LOG_LEVEL_INFO, "console sink check") < 0) { return TEST_FAIL; }
  PASS("USB/UART logging");
}

TestStatusTypeDef Test_RunOne(uint8_t case_id)
{
  TestStatusTypeDef result = TEST_FAIL;
  s_last_case = case_id;
  switch (case_id) {
    case TEST_CASE_USB_LOGGING: result = Test_USB_Logging(); break;
    case TEST_CASE_DISPLAY: result = Test_Display(); break;
    case TEST_CASE_UART: result = Test_UART_Commands(); break;
    case TEST_CASE_LED: result = Test_LED_Control(); break;
    case TEST_CASE_BLE: result = Test_BLE_Status(); break;
    default:
      USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: invalid test case %u", (unsigned)case_id);
      s_last_status = APP_TEST_FAIL;
      return TEST_FAIL;
  }
  s_total = 1U;
  s_passed = (result == TEST_PASS) ? 1U : 0U;
  s_failed = (result == TEST_FAIL) ? 1U : 0U;
  s_last_status = (result == TEST_PASS) ? APP_TEST_PASS : APP_TEST_FAIL;
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test case %u: %s", (unsigned)case_id,
                     (result == TEST_PASS) ? "PASS" : "FAIL");
  return result;
}

int Test_RunAll(void)
{
  int failed = 0;
  uint16_t total = 0U;
  uint16_t passed = 0U;
  uint16_t failed_count = 0U;
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Board bring-up test suite: 5 cases ===");
  for (uint8_t case_id = 1U; case_id <= TEST_CASE_COUNT; case_id++) {
    TestStatusTypeDef result = Test_RunOne(case_id);
    total++;
    if (result == TEST_PASS) { passed++; } else { failed_count++; failed++; }
  }
  s_total = total;
  s_passed = passed;
  s_failed = failed_count;
  s_last_status = (failed_count == 0U) ? APP_TEST_PASS : APP_TEST_FAIL;
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Done: %u/%u passed, %u failed ===",
                     (unsigned)passed, (unsigned)total, (unsigned)failed_count);
  return failed;
}

void Test_GetSummary(uint8_t *last_case, uint8_t *last_status, uint16_t *total,
                     uint16_t *passed, uint16_t *failed)
{
  *last_case = s_last_case;
  *last_status = s_last_status;
  *total = s_total;
  *passed = s_passed;
  *failed = s_failed;
}
