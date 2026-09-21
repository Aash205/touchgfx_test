/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    uart_commands.h
  * @brief   UART Command Handler Header
  *          Handle LED control and status commands via UART
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __UART_COMMANDS_H__
#define __UART_COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* LED States ----------------------------------------------------------------*/
typedef enum {
  LED_OFF,
  LED_ON,
  LED_TOGGLE,
  LED_BLINK_SLOW,
  LED_BLINK_FAST
} LED_StateTypeDef;

/* LED Handle ----------------------------------------------------------------*/
typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  LED_StateTypeDef state;
  uint32_t blink_count;
  uint32_t blink_period;
} LED_HandleTypeDef;

/* UART Command Handler -------------------------------------------------------*/
typedef struct {
  UART_HandleTypeDef *huart;
  char rx_buffer[64];
  uint8_t rx_byte;            /* single-byte interrupt receive target */
  uint8_t rx_index;
  uint8_t command_ready;
  LED_HandleTypeDef leds[4];  /* Support up to 4 LEDs */
  uint8_t led_count;
} UART_CommandTypeDef;

/* Function Prototypes -------------------------------------------------------*/
/**
 * @brief Initialize UART command handler
 * @param handler: UART command handler
 * @param huart: UART handle
 * @retval HAL status
 */
HAL_StatusTypeDef UART_CMD_Init(UART_CommandTypeDef *handler, UART_HandleTypeDef *huart);

/**
 * @brief Register LED
 * @param handler: UART command handler
 * @param port: GPIO port
 * @param pin: GPIO pin
 * @retval HAL status
 */
HAL_StatusTypeDef UART_CMD_RegisterLED(UART_CommandTypeDef *handler, GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief Start listening for UART commands
 * @param handler: UART command handler
 * @retval HAL status
 */
HAL_StatusTypeDef UART_CMD_StartListening(UART_CommandTypeDef *handler);

/**
 * @brief Process received command
 * @param handler: UART command handler
 * @retval Status (0=success, -1=error, 1=no command)
 */
int UART_CMD_Process(UART_CommandTypeDef *handler);

/**
 * @brief Handle UART receive callback
 * @param handler: UART command handler
 * @param data: Received data
 */
void UART_CMD_ReceiveCallback(UART_CommandTypeDef *handler, uint8_t data);

/**
 * @brief Update LED states (for blinking)
 * @param handler: UART command handler
 */
void UART_CMD_UpdateLEDs(UART_CommandTypeDef *handler);

/**
 * @brief Set LED state
 * @param handler: UART command handler
 * @param led_index: LED index
 * @param state: LED state
 */
void UART_CMD_SetLEDState(UART_CommandTypeDef *handler, uint8_t led_index, LED_StateTypeDef state);

/**
 * @brief Get status string
 * @param handler: UART command handler
 * @param status_str: Output status string
 */
void UART_CMD_GetStatus(UART_CommandTypeDef *handler, char *status_str);

#ifdef __cplusplus
}
#endif

#endif /* __UART_COMMANDS_H__ */
