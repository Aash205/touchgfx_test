/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_driver.h
  * @brief   Waveshare 1.69-inch LCD (ST7789V2, 280x240) driver via SPI2.
  *          Provides the same OLED_* API so no callers need to change.
  *
  *  Hardware connections (STM32L496 - SPI1 is used by BLE, so we use SPI2):
  *
  *    Display Pin | MCU Pin | Function
  *    ------------|---------|---------------------------
  *    SCL (CLK)   | PB13    | SPI2_SCK  (hardware SPI2)
  *    SDI (MOSI)  | PB15    | SPI2_MOSI (hardware SPI2)
  *    CS          | PD0     | GPIO Output (active low)
  *    DC / RS     | PD1     | GPIO Output (HIGH=data, LOW=cmd)
  *    RES         | PD2     | GPIO Output (active low reset)
  *
  *  In STM32CubeMX (.ioc):
  *    - Enable SPI2: Full-Duplex Master, Prescaler for ~20MHz, NSS=Software
  *    - Set PB13 = SPI2_SCK
  *    - Set PB15 = SPI2_MOSI
  *    - Set PD0  = GPIO_Output, label DISP_CS,  default HIGH
  *    - Set PD1  = GPIO_Output, label DISP_DC,  default HIGH
  *    - Set PD2  = GPIO_Output, label DISP_RES, default HIGH
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include <string.h>
#include <stdint.h>

/* Display dimensions --------------------------------------------------------*/
#define OLED_WIDTH       280U   /* Waveshare 1.69" landscape width  */
#define OLED_HEIGHT      240U   /* Waveshare 1.69" landscape height */

/* Use hardware SPI2 (1) or GPIO bit-bang (0) ---------------------------------*/
#define OLED_USE_HW_SPI  1

/* Control GPIO pins (CS, DC, RES) -------------------------------------------*/
/* CS  - PD0 (chip select, active low) */
#define OLED_CS_PORT     GPIOD
#define OLED_CS_PIN      GPIO_PIN_0
/* DC  - PD1 (data/command: HIGH=data, LOW=command) */
#define OLED_DC_PORT     GPIOD
#define OLED_DC_PIN      GPIO_PIN_1
/* RES - PD2 (reset, active low) */
#define OLED_RES_PORT    GPIOD
#define OLED_RES_PIN     GPIO_PIN_2

/* ST7789 timing / colour helpers --------------------------------------------*/
#define OLED_I2C_TIMEOUT  1000U   /* kept for API compatibility, not used    */

/* 16-bit RGB565 colour constants */
#define OLED_COLOR_BLACK    0x0000U
#define OLED_COLOR_WHITE    0xFFFFU
#define OLED_COLOR_RED      0xF800U
#define OLED_COLOR_GREEN    0x07E0U
#define OLED_COLOR_BLUE     0x001FU
#define OLED_COLOR_YELLOW   0xFFE0U
#define OLED_COLOR_CYAN     0x07FFU
#define OLED_COLOR_MAGENTA  0xF81FU

/* Font metrics (5×8 bitmap, 6-pixel advance) --------------------------------*/
#define OLED_FONT_W   5U
#define OLED_FONT_H   8U

/* Handle: kept for API compatibility ----------------------------------------*/
typedef struct {
    /* Foreground/background colours for text rendering */
    uint16_t fg_color;
    uint16_t bg_color;
    /* Internal: remembers last cursor position */
    uint16_t cursor_x;
    uint16_t cursor_y;
    /* Dummy field kept for binary-compat with old OLED_HandleTypeDef users   */
    void     *hi2c;   /* unused – was I2C_HandleTypeDef* */
} OLED_HandleTypeDef;

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief  Initialise the Waveshare 1.69" LCD (ST7789V2).
 * @param  handle  Pointer to an OLED_HandleTypeDef (caller-allocated).
 * @param  hi2c    Ignored (kept for source compatibility with old SSD1306 API).
 * @retval HAL_OK always (hardware errors are silently suppressed).
 */
HAL_StatusTypeDef OLED_Init(OLED_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c);

/**
 * @brief  Turn display on (backlight / ST7789 DISPON).
 */
HAL_StatusTypeDef OLED_DisplayOn(OLED_HandleTypeDef *handle);

/**
 * @brief  Turn display off (ST7789 DISPOFF).
 */
HAL_StatusTypeDef OLED_DisplayOff(OLED_HandleTypeDef *handle);

/**
 * @brief  Fill the entire screen with black.
 */
HAL_StatusTypeDef OLED_Clear(OLED_HandleTypeDef *handle);

/**
 * @brief  No-op for this driver (the display is written directly, no buffer).
 *         Kept for source compatibility.
 */
HAL_StatusTypeDef OLED_UpdateDisplay(OLED_HandleTypeDef *handle);

/**
 * @brief  Draw a single pixel.
 * @param  x, y   Coordinates (0-based).
 * @param  state  1 = foreground colour, 0 = background colour.
 */
void OLED_SetPixel(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint8_t state);

/**
 * @brief  Print an ASCII string at character-grid position (x, row).
 * @param  x    Pixel x-coordinate of the first character.
 * @param  y    Character row (0 = top, each row is OLED_FONT_H pixels).
 * @param  str  Null-terminated ASCII string.
 */
void OLED_PrintStr(OLED_HandleTypeDef *handle, uint16_t x, uint8_t y, const char *str);

/**
 * @brief  Draw a horizontal line.
 */
void OLED_DrawHLine(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t width);

/**
 * @brief  Draw a vertical line.
 */
void OLED_DrawVLine(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t height);

/**
 * @brief  Draw a rectangle outline.
 */
void OLED_DrawRect(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/**
 * @brief  Fill a rectangle with a colour.
 */
void OLED_FillRect(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_DRIVER_H__ */
