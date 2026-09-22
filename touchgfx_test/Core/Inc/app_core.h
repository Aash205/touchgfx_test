/**
  ******************************************************************************
  * @file    app_core.h
  * @brief   Application core (C only): owns the LED / console handler and the
  *          shared AppState (app_state.h) that the GUI and BLE read.
  ******************************************************************************
  */
#ifndef APP_CORE_H
#define APP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_state.h"
#include "stm32l4xx_hal.h"

/** Register LEDs, start console reception, configure the user button (PC13). */
void AppCore_Init(UART_HandleTypeDef *console_uart);

/** Console line processing + LED blinking + user-button poll. Call every ~20 ms. */
void AppCore_Process(void);

/** Forward HAL_UART_RxCpltCallback / ErrorCallback here. */
void AppCore_UartRxCplt(UART_HandleTypeDef *huart);
void AppCore_UartError(UART_HandleTypeDef *huart);

/** Once per second from the monitor thread: refreshes the FPS figure. */
void AppCore_Tick1s(void);

/** Bumps the heartbeat counter (monitor thread, every 5 s). Returns the new value. */
uint32_t AppCore_Heartbeat(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CORE_H */
