/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_tests.h
  * @brief   On-target smoke tests (run with the UART command "TEST").
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __APP_TESTS_H__
#define __APP_TESTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"

typedef enum {
  TEST_PASS = 0,
  TEST_FAIL = 1,
  TEST_SKIP = 2
} TestStatusTypeDef;

TestStatusTypeDef Test_Display(void);        /* driver geometry / rotation constants */
TestStatusTypeDef Test_UART_Commands(void);  /* RX line assembly + command parsing */
TestStatusTypeDef Test_LED_Control(void);    /* LED state machine */
TestStatusTypeDef Test_BLE_Status(void);     /* BLE stack came up and is advertising/connected */
TestStatusTypeDef Test_USB_Logging(void);    /* console sink accepts data */

/** Run everything; returns the number of failed tests. */
int Test_RunAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TESTS_H__ */
