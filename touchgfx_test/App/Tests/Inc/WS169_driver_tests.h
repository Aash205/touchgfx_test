/**
  ******************************************************************************
  * @file    WS169_driver_tests.h
  * @brief   Deterministic on-target tests for WS169 driver geometry and guards.
  ******************************************************************************
  */

#ifndef WS169_DRIVER_TESTS_H
#define WS169_DRIVER_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    WS169_TEST_PANEL_CONSTANTS = (1UL << 0),
    WS169_TEST_GEOMETRY_0      = (1UL << 1),
    WS169_TEST_GEOMETRY_90     = (1UL << 2),
    WS169_TEST_GEOMETRY_180    = (1UL << 3),
    WS169_TEST_GEOMETRY_270    = (1UL << 4),
    WS169_TEST_WINDOW_0        = (1UL << 5),
    WS169_TEST_WINDOW_90       = (1UL << 6),
    WS169_TEST_WINDOW_180      = (1UL << 7),
    WS169_TEST_WINDOW_270      = (1UL << 8),
    WS169_TEST_REVERSED_WINDOW = (1UL << 9),
    WS169_TEST_RANGE_GUARD     = (1UL << 10),
    WS169_TEST_NULL_GUARD      = (1UL << 11)
} WS169_TestFailure_t;

typedef struct
{
    uint16_t executed;
    uint16_t passed;
    uint16_t failed;
    uint32_t failure_mask;
} WS169_TestReport_t;

/** Run 12 deterministic tests; failure_mask contains WS169_TestFailure_t bits. */
void WS169_DriverTests_Run(WS169_TestReport_t *report);

#ifdef __cplusplus
}
#endif

#endif /* WS169_DRIVER_TESTS_H */
