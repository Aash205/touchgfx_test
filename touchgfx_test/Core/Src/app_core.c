/**
  ******************************************************************************
  * @file    app_core.c
  * @brief   LED / console ownership and the shared AppState (see app_core.h).
  ******************************************************************************
  */

#include "app_core.h"
#include "uart_commands.h"
#include "ble_app.h"
#include "usb_logging.h"
#include "tx_api.h"

#define USER_BTN_PORT   GPIOC          /* Nucleo B1 (blue), active high */
#define USER_BTN_PIN    GPIO_PIN_13
#define BTN_DEBOUNCE_POLLS 2U

static UART_CommandTypeDef s_console;
static volatile uint32_t s_frames;       /* flushes since the last Tick1s */
static volatile uint16_t s_fps;
static volatile uint32_t s_heartbeat;

void AppCore_Init(UART_HandleTypeDef *console_uart)
{
  GPIO_InitTypeDef gpio = {0};
  HAL_StatusTypeDef uart_status;

  UART_CMD_Init(&s_console, console_uart);
  UART_CMD_RegisterLED(&s_console, GPIOC, GPIO_PIN_7);    /* LD1 */
  UART_CMD_RegisterLED(&s_console, GPIOB, GPIO_PIN_14);   /* LD3 */

  uart_status = UART_CMD_StartListening(&s_console);
  if (uart_status == HAL_OK) {
    USB_Logging_Printf(LOG_LEVEL_INFO, "LPUART1 RX ready on PG8 at 115200 8N1");
  } else {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "LPUART1 RX arm failed: %d", (int)uart_status);
  }

  __HAL_RCC_GPIOC_CLK_ENABLE();
  gpio.Pin = USER_BTN_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_BTN_PORT, &gpio);
}

static void poll_button(void)
{
  static uint8_t stable, last;

  uint8_t now = (HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN) == GPIO_PIN_SET);
  if (now == last) {
    if (stable < BTN_DEBOUNCE_POLLS && ++stable == BTN_DEBOUNCE_POLLS && now) {
      AppState_ToggleLed(0);      /* B1 press toggles LD1 */
    }
  } else {
    last = now;
    stable = 0;
  }
}

void AppCore_Process(void)
{
  UART_CMD_Process(&s_console);
  UART_CMD_UpdateLEDs(&s_console);
  poll_button();
}

void AppCore_UartRxCplt(UART_HandleTypeDef *huart)
{
  if (huart == s_console.huart) {
    UART_CMD_ReceiveCallback(&s_console, s_console.rx_byte);
  }
}

void AppCore_UartError(UART_HandleTypeDef *huart)
{
  if (huart == s_console.huart) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    UART_CMD_StartListening(&s_console);
  }
}

void AppCore_Tick1s(void)
{
  s_fps = (uint16_t)s_frames;
  s_frames = 0;
}

uint32_t AppCore_Heartbeat(void)
{
  return ++s_heartbeat;
}

/* ---- AppState (app_state.h) --------------------------------------------- */

void AppState_Get(AppState *out)
{
  for (uint8_t i = 0; i < APP_LED_COUNT; i++) {
    out->led[i] = (i < s_console.led_count) && (s_console.leds[i].state != LED_OFF);
  }
  out->ble_status = (uint8_t)BLE_App_GetStatus();
  /* Use the RTOS clock for application uptime. It starts when ThreadX starts and
     is independent of the HAL peripheral time base used by driver timeouts. */
  out->uptime_s = (uint32_t)(tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
  out->heartbeat = s_heartbeat;
  out->fps = s_fps;
}

void AppState_SetLed(uint8_t idx, uint8_t on)
{
  if (idx < APP_LED_COUNT) {
    UART_CMD_SetLEDState(&s_console, idx, on ? LED_ON : LED_OFF);
  }
}

void AppState_ToggleLed(uint8_t idx)
{
  if (idx < s_console.led_count) {
    AppState_SetLed(idx, s_console.leds[idx].state == LED_OFF);
  }
}

void AppState_FrameFlushed(void)
{
  s_frames++;
}
