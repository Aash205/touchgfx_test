/**
  ******************************************************************************
  * @file    WS169_driver.h
  * @brief   Waveshare 1.69-inch ST7789V2 LCD driver interface.
  *
  * The driver owns no framebuffer. A caller such as TouchGFX supplies RGB565
  * pixels, preventing duplication of the 134.4 KiB 280x240 framebuffer.
  ******************************************************************************
  */

#ifndef WS169_DRIVER_H
#define WS169_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define WS169_PORTRAIT_WIDTH       240U
#define WS169_PORTRAIT_HEIGHT      280U
#define WS169_LANDSCAPE_WIDTH      280U
#define WS169_LANDSCAPE_HEIGHT     240U
#define WS169_CONTROLLER_RAM_OFFSET 20U

typedef enum
{
    WS169_ROTATION_0 = 0,
    WS169_ROTATION_90,
    WS169_ROTATION_180,
    WS169_ROTATION_270,
    WS169_ROTATION_COUNT
} WS169_Rotation_t;

typedef enum
{
    WS169_STATUS_OK = 0,
    WS169_STATUS_INVALID_ARGUMENT,
    WS169_STATUS_NOT_INITIALIZED,
    WS169_STATUS_SPI_ERROR,
    WS169_STATUS_SPI_BUSY,
    WS169_STATUS_TIMEOUT,
    WS169_STATUS_RTOS_ERROR,
    WS169_STATUS_DMA_ERROR
} WS169_Status_t;

typedef struct
{
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef *dc_port;
    uint16_t dc_pin;
    GPIO_TypeDef *reset_port;
    uint16_t reset_pin;
    uint32_t command_timeout_ms;
    uint32_t data_timeout_ms;
} WS169_Config_t;

typedef struct
{
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
} WS169_Window_t;

typedef struct
{
    WS169_Status_t last_status;
    uint32_t last_hal_error;
    uint32_t successful_transfer_count;
    uint32_t spi_error_count;
    uint32_t dma_error_count;
    uint32_t timeout_count;
    bool initialized;
    bool rtos_ready;
} WS169_Diagnostics_t;

/** Initialize the panel using the Waveshare/ST7789V2 command sequence. */
WS169_Status_t WS169_Init(const WS169_Config_t *config, WS169_Rotation_t rotation);

/** Create bounded-wait ThreadX synchronization for DMA transfers. */
WS169_Status_t WS169_RtosInit(void);

WS169_Status_t WS169_Reset(void);
WS169_Status_t WS169_SetRotation(WS169_Rotation_t rotation);
WS169_Status_t WS169_DisplayOn(void);
WS169_Status_t WS169_DisplayOff(void);
WS169_Status_t WS169_SleepIn(void);
WS169_Status_t WS169_SleepOut(void);
WS169_Status_t WS169_SetAddressWindow(uint16_t x1,
                                      uint16_t y1,
                                      uint16_t x2,
                                      uint16_t y2);
WS169_Status_t WS169_FillScreenRGB565(uint16_t color);

/**
 * Flush a rectangle from a caller-owned RGB565 framebuffer.
 * framebuffer_stride_pixels is the number of pixels between adjacent rows.
 */
WS169_Status_t WS169_FlushRectRGB565(const uint16_t *framebuffer,
                                    uint16_t framebuffer_stride_pixels,
                                    uint16_t x,
                                    uint16_t y,
                                    uint16_t width,
                                    uint16_t height);

/** Pure geometry helper used by the driver and by on-target unit tests. */
WS169_Status_t WS169_TranslateWindow(WS169_Rotation_t rotation,
                                    uint16_t x1,
                                    uint16_t y1,
                                    uint16_t x2,
                                    uint16_t y2,
                                    WS169_Window_t *translated);

WS169_Status_t WS169_GetGeometry(WS169_Rotation_t rotation,
                                 uint16_t *width,
                                 uint16_t *height,
                                 uint8_t *madctl);

uint16_t WS169_GetWidth(void);
uint16_t WS169_GetHeight(void);
WS169_Rotation_t WS169_GetRotation(void);
WS169_Status_t WS169_GetLastStatus(void);
void WS169_GetDiagnostics(WS169_Diagnostics_t *diagnostics);
void WS169_ClearDiagnostics(void);

/** HAL callback dispatchers. Return true when the callback belongs to this driver. */
bool WS169_OnSpiTxComplete(SPI_HandleTypeDef *spi);
bool WS169_OnSpiError(SPI_HandleTypeDef *spi);

#ifdef __cplusplus
}
#endif

#endif /* WS169_DRIVER_H */
