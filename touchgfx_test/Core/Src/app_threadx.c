/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "waveshare_driver.h"
#include "ble_app.h"
#include "app_core.h"
#include "usb_logging.h"
#include "usb_cdc_log.h"

extern UART_HandleTypeDef hlpuart1;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THREAD_STACK_BYTES_SMALL  3072U
#define THREAD_STACK_BYTES_BLE    4096U

/* Lower number = higher priority. The TouchGFX thread (priority 5) stays above these. */
#define PRIO_BLE      10U
#define PRIO_UART     11U
#define PRIO_MONITOR  12U

/* The SPI panel has no VSYNC: a periodic timer paces TouchGFX (2 ticks = 20 ms @ 100 Hz). */
#define TOUCHGFX_VSYNC_TICKS  2U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern void touchgfxSignalVSync(void);

static TX_TIMER  vsync_timer;

static TX_THREAD thread_ble;
static TX_THREAD thread_uart_cmd;
static TX_THREAD thread_monitor;

static ULONG thread_ble_stack[THREAD_STACK_BYTES_BLE / sizeof(ULONG)];
static ULONG thread_uart_cmd_stack[THREAD_STACK_BYTES_SMALL / sizeof(ULONG)];
static ULONG thread_monitor_stack[THREAD_STACK_BYTES_SMALL / sizeof(ULONG)];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void vsync_timer_cb(ULONG input);
static void thread_ble_entry(ULONG input);
static void thread_uart_cmd_entry(ULONG input);
static void thread_monitor_entry(ULONG input);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */

  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  (void)memory_ptr;

  /* Display: from now on pixel DMA yields to other threads instead of spinning. */
  if (WS169_RtosInit() != WS169_STATUS_OK) return TX_NOT_DONE;

  /* Console/logging mutex. Log output is sent through USBX CDC-ACM. */
  USB_Logging_Init();

  /* USB CDC log drain thread; boot logs queue until the host enumerates it. */
  ret = UsbCdcLog_Init();
  if (ret != TX_SUCCESS) return ret;

  /* TouchGFX frame pacing. */
  ret = tx_timer_create(&vsync_timer, "TouchGFX VSync", vsync_timer_cb, 0,
                        TOUCHGFX_VSYNC_TICKS, TOUCHGFX_VSYNC_TICKS, TX_AUTO_ACTIVATE);
  if (ret != TX_SUCCESS) return ret;

  /* LEDs (LD1 = PC7, LD3 = PB14), UART command console, user button, shared AppState. */
  AppCore_Init(&hlpuart1);

  ret = tx_thread_create(&thread_ble, "BLE", thread_ble_entry, 0,
                         thread_ble_stack, sizeof(thread_ble_stack),
                         PRIO_BLE, PRIO_BLE, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) return ret;

  ret = tx_thread_create(&thread_uart_cmd, "UART cmd", thread_uart_cmd_entry, 0,
                         thread_uart_cmd_stack, sizeof(thread_uart_cmd_stack),
                         PRIO_UART, PRIO_UART, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) return ret;

  ret = tx_thread_create(&thread_monitor, "Monitor", thread_monitor_entry, 0,
                         thread_monitor_stack, sizeof(thread_monitor_stack),
                         PRIO_MONITOR, PRIO_MONITOR, TX_NO_TIME_SLICE, TX_AUTO_START);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */

static void vsync_timer_cb(ULONG input)
{
  (void)input;
  touchgfxSignalVSync();
}

/* UART RX interrupt: one byte at a time into the command assembler. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  AppCore_UartRxCplt(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  AppCore_UartError(huart);
}

/* BlueNRG-2: bring the stack up, advertise, then service HCI events. */
static void thread_ble_entry(ULONG input)
{
  (void)input;

  if (BLE_App_Init() == BLE_APP_STATUS_INITIALIZED &&
      BLE_App_StartAdvertising("Nucleo-BLE-Demo") == BLE_APP_STATUS_ADVERTISING) {
    USB_Logging_Printf(LOG_LEVEL_INFO, "BLE advertising as %s",
                       (char *)BLE_App_GetHandle()->device_name);
  } else {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "BLE bring-up failed (is the BlueNRG-2 shield fitted?)");
  }

  for (;;) {
    BLE_App_Process();
    tx_thread_sleep(1); /* 10 ms */
  }
}

/* LPUART1 command console + LED blink service. */
static void thread_uart_cmd_entry(ULONG input)
{
  (void)input;

  USB_Logging_Printf(LOG_LEVEL_INFO, "Console ready. Type HELP.");

  for (;;) {
    AppCore_Process();
    tx_thread_sleep(2); /* 20 ms */
  }
}

/* 1 s tick (FPS window); heartbeat log every 5 s. */
static void thread_monitor_entry(ULONG input)
{
  (void)input;
  uint32_t seconds = 0;

  for (;;) {
    tx_thread_sleep(100); /* 1 s */
    AppCore_Tick1s();

    if (++seconds % 5U == 0U) {
      AppState st;
      WS169_Diagnostics_t display_diagnostics;
      AppState_Get(&st);
      WS169_GetDiagnostics(&display_diagnostics);
      USB_Logging_Printf(LOG_LEVEL_INFO,
                         "HEALTH ThreadX=OK USBX=%s TouchGFX_FPS=%u BLE=%d Display=%d "
                         "DisplayFaults=%lu Heartbeat=%lu",
                         UsbCdcLog_IsActive() ? "ACTIVE" : "WAIT",
                          (unsigned)st.fps, (int)st.ble_status,
                         (int)display_diagnostics.last_status,
                         (unsigned long)(display_diagnostics.spi_error_count +
                                         display_diagnostics.dma_error_count +
                                         display_diagnostics.timeout_count),
                         (unsigned long)AppCore_Heartbeat());
    }
  }
}

/* USER CODE END 1 */
