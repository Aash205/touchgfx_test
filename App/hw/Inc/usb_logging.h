/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_logging.h
  * @brief   USB Logging Header
  *          USBX-based logging for diagnostics
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef USB_LOGGING_H
#define USB_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Log Level Constants: LogLevelTypeDef and LOG_LEVEL_* live in App/logic/log_format.h -----*/
#include "log_format.h"

/* Function Prototypes -------------------------------------------------------*/
/**
 * @brief Initialize USB logging
 * @retval Status
 */
int USB_Logging_Init(void);

/**
 * @brief Send a formatted log message to LPUART1 and USB CDC
 * @param level: Log level
 * @param format: Format string (printf style)
 * @retval Status
 */
int USB_Logging_Printf(LogLevelTypeDef level, const char *format, ...);

/**
 * @brief Send raw data to LPUART1 and USB CDC
 * @param data: Data buffer
 * @param size: Data size
 * @retval Status
 */
int USB_Logging_SendRaw(const uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* USB_LOGGING_H */
