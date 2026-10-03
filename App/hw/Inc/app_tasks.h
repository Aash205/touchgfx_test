/**
 ******************************************************************************
 * @file    app_tasks.h
 * @brief   Creates the application's ThreadX timer and threads.
 ******************************************************************************
 */

#ifndef APP_TASKS_H
#define APP_TASKS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tx_api.h"

/* Defined in TouchGFX/target/TouchGFXHAL.cpp: releases the TouchGFX frame wait. */
void touchgfxSignalVSync(void);

/**
 * @brief Bring up the display RTOS objects, logging, USB CDC log sink, the TouchGFX VSYNC
 *        timer, the console/LED service and the BLE, UART and monitor threads.
 * @retval TX_SUCCESS or the first ThreadX error code (TX_NOT_DONE if the display RTOS init fails)
 */
UINT AppTasks_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
