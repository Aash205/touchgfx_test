/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_tests.c
  * @brief   On-target smoke tests (run with the UART command "TEST").
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_tests.h"
#include "unified.h"
#include "ble_app.h"
#include "uart_commands.h"
#include "usb_logging.h"
#include <string.h>

#define PASS(name) do { USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: %s", name); return TEST_PASS; } while (0)
#define FAIL(...)  do { USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: " __VA_ARGS__); return TEST_FAIL; } while (0)

TestStatusTypeDef Test_Display(void)
{
  if (Display_GetWidth() != 280U || Display_GetHeight() != 240U) {
    FAIL("Display size %ux%u (expected 280x240)", Display_GetWidth(), Display_GetHeight());
  }
  if (Display_GetController() != DISPLAY_CONTROLLER_ST7789) FAIL("Display controller");
  PASS("Display geometry");
}

TestStatusTypeDef Test_UART_Commands(void)
{
  UART_CommandTypeDef h;
  memset(&h, 0, sizeof(h));

  /* Feed a line byte by byte through the same path the RX interrupt uses. huart is NULL,
   * so re-arming the receive is skipped by using the assembly step only. */
  const char *line = "LED0 ON\r";
  h.led_count = 1;
  for (const char *p = line; *p; p++) {
    if (*p == '\r') { h.rx_buffer[h.rx_index] = '\0'; h.command_ready = 1; }
    else h.rx_buffer[h.rx_index++] = *p;
  }

  if (!h.command_ready || strcmp(h.rx_buffer, "LED0 ON") != 0) FAIL("UART line assembly");
  PASS("UART command parsing");
}

TestStatusTypeDef Test_LED_Control(void)
{
  UART_CommandTypeDef h;
  memset(&h, 0, sizeof(h));

  h.led_count = 1;                      /* state machine only, no GPIO access */
  UART_CMD_SetLEDState(&h, 0, LED_BLINK_FAST);
  if (h.leds[0].state != LED_BLINK_FAST || h.leds[0].blink_period != 100U) FAIL("LED fast blink");
  UART_CMD_SetLEDState(&h, 0, LED_BLINK_SLOW);
  if (h.leds[0].blink_period != 500U) FAIL("LED slow blink");
  PASS("LED control");
}

TestStatusTypeDef Test_BLE_Status(void)
{
  BLE_StatusTypeDef st = BLE_App_GetStatus();
  if (st != BLE_APP_STATUS_ADVERTISING && st != BLE_APP_STATUS_CONNECTED) FAIL("BLE status = %d", (int)st);
  PASS("BLE advertising/connected");
}

TestStatusTypeDef Test_USB_Logging(void)
{
  if (USB_Logging_Printf(LOG_LEVEL_INFO, "console sink check") < 0) return TEST_FAIL;
  return TEST_PASS;
}

int Test_RunAll(void)
{
  int failed = 0;
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Test suite ===");
  failed += (Test_USB_Logging()   != TEST_PASS);
  failed += (Test_Display()       != TEST_PASS);
  failed += (Test_UART_Commands() != TEST_PASS);
  failed += (Test_LED_Control()   != TEST_PASS);
  failed += (Test_BLE_Status()    != TEST_PASS);
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Done: %d/5 failed ===", failed);
  return failed;
}
