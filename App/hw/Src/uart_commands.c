/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    uart_commands.c
  * @brief   UART Command Handler Implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "uart_commands.h"
#include "usb_logging.h"
#include "ble_app.h"
#include "uart_line.h"
#include "cmd_parse.h"
#include <stdbool.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
static void UART_CMD_ParseCommand(UART_CommandTypeDef *handler, const char *cmd);
static void UART_CMD_SendResponse(UART_CommandTypeDef *handler, const char *response);
static void UART_CMD_GetStatus(const UART_CommandTypeDef *handler, char *status_str);
static void UART_CMD_ApplyPinAction(const LED_HandleTypeDef *led, LedPinAction_t action);

/**
 * @brief Initialize UART command handler
 */
HAL_StatusTypeDef UART_CMD_Init(UART_CommandTypeDef *handler, UART_HandleTypeDef *huart)
{
  handler->huart = huart;
  handler->rx_index = 0;
  handler->command_ready = 0;
  handler->led_count = 0;
  (void)memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
  return HAL_OK;
}

/**
 * @brief Register LED
 */
HAL_StatusTypeDef UART_CMD_RegisterLED(UART_CommandTypeDef *handler, GPIO_TypeDef *port, uint16_t pin)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  if (handler->led_count < 4U) {
    GPIO_InitTypeDef gpio = {0};

    handler->leds[handler->led_count].port = port;
    handler->leds[handler->led_count].pin = pin;
    Led_Init(&handler->leds[handler->led_count].fsm);

    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    /* cppcheck-suppress misra-c2012-7.3 -- the HAL's GPIO_NOPULL constant */
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    handler->led_count++;
    status = HAL_OK;
  }

  return status;
}

/**
 * @brief Start listening for UART commands
 */
HAL_StatusTypeDef UART_CMD_StartListening(UART_CommandTypeDef *handler)
{
  handler->rx_index = 0;
  handler->command_ready = 0;
  return HAL_UART_Receive_IT(handler->huart, &handler->rx_byte, 1);
}

/**
 * @brief Process received command
 */
int UART_CMD_Process(UART_CommandTypeDef *handler)
{
  int result = 1;

  if (handler->command_ready != 0U) {
    (void)USB_Logging_Printf(LOG_LEVEL_DEBUG, "LPUART1 command received: %s", handler->rx_buffer);
    UART_CMD_ParseCommand(handler, handler->rx_buffer);
    handler->rx_index = 0;
    (void)memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
    handler->command_ready = 0;   /* release the buffer to the RX interrupt last */
    result = 0;
  }

  return result;
}

/**
 * @brief Handle UART receive callback
 */
void UART_CMD_ReceiveCallback(UART_CommandTypeDef *handler, uint8_t data)
{
  UART_LineFeedByte(handler->rx_buffer, (uint8_t)sizeof(handler->rx_buffer),
                    &handler->rx_index, &handler->command_ready, data);

  (void)HAL_UART_Receive_IT(handler->huart, &handler->rx_byte, 1U);
}

/**
 * @brief Parse and execute command
 */
static void UART_CMD_ParseCommand(UART_CommandTypeDef *handler, const char *cmd)
{
  char response[128];
  const Cmd_t parsed = Cmd_Parse(cmd, handler->led_count);

  switch (parsed.kind) {
    case CMD_LED:
      UART_CMD_SetLEDState(handler, parsed.led_index, parsed.action);
      (void)Cmd_FormatLedReply(response, sizeof(response), parsed.led_index, parsed.action);
      UART_CMD_SendResponse(handler, response);
      break;
    case CMD_STATUS:
      UART_CMD_GetStatus(handler, response);
      UART_CMD_SendResponse(handler, response);
      break;
    case CMD_BLE:
      (void)Cmd_FormatBleStatus(response, sizeof(response), (unsigned)BLE_App_GetStatus());
      UART_CMD_SendResponse(handler, response);
      break;
    case CMD_HELP:
      for (unsigned i = 0U; Cmd_HelpLine(i) != NULL; i++) {
        UART_CMD_SendResponse(handler, Cmd_HelpLine(i));
      }
      break;
    default: {
      /* CMD_INVALID_LED, CMD_INVALID_ACTION, CMD_UNKNOWN */
      const char *error_text = Cmd_ErrorText(parsed.kind);
      if (error_text != NULL) {
        UART_CMD_SendResponse(handler, error_text);
      }
      break;
    }
  }
}

/**
 * @brief Send response via UART
 */
static void UART_CMD_SendResponse(UART_CommandTypeDef *handler, const char *response)
{
  (void)handler;
  (void)USB_Logging_SendRaw((const uint8_t *)response,
                            (uint16_t)strlen(response)); /* shared, mutex-protected console */
}

/**
 * @brief Update LED states (for blinking)
 */
void UART_CMD_UpdateLEDs(UART_CommandTypeDef *handler)
{
  for (uint8_t i = 0U; i < handler->led_count; i++) {
    LED_HandleTypeDef *led = &handler->leds[i];

    UART_CMD_ApplyPinAction(led, Led_Update(&led->fsm, HAL_GetTick()));
  }
}

/**
 * @brief Do to the LED pin what the state machine decided
 */
static void UART_CMD_ApplyPinAction(const LED_HandleTypeDef *led, LedPinAction_t action)
{
  if (action == LED_PIN_HIGH) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
  } else if (action == LED_PIN_LOW) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
  } else if (action == LED_PIN_TOGGLE) {
    HAL_GPIO_TogglePin(led->port, led->pin);
  } else {
    /* LED_PIN_HOLD: leave the pin alone */
  }
}

/**
 * @brief Set LED state
 */
void UART_CMD_SetLEDState(UART_CommandTypeDef *handler, uint8_t led_index, LED_StateTypeDef state)
{
  if (led_index < handler->led_count) {
    LED_HandleTypeDef *led = &handler->leds[led_index];
    const bool pin_high = (HAL_GPIO_ReadPin(led->port, led->pin) == GPIO_PIN_SET);

    UART_CMD_ApplyPinAction(led, Led_Set(&led->fsm, state, HAL_GetTick(), pin_high));
  }
}

/**
 * @brief Get status string
 */
static void UART_CMD_GetStatus(const UART_CommandTypeDef *handler, char *status_str)
{
  LED_StateTypeDef states[sizeof(handler->leds) / sizeof(handler->leds[0])];

  (void)memset(states, 0, sizeof(states));   /* all LED_OFF; only led_count entries are used */
  for (uint8_t i = 0U; i < handler->led_count; i++) {
    states[i] = handler->leds[i].fsm.state;
  }

  (void)Cmd_FormatStatus(status_str, 128U, states, handler->led_count);
}
