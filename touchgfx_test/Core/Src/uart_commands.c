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
#include <string.h>
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
static void UART_CMD_ParseCommand(UART_CommandTypeDef *handler, const char *cmd);
static void UART_CMD_SendResponse(UART_CommandTypeDef *handler, const char *response);
static void UART_CMD_GetStatus(const UART_CommandTypeDef *handler, char *status_str);

/**
 * @brief Initialize UART command handler
 */
HAL_StatusTypeDef UART_CMD_Init(UART_CommandTypeDef *handler, UART_HandleTypeDef *huart)
{
  handler->huart = huart;
  handler->rx_index = 0;
  handler->command_ready = 0;
  handler->led_count = 0;
  memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
  return HAL_OK;
}

/**
 * @brief Register LED
 */
HAL_StatusTypeDef UART_CMD_RegisterLED(UART_CommandTypeDef *handler, GPIO_TypeDef *port, uint16_t pin)
{
  if (handler->led_count >= 4) return HAL_ERROR;
  
  handler->leds[handler->led_count].port = port;
  handler->leds[handler->led_count].pin = pin;
  handler->leds[handler->led_count].state = LED_OFF;
  handler->leds[handler->led_count].blink_count = 0;
  handler->leds[handler->led_count].blink_period = 100;

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(port, &gpio);
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
  handler->led_count++;
  
  return HAL_OK;
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
  if (!handler->command_ready) return 1;

  (void)USB_Logging_Printf(LOG_LEVEL_DEBUG, "LPUART1 command received: %s", handler->rx_buffer);
  UART_CMD_ParseCommand(handler, handler->rx_buffer);
  handler->rx_index = 0;
  memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
  handler->command_ready = 0;   /* release the buffer to the RX interrupt last */
  
  return 0;
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
  
  if (strncmp(cmd, "LED", 3) == 0) {
    /* LED command: LED<N> <ON|OFF|TOGGLE> */
    int led_num = (int)(cmd[3] - '0');
    if ((cmd[3] < '0') || (cmd[3] > '9') ||
        (led_num < 0) || (led_num >= (int)handler->led_count)) {
      UART_CMD_SendResponse(handler, "ERROR: Invalid LED\r\n");
      return;
    }
    
    if (strstr(cmd, "ON") != NULL) {
      UART_CMD_SetLEDState(handler, (uint8_t)led_num, LED_ON);
      snprintf(response, sizeof(response), "LED%d: ON\r\n", led_num);
    } else if (strstr(cmd, "OFF") != NULL) {
      UART_CMD_SetLEDState(handler, (uint8_t)led_num, LED_OFF);
      snprintf(response, sizeof(response), "LED%d: OFF\r\n", led_num);
    } else if (strstr(cmd, "FAST") != NULL) {
      UART_CMD_SetLEDState(handler, (uint8_t)led_num, LED_BLINK_FAST);
      snprintf(response, sizeof(response), "LED%d: BLINK FAST\r\n", led_num);
    } else if (strstr(cmd, "BLINK") != NULL) {
      UART_CMD_SetLEDState(handler, (uint8_t)led_num, LED_BLINK_SLOW);
      snprintf(response, sizeof(response), "LED%d: BLINK\r\n", led_num);
    } else if (strstr(cmd, "TOGGLE") != NULL) {
      UART_CMD_SetLEDState(handler, (uint8_t)led_num, LED_TOGGLE);
      snprintf(response, sizeof(response), "LED%d: TOGGLE\r\n", led_num);
    } else {
      UART_CMD_SendResponse(handler, "ERROR: Invalid command\r\n");
      return;
    }
    UART_CMD_SendResponse(handler, response);
  } else if (strncmp(cmd, "STATUS", 6) == 0) {
    /* Get status */
    UART_CMD_GetStatus(handler, response);
    UART_CMD_SendResponse(handler, response);
  } else if (strncmp(cmd, "BLE", 3) == 0) {
    snprintf(response, sizeof(response), "BLE status: %d\r\n", (int)BLE_App_GetStatus());
    UART_CMD_SendResponse(handler, response);
  } else if (strncmp(cmd, "HELP", 4) == 0) {
    UART_CMD_SendResponse(handler, "Commands:\r\n");
    UART_CMD_SendResponse(handler, "  LED<N> ON/OFF/TOGGLE/BLINK/FAST\r\n");
    UART_CMD_SendResponse(handler, "  BLE   (status)\r\n");
    UART_CMD_SendResponse(handler, "  STATUS\r\n");
    UART_CMD_SendResponse(handler, "  HELP\r\n");
  } else {
    UART_CMD_SendResponse(handler, "ERROR: Unknown command\r\n");
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
    
    if (led->state == LED_ON) {
      HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
    } else if (led->state == LED_OFF) {
      HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
    } else if (led->state == LED_TOGGLE) {
      HAL_GPIO_TogglePin(led->port, led->pin);
      led->state = LED_OFF;  /* Reset after toggle */
    } else if (led->state == LED_BLINK_SLOW || led->state == LED_BLINK_FAST) {
      /* blink_count holds the tick of the last toggle; blink_period is the half period in ms */
      if ((HAL_GetTick() - led->blink_count) >= led->blink_period) {
        HAL_GPIO_TogglePin(led->port, led->pin);
        led->blink_count = HAL_GetTick();
      }
    }
  }
}

/**
 * @brief Set LED state
 */
void UART_CMD_SetLEDState(UART_CommandTypeDef *handler, uint8_t led_index, LED_StateTypeDef state)
{
  if (led_index >= handler->led_count) { return; }
  
  LED_HandleTypeDef *led = &handler->leds[led_index];
  led->state = state;
  
  if (state == LED_ON) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
  } else if (state == LED_OFF) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
  } else if (state == LED_TOGGLE) {
    HAL_GPIO_TogglePin(led->port, led->pin);
    led->state = (HAL_GPIO_ReadPin(led->port, led->pin) == GPIO_PIN_SET) ? LED_ON : LED_OFF;
  } else if (state == LED_BLINK_SLOW) {
    led->blink_period = 500U;  /* toggle every 500 ms */
    led->blink_count = HAL_GetTick();
  } else if (state == LED_BLINK_FAST) {
    led->blink_period = 100U;  /* toggle every 100 ms */
    led->blink_count = HAL_GetTick();
  }
}

/**
 * @brief Get status string
 */
static void UART_CMD_GetStatus(const UART_CommandTypeDef *handler, char *status_str)
{
  char led_status[96] = "";
  size_t used = 0U;

  for (uint8_t i = 0U; i < handler->led_count; i++) {
    char buf[24];
    const char *state_str;
    
    switch (handler->leds[i].state) {
      case LED_ON: state_str = "ON"; break;
      case LED_OFF: state_str = "OFF"; break;
      case LED_TOGGLE: state_str = "TOGGLE"; break;
      case LED_BLINK_SLOW: state_str = "BLINK_SLOW"; break;
      case LED_BLINK_FAST: state_str = "BLINK_FAST"; break;
      default: state_str = "UNKNOWN"; break;
    }
    
    int len = snprintf(buf, sizeof(buf), "LED%u: %s\r\n", (unsigned)i, state_str);
    if ((len > 0) && ((size_t)len < sizeof(buf)) && (used < sizeof(led_status)))
    {
      int appended = snprintf(&led_status[used], sizeof(led_status) - used, "%s", buf);
      if ((appended > 0) && ((size_t)appended < (sizeof(led_status) - used)))
      {
        used += (size_t)appended;
      }
    }
  }
  
  snprintf(status_str, 128, "=== Status ===\r\n%s", led_status);
}
