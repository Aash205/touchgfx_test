/**
  ******************************************************************************
  * @file    app_state.h
  * @brief   Thread-safe snapshot of application state shared between the RTOS
  *          threads (BLE, UART console, monitor) and the TouchGFX GUI.
  *          Plain C so both C and C++ (Model) can include it.
  ******************************************************************************
  */
#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define APP_LED_COUNT 2U   /* LED0 = LD1 (PC7), LED1 = LD3 (PB14) */

/* Mirrors BLE_StatusTypeDef in ble_app.h (0 idle .. 6 error) */
typedef struct {
  uint8_t  led[APP_LED_COUNT];  /* 1 = on (blinking counts as on), 0 = off */
  uint8_t  ble_status;          /* BLE_StatusTypeDef value */
  uint32_t uptime_s;            /* seconds since boot */
  uint32_t heartbeat;           /* monitor heartbeat counter (5 s period) */
  uint16_t fps;                 /* display flushes in the last second */
} AppState;

/** Copy the current state (safe from any thread / the GUI). */
void AppState_Get(AppState *out);

/** Control LEDs from any source (GUI, BLE, button, UART). idx < APP_LED_COUNT. */
void AppState_SetLed(uint8_t idx, uint8_t on);
void AppState_ToggleLed(uint8_t idx);

/** Called by TouchGFXHAL after every panel flush; feeds the FPS figure. */
void AppState_FrameFlushed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
