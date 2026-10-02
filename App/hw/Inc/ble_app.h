/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ble_app.h
  * @brief   BLE Application Header
  *          BLE initialization, advertising, and pairing management
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef BLE_APP_H
#define BLE_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* BLE Status Codes ----------------------------------------------------------*/
typedef enum {
  BLE_APP_STATUS_IDLE,
  BLE_APP_STATUS_INITIALIZING,
  BLE_APP_STATUS_INITIALIZED,
  BLE_APP_STATUS_ADVERTISING,
  BLE_APP_STATUS_CONNECTED,
  BLE_APP_STATUS_PAIRED,
  BLE_APP_STATUS_ERROR
} BLE_StatusTypeDef;

/* BLE Application Handle ---------------------------------------------------*/
typedef struct {
  BLE_StatusTypeDef status;
  uint8_t device_name[32];
  uint32_t connection_handle;
  uint8_t is_paired;
  uint32_t last_update_time;
} BLE_AppHandleTypeDef;

/* Function Prototypes -------------------------------------------------------*/
/**
 * @brief Initialize BLE application
 * @retval BLE_StatusTypeDef
 */
BLE_StatusTypeDef BLE_App_Init(void);

/** Drive the BlueNRG chip-select inactive. Called from main() before the RTOS starts. */
void BLE_App_Deselect(void);

/**
 * @brief Start BLE advertising
 * @param device_name: Device name to advertise
 * @retval BLE_StatusTypeDef
 */
BLE_StatusTypeDef BLE_App_StartAdvertising(const char *device_name);

/**
 * @brief Stop BLE advertising
 * @retval BLE_StatusTypeDef
 */
BLE_StatusTypeDef BLE_App_StopAdvertising(void);

/**
 * @brief Get BLE connection status
 * @retval BLE_StatusTypeDef
 */
BLE_StatusTypeDef BLE_App_GetStatus(void);

/**
 * @brief Get BLE application handle
 * @retval Pointer to BLE application handle
 */
BLE_AppHandleTypeDef *BLE_App_GetHandle(void);

/**
 * @brief Process BLE events (call periodically)
 */
void BLE_App_Process(void);

/**
 * @brief Send data over BLE
 * @param data: Data buffer
 * @param size: Data size
 * @retval Status
 */
int BLE_App_SendData(uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* BLE_APP_H */
