/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_logging.c
  * @brief   Logging / console output.
  *
  *          Primary sink: USBX CDC-ACM through the board's User USB connector.
  *          Boot messages are queued before enumeration and drained after the host
  *          opens the virtual COM port. LPUART remains configured for the legacy
  *          command receiver, but logging no longer depends on ST-LINK VCP routing.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usb_logging.h"
#include "tx_api.h"
#include "usb_cdc_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef hlpuart1;

static TX_MUTEX log_mutex;
static volatile uint8_t log_mutex_ready = 0;

static USB_LoggingTypeDef usb_logging = {
  .tx_size = 0,
  .log_level = LOG_LEVEL_DEBUG,
  .log_count = 0
};

/**
 * @brief Initialize USB logging
 */
int USB_Logging_Init(void)
{
  usb_logging.tx_size = 0;
  usb_logging.log_count = 0;
  
  if (!log_mutex_ready) {
    if (tx_mutex_create(&log_mutex, (CHAR *)"Log mutex", TX_INHERIT) != TX_SUCCESS) return -1;
    log_mutex_ready = 1;
  }

  return 0;
}

/**
 * @brief Send log message via USB
 */
int USB_Logging_Printf(LogLevelTypeDef level, const char *format, ...)
{
  va_list args;
  char log_buffer[256];
  char level_str[16];
  int written;
  
  if (level < usb_logging.log_level) return 0;
  
  switch (level) {
    case LOG_LEVEL_DEBUG: strcpy(level_str, "[DEBUG]"); break;
    case LOG_LEVEL_INFO: strcpy(level_str, "[INFO]"); break;
    case LOG_LEVEL_WARNING: strcpy(level_str, "[WARN]"); break;
    case LOG_LEVEL_ERROR: strcpy(level_str, "[ERROR]"); break;
    case LOG_LEVEL_CRITICAL: strcpy(level_str, "[CRIT]"); break;
    default: strcpy(level_str, "[?]"); break;
  }
  
  va_start(args, format);
  written = vsnprintf(log_buffer, sizeof(log_buffer) - 20, format, args);
  va_end(args);
  
  if (written > 0) {
    /* Format: [LEVEL] timestamp: message\r\n */
    char formatted[256];
    uint32_t ms = HAL_GetTick();
    int len = snprintf(formatted, sizeof(formatted), "%s [%04lu.%03lu] %s\r\n", 
                      level_str, ms/1000, ms%1000, log_buffer);
    
    if (len > 0) {
      USB_Logging_SendRaw((uint8_t *)formatted, len);
      usb_logging.log_count++;
    }
    return len;
  }
  
  return -1;
}

/**
 * @brief Send raw data via USB
 */
int USB_Logging_SendRaw(uint8_t *data, uint16_t size)
{
  if (!data || size == 0) return -1;

  /* The mutex only exists once the kernel runs; before that there is a single context. */
  uint8_t locked = 0;
  if (log_mutex_ready && tx_thread_identify() != NULL) {
    if (tx_mutex_get(&log_mutex, TX_WAIT_FOREVER) != TX_SUCCESS) return -1;
    locked = 1;
  }

  /* Queue to USB even before enumeration so early BLE/RTOS diagnostics survive. */
  unsigned queued = UsbCdcLog_Write(data, size);

  if (locked) tx_mutex_put(&log_mutex);

  return (queued == size) ? (int)size : -1;
}

/**
 * @brief Log system status
 */
int USB_Logging_LogStatus(const char *status_str)
{
  return USB_Logging_Printf(LOG_LEVEL_INFO, "STATUS: %s", status_str);
}

/**
 * @brief Flush log buffer
 */
int USB_Logging_Flush(void)
{
  return 0; /* UART sink is unbuffered */
}
