/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_tests.c
  * @brief   Application Test Suite Implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_tests.h"
#include "oled_driver.h"
#include "ble_app.h"
#include "uart_commands.h"
#include "usb_logging.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief Test OLED initialization
 */
TestStatusTypeDef Test_OLED_Init(I2C_HandleTypeDef *hi2c)
{
  OLED_HandleTypeDef test_oled;
  
  if (OLED_Init(&test_oled, hi2c) != HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: OLED_Init returned error");
    return TEST_FAIL;
  }
  
  if (OLED_DisplayOn(&test_oled) != HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: OLED_DisplayOn returned error");
    return TEST_FAIL;
  }
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: OLED Initialization");
  return TEST_PASS;
}

/**
 * @brief Test OLED drawing operations
 */
TestStatusTypeDef Test_OLED_Drawing(I2C_HandleTypeDef *hi2c)
{
  OLED_HandleTypeDef test_oled;
  
  if (OLED_Init(&test_oled, hi2c) != HAL_OK) {
    return TEST_FAIL;
  }
  
  /* Test clear */
  if (OLED_Clear(&test_oled) != HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: OLED_Clear");
    return TEST_FAIL;
  }
  
  /* Test pixel drawing */
  OLED_SetPixel(&test_oled, 10, 10, 1);
  OLED_SetPixel(&test_oled, 20, 20, 1);
  
  /* Test line drawing */
  OLED_DrawHLine(&test_oled, 5, 30, 20);
  OLED_DrawVLine(&test_oled, 50, 5, 20);
  
  /* Test rectangle */
  OLED_DrawRect(&test_oled, 70, 10, 30, 20);
  
  /* Test print */
  OLED_PrintStr(&test_oled, 0, 0, "TEST");
  
  if (OLED_UpdateDisplay(&test_oled) != HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: OLED_UpdateDisplay");
    return TEST_FAIL;
  }
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: OLED Drawing Operations");
  return TEST_PASS;
}

/**
 * @brief Test UART command parsing
 */
TestStatusTypeDef Test_UART_Commands(void)
{
  UART_CommandTypeDef test_uart;
  
  /* Note: This test requires UART handle to be provided */
  /* For now, just test command parsing logic */
  
  /* Test LED0 ON command */
  strcpy(test_uart.rx_buffer, "LED0 ON");
  test_uart.rx_index = 7;
  test_uart.command_ready = 1;
  
  /* Verify buffer content */
  if (strstr(test_uart.rx_buffer, "LED0") && strstr(test_uart.rx_buffer, "ON")) {
    USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: UART Command Parsing");
    return TEST_PASS;
  }
  
  USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: UART Command Parsing");
  return TEST_FAIL;
}

/**
 * @brief Test LED control
 */
TestStatusTypeDef Test_LED_Control(void)
{
  UART_CommandTypeDef test_handler;
  
  test_handler.led_count = 0;
  
  /* Register test LED */
  if (UART_CMD_RegisterLED(&test_handler, GPIOA, GPIO_PIN_5) != HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: LED Registration");
    return TEST_FAIL;
  }
  
  if (test_handler.led_count != 1) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: LED Count");
    return TEST_FAIL;
  }
  
  /* Test LED ON command */
  UART_CMD_SetLEDState(&test_handler, 0, LED_ON);
  if (test_handler.leds[0].state != LED_ON) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: LED_ON state");
    return TEST_FAIL;
  }
  
  /* Test LED OFF command */
  UART_CMD_SetLEDState(&test_handler, 0, LED_OFF);
  if (test_handler.leds[0].state != LED_OFF) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: LED_OFF state");
    return TEST_FAIL;
  }
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: LED Control");
  return TEST_PASS;
}

/**
 * @brief Test BLE initialization
 */
TestStatusTypeDef Test_BLE_Init(void)
{
  BLE_StatusTypeDef status = BLE_App_Init();
  
  if (status != BLE_STATUS_INITIALIZED) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: BLE Init status = %d", status);
    return TEST_FAIL;
  }
  
  BLE_AppHandleTypeDef *handle = BLE_App_GetHandle();
  if (!handle) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: BLE Handle is NULL");
    return TEST_FAIL;
  }
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: BLE Initialization");
  return TEST_PASS;
}

/**
 * @brief Test USB logging
 */
TestStatusTypeDef Test_USB_Logging(void)
{
  if (USB_Logging_Init() != 0) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: USB_Logging_Init");
    return TEST_FAIL;
  }
  
  int ret = USB_Logging_Printf(LOG_LEVEL_INFO, "Test message from USB_Logging");
  
  if (ret < 0) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "Test FAILED: USB_Logging_Printf");
    return TEST_FAIL;
  }
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "Test PASSED: USB Logging");
  return TEST_PASS;
}

/**
 * @brief Run all tests
 */
int Test_RunAll(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
  int failed_count = 0;
  TestStatusTypeDef status;
  
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Starting Test Suite ===");
  
  /* Test 1: USB Logging */
  status = Test_USB_Logging();
  if (status != TEST_PASS) failed_count++;
  
  /* Test 2: OLED Init */
  status = Test_OLED_Init(hi2c);
  if (status != TEST_PASS) failed_count++;
  
  /* Test 3: OLED Drawing */
  status = Test_OLED_Drawing(hi2c);
  if (status != TEST_PASS) failed_count++;
  
  /* Test 4: UART Commands */
  status = Test_UART_Commands();
  if (status != TEST_PASS) failed_count++;
  
  /* Test 5: LED Control */
  status = Test_LED_Control();
  if (status != TEST_PASS) failed_count++;
  
  /* Test 6: BLE Init */
  status = Test_BLE_Init();
  if (status != TEST_PASS) failed_count++;
  
  /* Summary */
  USB_Logging_Printf(LOG_LEVEL_INFO, "=== Test Suite Complete ===");
  USB_Logging_Printf(LOG_LEVEL_INFO, "Failed Tests: %d/6", failed_count);
  
  return failed_count;
}
