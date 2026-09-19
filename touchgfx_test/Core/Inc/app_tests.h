/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_tests.h
  * @brief   Application Test Suite Header
  *          Unit tests for demo components
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __APP_TESTS_H__
#define __APP_TESTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Test Status Codes ----------------------------------------------------------*/
typedef enum {
  TEST_PASS = 0,
  TEST_FAIL = 1,
  TEST_SKIP = 2
} TestStatusTypeDef;

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief Test OLED initialization and basic operations
 * @param hi2c: I2C handle
 * @retval Test status
 */
TestStatusTypeDef Test_OLED_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Test OLED display and drawing
 * @param hi2c: I2C handle
 * @retval Test status
 */
TestStatusTypeDef Test_OLED_Drawing(I2C_HandleTypeDef *hi2c);

/**
 * @brief Test UART command parsing
 * @retval Test status
 */
TestStatusTypeDef Test_UART_Commands(void);

/**
 * @brief Test LED control
 * @retval Test status
 */
TestStatusTypeDef Test_LED_Control(void);

/**
 * @brief Test BLE initialization
 * @retval Test status
 */
TestStatusTypeDef Test_BLE_Init(void);

/**
 * @brief Test USB logging
 * @retval Test status
 */
TestStatusTypeDef Test_USB_Logging(void);

/**
 * @brief Run all tests
 * @param hi2c: I2C handle
 * @param huart: UART handle
 * @retval Number of failed tests
 */
int Test_RunAll(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TESTS_H__ */
