/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_logging.c
  * @brief   Logging / console output.
  *
  *          Mirrored sinks:
  *          - LPUART1 for immediate board bring-up and command responses;
  *          - USBX CDC-ACM through the board's User USB connector.
  *          USB messages are queued before enumeration. UART transmission and
  *          synchronization use bounded waits so logging cannot block forever.
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

#define LOG_UART_TIMEOUT_MS   100U
#define LOG_MUTEX_TIMEOUT_MS  100U

static TX_MUTEX log_mutex;
static volatile uint8_t log_mutex_ready = 0;

static USB_LoggingTypeDef usb_logging = {
  .tx_size = 0,
  .log_level = LOG_LEVEL_DEBUG,
  .log_count = 0
};

static ULONG log_timeout_ticks(void)
{
  ULONG ticks = (ULONG)(((uint64_t)LOG_MUTEX_TIMEOUT_MS *
                         (uint64_t)TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL);

  return (ticks == 0U) ? 1U : ticks;
}

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
 * @brief Send a formatted log message to LPUART1 and USB CDC
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
 * @brief Send raw data to LPUART1 and USB CDC
 */
int USB_Logging_SendRaw(uint8_t *data, uint16_t size)
{
  HAL_StatusTypeDef uart_status;
  unsigned queued;

  if (!data || size == 0) return -1;

  /* The mutex only exists once the kernel runs; before that there is a single context. */
  uint8_t locked = 0;
  if (log_mutex_ready && tx_thread_identify() != NULL) {
    if (tx_mutex_get(&log_mutex, log_timeout_ticks()) != TX_SUCCESS) return -1;
    locked = 1;
  }

  /* Queue to USB even before enumeration so early BLE/RTOS diagnostics survive. */
  queued = UsbCdcLog_Write(data, size);

  /* LPUART1 is full duplex: interrupt-driven RX remains active during this bounded TX. */
  uart_status = HAL_UART_Transmit(&hlpuart1, data, size, LOG_UART_TIMEOUT_MS);

  if (locked) tx_mutex_put(&log_mutex);

  return ((queued == size) || (uart_status == HAL_OK)) ? (int)size : -1;
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
