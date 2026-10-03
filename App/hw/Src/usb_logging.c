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
#include "app_board.h"
#include "timeouts.h"
#include "tx_api.h"
#include "usb_cdc_log.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
#define LOG_UART_TIMEOUT_MS 100U
#define LOG_MUTEX_TIMEOUT_MS 100U

/* Every level is logged. */
#define LOG_THRESHOLD LOG_LEVEL_DEBUG

static TX_MUTEX log_mutex;
static volatile uint8_t log_mutex_ready = 0;

/* Converted once in USB_Logging_Init, before log_mutex_ready is set. */
static ULONG log_mutex_ticks;

/**
 * @brief Initialize USB logging
 */
int USB_Logging_Init(void)
{
    int status = 0;

    if (log_mutex_ready == 0U)
    {
        static CHAR log_mutex_name[] = {'L', 'o', 'g', ' ', 'm', 'u', 't', 'e', 'x', '\0'};

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
int USB_Logging_Printf(LogLevelTypeDef level, const char* format, ...)
{
    int result = -1;

    if (!LogFormat_ShouldLog(level, LOG_THRESHOLD))
    {
        result = 0;
    }
    else if (format != NULL)
    {
        char log_buffer[256];
        int written;
        /* cppcheck-suppress misra-c2012-17.1 -- a printf-style logging API is the point of this
         * function */
        va_list args;

        /* cppcheck-suppress misra-c2012-17.1 -- see above */
        va_start(args, format);
        /* cppcheck-suppress misra-c2012-21.6 -- vsnprintf formats the message into a bounded buffer
         */
        written = vsnprintf(log_buffer, sizeof(log_buffer) - 20U, format, args);
        /* cppcheck-suppress misra-c2012-17.1 -- see above */
        va_end(args);

        if (written > 0)
        {
            char formatted[256];
            size_t transmitted =
                LogFormat_Line(formatted, sizeof(formatted), level, HAL_GetTick(), log_buffer);

            if (transmitted > 0U)
            {
                (void)USB_Logging_SendRaw((const uint8_t*)formatted, (uint16_t)transmitted);
                result = (int)transmitted;
            }
        }
    }
    else
    {
        /* No format string: nothing to log. */
    }

    return result;
}

/**
 * @brief Send raw data to LPUART1 and USB CDC
 */
int USB_Logging_SendRaw(const uint8_t* data, uint16_t size)
{
    int result = -1;

    if ((data != NULL) && (size != 0U))
    {
        uint8_t locked = 0U;
        uint8_t can_send = 0U;

        /* The mutex only exists once the kernel runs; before that there is a single context. */
        if ((log_mutex_ready != 0U) && (tx_thread_identify() != NULL))
        {
            if (tx_mutex_get(&log_mutex, log_mutex_ticks) == TX_SUCCESS)
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
            const unsigned queued = UsbCdcLog_Write(data, (unsigned)size);
            const HAL_StatusTypeDef uart_status =
                /* cppcheck-suppress misra-c2012-11.4 -- HAL takes a non-const buffer, only reads */
                HAL_UART_Transmit(&hlpuart1, (uint8_t*)(uintptr_t)data, size, LOG_UART_TIMEOUT_MS);
            result = ((queued == (unsigned)size) || (uart_status == HAL_OK)) ? (int)size : -1;

            if (locked != 0U)
            {
                if (tx_mutex_put(&log_mutex) != TX_SUCCESS)
                {
                    result = -1;
                }
            }
        }
    }
    return result;
}
