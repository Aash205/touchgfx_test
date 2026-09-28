/**
  ******************************************************************************
  * @file    unified.h
  * @brief   Compatibility adapter for the application-owned WS169 driver.
  ******************************************************************************
  */

#ifndef UNIFIED_H
#define UNIFIED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "WS169_driver.h"
#include <stdint.h>

#define DISPLAY_WIDTH              WS169_LANDSCAPE_WIDTH
#define DISPLAY_HEIGHT             WS169_LANDSCAPE_HEIGHT
#define DISPLAY_DEFAULT_ROTATION   DISPLAY_ROT_90

#define DISPLAY_COLOR_BLACK        0x0000U
#define DISPLAY_COLOR_WHITE        0xFFFFU
#define DISPLAY_COLOR_RED          0xF800U
#define DISPLAY_COLOR_GREEN        0x07E0U
#define DISPLAY_COLOR_BLUE         0x001FU
#define DISPLAY_COLOR_YELLOW       0xFFE0U
#define DISPLAY_COLOR_CYAN         0x07FFU
#define DISPLAY_COLOR_MAGENTA      0xF81FU

typedef enum
{
    DISPLAY_ROT_0 = 0,
    DISPLAY_ROT_90,
    DISPLAY_ROT_180,
    DISPLAY_ROT_270
} DisplayRotation_t;

typedef enum
{
    DISPLAY_CONTROLLER_ST7789 = 0
} DisplayController_t;

WS169_Status_t Display_RtosInit(void);
WS169_Status_t Display_Init(DisplayRotation_t rotation);
WS169_Status_t Display_Reset(void);
WS169_Status_t Display_SetAddressWindow(uint16_t x1,
                                        uint16_t y1,
                                        uint16_t x2,
                                        uint16_t y2);
WS169_Status_t Display_FillScreenDirect(uint16_t color);
WS169_Status_t Display_FlushRectRGB565(const uint16_t *framebuffer,
                                      uint16_t x,
                                      uint16_t y,
                                      uint16_t width,
                                      uint16_t height);

uint16_t Display_GetWidth(void);
uint16_t Display_GetHeight(void);
DisplayController_t Display_GetController(void);
WS169_Status_t Display_GetLastStatus(void);
void Display_GetDiagnostics(WS169_Diagnostics_t *diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_H */
