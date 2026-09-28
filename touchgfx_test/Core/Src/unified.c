/**
  ******************************************************************************
  * @file    unified.c
  * @brief   Compatibility adapter for the application-owned WS169 driver.
  ******************************************************************************
  */

#include "unified.h"
#include "main.h"

extern SPI_HandleTypeDef hspi2;

static WS169_Rotation_t display_to_ws169_rotation(DisplayRotation_t rotation)
{
    switch (rotation)
    {
        case DISPLAY_ROT_0:
            return WS169_ROTATION_0;
        case DISPLAY_ROT_90:
            return WS169_ROTATION_90;
        case DISPLAY_ROT_180:
            return WS169_ROTATION_180;
        case DISPLAY_ROT_270:
            return WS169_ROTATION_270;
        default:
            return WS169_ROTATION_COUNT;
    }
}

WS169_Status_t Display_Init(DisplayRotation_t rotation)
{
    const WS169_Config_t config = {
        .spi = &hspi2,
        .cs_port = DISP_CS_GPIO_Port,
        .cs_pin = DISP_CS_Pin,
        .dc_port = DISP_DC_GPIO_Port,
        .dc_pin = DISP_DC_Pin,
        .reset_port = DISP_RES_GPIO_Port,
        .reset_pin = DISP_RES_Pin,
        .command_timeout_ms = 100U,
        .data_timeout_ms = 1000U
    };

    return WS169_Init(&config, display_to_ws169_rotation(rotation));
}

WS169_Status_t Display_RtosInit(void)
{
    return WS169_RtosInit();
}

WS169_Status_t Display_Reset(void)
{
    return WS169_Reset();
}

WS169_Status_t Display_SetAddressWindow(uint16_t x1,
                                        uint16_t y1,
                                        uint16_t x2,
                                        uint16_t y2)
{
    return WS169_SetAddressWindow(x1, y1, x2, y2);
}

WS169_Status_t Display_FillScreenDirect(uint16_t color)
{
    return WS169_FillScreenRGB565(color);
}

WS169_Status_t Display_FlushRectRGB565(const uint16_t *framebuffer,
                                      uint16_t x,
                                      uint16_t y,
                                      uint16_t width,
                                      uint16_t height)
{
    return WS169_FlushRectRGB565(framebuffer,
                                DISPLAY_WIDTH,
                                x,
                                y,
                                width,
                                height);
}

uint16_t Display_GetWidth(void)
{
    return WS169_GetWidth();
}

uint16_t Display_GetHeight(void)
{
    return WS169_GetHeight();
}

DisplayController_t Display_GetController(void)
{
    return DISPLAY_CONTROLLER_ST7789;
}

WS169_Status_t Display_GetLastStatus(void)
{
    return WS169_GetLastStatus();
}

void Display_GetDiagnostics(WS169_Diagnostics_t *diagnostics)
{
    WS169_GetDiagnostics(diagnostics);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
    (void)WS169_OnSpiTxComplete(spi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi)
{
    (void)WS169_OnSpiError(spi);
}
