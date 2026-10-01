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
#include "timeouts.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef hlpuart1;

#define LOG_UART_TIMEOUT_MS   100U
#define LOG_MUTEX_TIMEOUT_MS  100U

static TX_MUTEX log_mutex;
static volatile uint8_t log_mutex_ready = 0;
static CHAR log_mutex_name[] = {'L', 'o', 'g', ' ', 'm', 'u', 't', 'e', 'x', '\0'};

static USB_LoggingTypeDef usb_logging = {
  .tx_size = 0,
  .log_level = LOG_LEVEL_DEBUG,
  .log_count = 0
};

/* Converted once in USB_Logging_Init, before log_mutex_ready is set. */
static ULONG log_mutex_ticks;

/**
 * @brief Initialize USB logging
 */
int USB_Logging_Init(void)
{
  int status = 0;

  usb_logging.tx_size = 0;
  usb_logging.log_count = 0;
  
  if (log_mutex_ready == 0U) {
    log_mutex_ticks = (ULONG)Timeout_MsToTicks(LOG_MUTEX_TIMEOUT_MS, TX_TIMER_TICKS_PER_SECOND);
    if (tx_mutex_create(&log_mutex, log_mutex_name, TX_INHERIT) != TX_SUCCESS)
    {
      status = -1;
    }
    else
    {
      log_mutex_ready = 1U;
    }
  }

  return status;
}

/**
 * @brief Send a formatted log message to LPUART1 and USB CDC
 */
int USB_Logging_Printf(LogLevelTypeDef level, const char *format, ...)
{
  va_list args;
  char log_buffer[256];
  int written;
  int result = -1;
  
  if (!LogFormat_ShouldLog(level, usb_logging.log_level))
  {
    result = 0;
  }
  else if (format == NULL)
  {
    result = -1;
  }
  else
  {
    va_start(args, format);
    written = vsnprintf(log_buffer, sizeof(log_buffer) - 20U, format, args);
    va_end(args);

    if (written > 0)
    {
      char formatted[256];
      size_t transmitted = LogFormat_Line(formatted, sizeof(formatted), level, HAL_GetTick(),
                                          log_buffer);

      if (transmitted > 0U)
      {
        (void)USB_Logging_SendRaw((const uint8_t *)formatted, (uint16_t)transmitted);
        usb_logging.log_count++;
        result = (int)transmitted;
      }
    }
  }

  return result;
}

/**
 * @brief Send raw data to LPUART1 and USB CDC
 */
int USB_Logging_SendRaw(const uint8_t *data, uint16_t size)
{
  HAL_StatusTypeDef uart_status;
  UINT lock_status = TX_SUCCESS;
  UINT unlock_status = TX_SUCCESS;
  unsigned queued;
  uint8_t locked = 0U;
  uint8_t can_send = 0U;

  int result = -1;

  if ((data != NULL) && (size != 0U))
  {
    /* The mutex only exists once the kernel runs; before that there is a single context. */
    if ((log_mutex_ready != 0U) && (tx_thread_identify() != NULL))
    {
      lock_status = tx_mutex_get(&log_mutex, log_mutex_ticks);
      if (lock_status == TX_SUCCESS)
      {
        locked = 1U;
        can_send = 1U;
      }
    }
    else
    {
      can_send = 1U;
    }

    if (can_send != 0U)
    {
      queued = UsbCdcLog_Write(data, (unsigned)size);
      uart_status = HAL_UART_Transmit(&hlpuart1, (uint8_t *)(uintptr_t)data,
                                      size, LOG_UART_TIMEOUT_MS);
      result = ((queued == (unsigned)size) || (uart_status == HAL_OK)) ? (int)size : -1;

      if (locked != 0U)
      {
        unlock_status = tx_mutex_put(&log_mutex);
        if (unlock_status != TX_SUCCESS)
        {
          result = -1;
        }
      }
    }
  }
  return result;
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
