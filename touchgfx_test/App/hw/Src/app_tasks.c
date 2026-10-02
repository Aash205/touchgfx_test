/**
 ******************************************************************************
 * @file    app_tasks.c
 * @brief   The application's ThreadX objects: VSYNC timer, BLE, UART console and
 *          monitor threads. Called from the single USER CODE line of
 *          App_ThreadX_Init() in Core/Src/app_threadx.c.
 ******************************************************************************
 */

#include "app_tasks.h"

#include "app_board.h"
#include "app_core.h"
#include "ble_app.h"
#include "health_format.h"
#include "main.h"
#include "usb_cdc_log.h"
#include "usb_logging.h"
#include "waveshare_driver.h"

#define THREAD_STACK_BYTES_SMALL 3072U
#define THREAD_STACK_BYTES_BLE 4096U

/* Lower number = higher priority. The TouchGFX thread (priority 5) stays above these. */
#define PRIO_BLE 10U
#define PRIO_UART 11U
#define PRIO_MONITOR 12U

/* The SPI panel has no VSYNC: a periodic timer paces TouchGFX (2 ticks = 20 ms @ 100 Hz). */
#define TOUCHGFX_VSYNC_TICKS 2U

static void vsync_timer_cb(ULONG input);
static void thread_ble_entry(ULONG input);
static void thread_uart_cmd_entry(ULONG input);
static void thread_monitor_entry(ULONG input);

UINT AppTasks_Init(void)
{
    /* ThreadX keeps pointers to these for the life of the program, so they are static. */
    static TX_TIMER vsync_timer;
    static TX_THREAD thread_ble;
    static TX_THREAD thread_uart_cmd;
    static TX_THREAD thread_monitor;
    static ULONG thread_ble_stack[THREAD_STACK_BYTES_BLE / sizeof(ULONG)];
    static ULONG thread_uart_cmd_stack[THREAD_STACK_BYTES_SMALL / sizeof(ULONG)];
    static ULONG thread_monitor_stack[THREAD_STACK_BYTES_SMALL / sizeof(ULONG)];

    UINT ret = TX_SUCCESS;

    /* Display: from now on pixel DMA yields to other threads instead of spinning. */
    if (WS169_RtosInit() != WS169_STATUS_OK)
    {
        ret = TX_NOT_DONE;
    }

    /* Console/logging mutex. Log output is sent through USBX CDC-ACM. */
    if (ret == TX_SUCCESS)
    {
        (void)USB_Logging_Init();

        /* USB CDC log drain thread; boot logs queue until the host enumerates it. */
        ret = UsbCdcLog_Init();
    }

    /* TouchGFX frame pacing. */
    if (ret == TX_SUCCESS)
    {
        /* cppcheck-suppress [misra-c2012-7.4, misra-c2012-11.8] -- ThreadX takes a non-const name
         */
        ret = tx_timer_create(&vsync_timer, "TouchGFX VSync", vsync_timer_cb, 0,
                              TOUCHGFX_VSYNC_TICKS, TOUCHGFX_VSYNC_TICKS, TX_AUTO_ACTIVATE);
    }

    if (ret == TX_SUCCESS)
    {
        /* LEDs (LD1 = PC7, LD3 = PB14), UART command console, user button, shared AppState. */
        AppCore_Init(&hlpuart1);

        /* cppcheck-suppress [misra-c2012-7.4, misra-c2012-11.8] -- ThreadX takes a non-const name
         */
        ret = tx_thread_create(&thread_ble, "BLE", thread_ble_entry, 0, thread_ble_stack,
                               sizeof(thread_ble_stack), PRIO_BLE, PRIO_BLE, TX_NO_TIME_SLICE,
                               TX_AUTO_START);
    }

    if (ret == TX_SUCCESS)
    {
        /* cppcheck-suppress [misra-c2012-7.4, misra-c2012-11.8] -- ThreadX takes a non-const name
         */
        ret = tx_thread_create(&thread_uart_cmd, "UART cmd", thread_uart_cmd_entry, 0,
                               thread_uart_cmd_stack, sizeof(thread_uart_cmd_stack), PRIO_UART,
                               PRIO_UART, TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    if (ret == TX_SUCCESS)
    {
        /* cppcheck-suppress [misra-c2012-7.4, misra-c2012-11.8] -- ThreadX takes a non-const name
         */
        ret = tx_thread_create(&thread_monitor, "Monitor", thread_monitor_entry, 0,
                               thread_monitor_stack, sizeof(thread_monitor_stack), PRIO_MONITOR,
                               PRIO_MONITOR, TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    return ret;
}

static void vsync_timer_cb(ULONG input)
{
    (void)input;
    touchgfxSignalVSync();
}

/* BlueNRG-2: bring the stack up, advertise, then service HCI events. */
static void thread_ble_entry(ULONG input)
{
    (void)input;

    if ((BLE_App_Init() == BLE_APP_STATUS_INITIALIZED) &&
        (BLE_App_StartAdvertising("Nucleo-BLE-Demo") == BLE_APP_STATUS_ADVERTISING))
    {
        (void)USB_Logging_Printf(LOG_LEVEL_INFO, "BLE advertising as %s",
                                 (char*)BLE_App_GetHandle()->device_name);
    }
    else
    {
        (void)USB_Logging_Printf(LOG_LEVEL_ERROR,
                                 "BLE bring-up failed (is the BlueNRG-2 shield fitted?)");
    }

    for (;;)
    {
        BLE_App_Process();
        (void)tx_thread_sleep(1); /* 10 ms */
    }
}

/* LPUART1 command console + LED blink service. */
static void thread_uart_cmd_entry(ULONG input)
{
    (void)input;

    (void)USB_Logging_Printf(LOG_LEVEL_INFO, "Console ready. Type HELP.");

    for (;;)
    {
        AppCore_Process();
        (void)tx_thread_sleep(2); /* 20 ms */
    }
}

/* 1 s tick (FPS window); heartbeat log every 5 s. */
static void thread_monitor_entry(ULONG input)
{
    (void)input;
    uint32_t seconds = 0;

    for (;;)
    {
        (void)tx_thread_sleep(100); /* 1 s */
        AppCore_Tick1s();

        seconds++;
        if ((seconds % 5U) == 0U)
        {
            AppState st;
            WS169_Diagnostics_t display_diagnostics;
            static char
                health[HEALTH_FORMAT_MAX_LENGTH + 1U]; /* static: keeps it off the 3 KB stack */
            AppState_Get(&st);
            WS169_GetDiagnostics(&display_diagnostics);
            (void)HealthFormat_Line(health, sizeof(health), UsbCdcLog_IsActive(), st.fps,
                                    st.ble_status, (uint32_t)display_diagnostics.last_status,
                                    display_diagnostics.spi_error_count +
                                        display_diagnostics.dma_error_count +
                                        display_diagnostics.timeout_count,
                                    AppCore_Heartbeat());
            (void)USB_Logging_Printf(LOG_LEVEL_INFO, "%s", health);
        }
    }
}
