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
#include <string.h>
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
static void UART_CMD_ParseCommand(UART_CommandTypeDef *handler, const char *cmd);
static void UART_CMD_SendResponse(UART_CommandTypeDef *handler, const char *response);

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
  return HAL_UART_Receive_IT(handler->huart, (uint8_t *)&handler->rx_buffer[0], 1);
}

/**
 * @brief Process received command
 */
int UART_CMD_Process(UART_CommandTypeDef *handler)
{
  if (!handler->command_ready) return 1;
  
  handler->command_ready = 0;
  UART_CMD_ParseCommand(handler, handler->rx_buffer);
  handler->rx_index = 0;
  memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
  
  return 0;
}

/**
 * @brief Handle UART receive callback
 */
void UART_CMD_ReceiveCallback(UART_CommandTypeDef *handler, uint8_t data)
{
  if (data == '\r' || data == '\n') {
    if (handler->rx_index > 0) {
      handler->rx_buffer[handler->rx_index] = '\0';
      handler->command_ready = 1;
    }
    handler->rx_index = 0;
  } else if (handler->rx_index < sizeof(handler->rx_buffer) - 1) {
    handler->rx_buffer[handler->rx_index++] = data;
  }
  
  /* Continue listening */
  HAL_UART_Receive_IT(handler->huart, (uint8_t *)&handler->rx_buffer[handler->rx_index], 1);
}

/**
 * @brief Parse and execute command
 */
static void UART_CMD_ParseCommand(UART_CommandTypeDef *handler, const char *cmd)
{
  char response[64];
  
  if (strncmp(cmd, "LED", 3) == 0) {
    /* LED command: LED<N> <ON|OFF|TOGGLE> */
    int led_num = cmd[3] - '0';
    if (led_num < 0 || led_num >= handler->led_count) {
      UART_CMD_SendResponse(handler, "ERROR: Invalid LED\r\n");
      return;
    }
    
    if (strstr(cmd, "ON")) {
      UART_CMD_SetLEDState(handler, led_num, LED_ON);
      snprintf(response, sizeof(response), "LED%d: ON\r\n", led_num);
    } else if (strstr(cmd, "OFF")) {
      UART_CMD_SetLEDState(handler, led_num, LED_OFF);
      snprintf(response, sizeof(response), "LED%d: OFF\r\n", led_num);
    } else if (strstr(cmd, "TOGGLE")) {
      UART_CMD_SetLEDState(handler, led_num, LED_TOGGLE);
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
  } else if (strncmp(cmd, "HELP", 4) == 0) {
    UART_CMD_SendResponse(handler, "Commands:\r\n");
    UART_CMD_SendResponse(handler, "  LED<N> ON/OFF/TOGGLE\r\n");
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
  uint16_t size = strlen(response);
  HAL_UART_Transmit(handler->huart, (uint8_t *)response, size, 1000);
}

/**
 * @brief Update LED states (for blinking)
 */
void UART_CMD_UpdateLEDs(UART_CommandTypeDef *handler)
{
  for (int i = 0; i < handler->led_count; i++) {
    LED_HandleTypeDef *led = &handler->leds[i];
    
    if (led->state == LED_ON) {
      HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
    } else if (led->state == LED_OFF) {
      HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
    } else if (led->state == LED_TOGGLE) {
      HAL_GPIO_TogglePin(led->port, led->pin);
      led->state = LED_OFF;  /* Reset after toggle */
    } else if (led->state == LED_BLINK_SLOW || led->state == LED_BLINK_FAST) {
      led->blink_count++;
      if (led->blink_count >= led->blink_period) {
        HAL_GPIO_TogglePin(led->port, led->pin);
        led->blink_count = 0;
      }
    }
  }
}

/**
 * @brief Set LED state
 */
void UART_CMD_SetLEDState(UART_CommandTypeDef *handler, uint8_t led_index, LED_StateTypeDef state)
{
  if (led_index >= handler->led_count) return;
  
  LED_HandleTypeDef *led = &handler->leds[led_index];
  led->state = state;
  
  if (state == LED_ON) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
  } else if (state == LED_OFF) {
    HAL_GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
  } else if (state == LED_BLINK_SLOW) {
    led->blink_period = 500;  /* 500ms period */
    led->blink_count = 0;
  } else if (state == LED_BLINK_FAST) {
    led->blink_period = 100;  /* 100ms period */
    led->blink_count = 0;
  }
}

/**
 * @brief Get status string
 */
void UART_CMD_GetStatus(UART_CommandTypeDef *handler, char *status_str)
{
  char led_status[64] = "";
  
  for (int i = 0; i < handler->led_count; i++) {
    char buf[16];
    const char *state_str;
    
    switch (handler->leds[i].state) {
      case LED_ON: state_str = "ON"; break;
      case LED_OFF: state_str = "OFF"; break;
      case LED_TOGGLE: state_str = "TOGGLE"; break;
      case LED_BLINK_SLOW: state_str = "BLINK_SLOW"; break;
      case LED_BLINK_FAST: state_str = "BLINK_FAST"; break;
      default: state_str = "UNKNOWN"; break;
    }
    
    snprintf(buf, sizeof(buf), "LED%d: %s\r\n", i, state_str);
    strcat(led_status, buf);
  }
  
  snprintf(status_str, 64, "=== Status ===\r\n%s", led_status);
}
