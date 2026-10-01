/**
  ******************************************************************************
  * @file    app_core.c
  * @brief   LED / console ownership and the shared AppState (see app_core.h).
  ******************************************************************************
  */

#include "app_core.h"
#include "uart_commands.h"
#include "counters.h"
#include "debounce.h"
#include "ble_app.h"
#include "usb_logging.h"
#include "tx_api.h"

#define USER_BTN_PORT   GPIOC          /* Nucleo B1 (blue), active high */
#define USER_BTN_PIN    GPIO_PIN_13
#define BTN_DEBOUNCE_POLLS 2U

static UART_CommandTypeDef s_console;
static Counters_t s_counters;            /* frame and heartbeat counters (App/logic/counters) */

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
  static Debounce_t debounce;

  const bool pressed = (HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN) == GPIO_PIN_SET);
  if (Debounce_Poll(&debounce, pressed, BTN_DEBOUNCE_POLLS)) {
    AppState_ToggleLed(0);        /* B1 press toggles LD1 */
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
  Counters_Tick1s(&s_counters);
}

uint32_t AppCore_Heartbeat(void)
{
  return Counters_Heartbeat(&s_counters);
}

/* ---- AppState (app_state.h) --------------------------------------------- */

void AppState_Get(AppState *out)
{
  for (uint8_t i = 0; i < APP_LED_COUNT; i++) {
    out->led[i] = (i < s_console.led_count) && (s_console.leds[i].fsm.state != LED_OFF);
  }
  out->ble_status = (uint8_t)BLE_App_GetStatus();
  /* Use the RTOS clock for application uptime. It starts when ThreadX starts and
     is independent of the HAL peripheral time base used by driver timeouts. */
  out->uptime_s = (uint32_t)(tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
  out->heartbeat = Counters_HeartbeatValue(&s_counters);
  out->fps = Counters_Fps(&s_counters);
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
    AppState_SetLed(idx, s_console.leds[idx].fsm.state == LED_OFF);
  }
}

void AppState_FrameFlushed(void)
{
  Counters_FrameFlushed(&s_counters);
}
