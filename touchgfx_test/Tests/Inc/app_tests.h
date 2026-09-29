/**
  * @file    app_tests.h
  * @brief   Deterministic board bring-up tests and test-runner status.
  */
#ifndef APP_TESTS_H
#define APP_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
  TEST_PASS = 0,
  TEST_FAIL = 1,
  TEST_SKIP = 2
} TestStatusTypeDef;

typedef enum {
  TEST_CASE_USB_LOGGING = 1,
  TEST_CASE_DISPLAY = 2,
  TEST_CASE_UART = 3,
  TEST_CASE_LED = 4,
  TEST_CASE_BLE = 5,
  TEST_CASE_COUNT = 5
} TestCaseIdTypeDef;

TestStatusTypeDef Test_Display(void);
TestStatusTypeDef Test_UART_Commands(void);
TestStatusTypeDef Test_LED_Control(void);
TestStatusTypeDef Test_BLE_Status(void);
TestStatusTypeDef Test_USB_Logging(void);

/** Run all five cases; returns the number of failed cases. */
int Test_RunAll(void);

/** Run one case (1..5); returns TEST_* for a valid case, TEST_FAIL otherwise. */
TestStatusTypeDef Test_RunOne(uint8_t case_id);

/** Snapshot the latest run for UART and TouchGFX presentation. */
void Test_GetSummary(uint8_t *last_case,
                     uint8_t *last_status,
                     uint16_t *total,
                     uint16_t *passed,
                     uint16_t *failed);

#ifdef __cplusplus
}
#endif

#endif /* APP_TESTS_H */
