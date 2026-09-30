#ifndef WS169_GEOMETRY_H
#define WS169_GEOMETRY_H

#include <stdint.h>

#define WS169_PORTRAIT_WIDTH        240U
#define WS169_PORTRAIT_HEIGHT       280U
#define WS169_LANDSCAPE_WIDTH       280U
#define WS169_LANDSCAPE_HEIGHT      240U
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
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
} WS169_Window_t;

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

#endif
