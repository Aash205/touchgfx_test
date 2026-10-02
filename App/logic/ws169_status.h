#ifndef WS169_STATUS_H
#define WS169_STATUS_H

#include "ws169_geometry.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* The HAL_StatusTypeDef codes of the STM32 HAL (the driver checks them at compile time). */
#define WS169_HAL_OK 0U
#define WS169_HAL_ERROR 1U
#define WS169_HAL_BUSY 2U
#define WS169_HAL_TIMEOUT 3U

/* Which diagnostic counter a driver status increments. */
typedef enum
{
    WS169_COUNTER_NONE = 0,
    WS169_COUNTER_SPI,
    WS169_COUNTER_DMA,
    WS169_COUNTER_TIMEOUT
} WS169_Counter_t;

/* clang-format off */
/*
 * WS169_StatusFromHal: the driver status for a HAL return code of an SPI call.
 * WS169_HAL_OK gives WS169_STATUS_OK, WS169_HAL_BUSY gives SPI_BUSY, WS169_HAL_TIMEOUT gives
 * TIMEOUT. WS169_HAL_ERROR, and any code the HAL does not define, gives DMA_ERROR for a DMA
 * transfer and SPI_ERROR for any other transfer.
 *
 * WS169_CounterOf: the diagnostic counter a status is counted in: SPI for SPI_ERROR and
 * SPI_BUSY, DMA for DMA_ERROR, TIMEOUT for TIMEOUT. Every other status, including values outside
 * the enum, is not counted (WS169_COUNTER_NONE).
 */
WS169_Status_t WS169_StatusFromHal(uint32_t hal_status, bool dma_transfer);
WS169_Counter_t WS169_CounterOf(WS169_Status_t status);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* WS169_STATUS_H */
