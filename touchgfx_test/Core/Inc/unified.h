/*
 * unified.h
 *
 *  Created on: Aug 7, 2026
 *      Author: rajes
 */

#ifndef INC_UNIFIED_H_
#define INC_UNIFIED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

/*
 * This delivered project is fixed for the Waveshare 1.69-inch display.
 * DISPLAY_SELECTED is intentionally set to Waveshare and TouchGFX is 280x240 landscape.
 */
#define DISPLAY_MODEL_NHD_15_240240        1   /* NHD-1.5-240240AF-CSXP-T, ST7789VI, 240x240 */
#define DISPLAY_MODEL_EA_TFT015_22AI       2   /* Display Visions EA TFT015-22AI/22AINN, ST7789V, 240x240 */
#define DISPLAY_MODEL_ATM0154B1            3   /* AZ Displays ATM0154B1, ST7789V2, 240x240 */
#define DISPLAY_MODEL_E18RA1_FW450_N       4   /* Focus LCDs E18RA1-FW450-N, ST7789V, 240x320 */
#define DISPLAY_MODEL_E20RA_FW600_N        5   /* Focus LCDs E20RA-FW600-N, ILI9342C, 320x240 */
#define DISPLAY_MODEL_WAVESHARE_169_LCD    6   /* Waveshare 1.69inch LCD Module, ST7789V2, 240x280 */

/* Active display is fixed for this project. */
#define DISPLAY_SELECTED DISPLAY_MODEL_WAVESHARE_169_LCD

/* Optional orientation controls for non-square displays. */
#ifndef DISPLAY_E18_LANDSCAPE
#define DISPLAY_E18_LANDSCAPE 1
#endif

#ifndef DISPLAY_WAVESHARE_169_LANDSCAPE
#define DISPLAY_WAVESHARE_169_LANDSCAPE 1
#endif

/* Optional colour-order correction. Set to 1 if red/blue are swapped.
 * For Waveshare keep this 0; the vendor demo uses RGB order. */
#ifndef DISPLAY_ST7789_BGR
#define DISPLAY_ST7789_BGR 0
#endif

#if (DISPLAY_SELECTED == DISPLAY_MODEL_E18RA1_FW450_N)
  #if DISPLAY_E18_LANDSCAPE
    #define DISPLAY_WIDTH   320U
    #define DISPLAY_HEIGHT  240U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_90
  #else
    #define DISPLAY_WIDTH   240U
    #define DISPLAY_HEIGHT  320U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_0
  #endif
#elif (DISPLAY_SELECTED == DISPLAY_MODEL_E20RA_FW600_N)
    #define DISPLAY_WIDTH   320U
    #define DISPLAY_HEIGHT  240U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_0
#elif (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
  #if DISPLAY_WAVESHARE_169_LANDSCAPE
    #define DISPLAY_WIDTH   280U
    #define DISPLAY_HEIGHT  240U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_90
  #else
    #define DISPLAY_WIDTH   240U
    #define DISPLAY_HEIGHT  280U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_0
  #endif
#else
    #define DISPLAY_WIDTH   240U
    #define DISPLAY_HEIGHT  240U
    #define DISPLAY_DEFAULT_ROTATION DISPLAY_ROT_0
#endif

extern SPI_HandleTypeDef hspi2;   /* defined in main.c */
#define DISPLAY_SPI_HANDLE        (&hspi2)
/* 80 MHz APB1 / 2 = 40 MHz. Use SPI_BAUDRATEPRESCALER_4 (20 MHz) if the wiring is marginal. */
#define DISPLAY_SPI_PRESCALER     SPI_BAUDRATEPRESCALER_2
#define DISPLAY_SPI_CMD_TIMEOUT   100U
#define DISPLAY_SPI_DATA_TIMEOUT  1000U

#define DISPLAY_COLOR_BLACK       0x0000U
#define DISPLAY_COLOR_WHITE       0xFFFFU
#define DISPLAY_COLOR_RED         0xF800U
#define DISPLAY_COLOR_GREEN       0x07E0U
#define DISPLAY_COLOR_BLUE        0x001FU
#define DISPLAY_COLOR_YELLOW      0xFFE0U
#define DISPLAY_COLOR_CYAN        0x07FFU
#define DISPLAY_COLOR_MAGENTA     0xF81FU

typedef enum
{
    DISPLAY_ROT_0 = 0,
    DISPLAY_ROT_90,
    DISPLAY_ROT_180,
    DISPLAY_ROT_270
} DisplayRotation_t;

typedef enum
{
    DISPLAY_CONTROLLER_ST7789 = 0,
    DISPLAY_CONTROLLER_ILI9342
} DisplayController_t;

void Display_TestLandscapePattern(void);
/* Call once the ThreadX kernel is running: pixel DMA then waits on a semaphore instead of blocking. */
void Display_RtosInit(void);
void Display_Init(DisplayRotation_t rotation);
void Display_Reset(void);
void Display_SetAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void Display_FillScreenDirect(uint16_t color);
void Display_FlushRectRGB565(const uint16_t *framebuffer,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height);

uint16_t Display_GetWidth(void);
uint16_t Display_GetHeight(void);
DisplayController_t Display_GetController(void);

#ifdef __cplusplus
}
#endif


#endif /* INC_UNIFIED_H_ */
