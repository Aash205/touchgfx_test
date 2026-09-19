/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    oled_driver.c
  * @brief   Waveshare 1.69-inch LCD (ST7789V2, 280x240 SPI) implementation.
  *
  *  This file replaces the previous SSD1306 I2C OLED driver.
  *  It preserves the same public OLED_* API so app_threadx.c needs no changes.
  *
  *  The Waveshare 1.69" module uses a ST7789V2 controller with a 240x280 active
  *  area inside 240x320 RAM.  In landscape mode (280 wide x 240 tall) we apply
  *  a 20-pixel X-offset to the address window.
  *
  *  SPI communication uses GPIO bit-bang so no peripheral reconfiguration is
  *  needed (set OLED_USE_HW_SPI=1 in the header to switch to hardware SPI2).
  *
  *  Pin mapping (see oled_driver.h for GPIO defines):
  *    SCL -> PA1,  SDI -> PA7,  CS -> PB4,  DC -> PE0,  RES -> PB1
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "oled_driver.h"
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * ST7789V2 command set
 * ========================================================================== */
#define ST7789_NOP        0x00U
#define ST7789_SWRESET    0x01U
#define ST7789_SLPIN      0x10U
#define ST7789_SLPOUT     0x11U
#define ST7789_NORON      0x13U
#define ST7789_INVOFF     0x20U
#define ST7789_INVON      0x21U
#define ST7789_DISPOFF    0x28U
#define ST7789_DISPON     0x29U
#define ST7789_CASET      0x2AU
#define ST7789_RASET      0x2BU
#define ST7789_RAMWR      0x2CU
#define ST7789_MADCTL     0x36U
#define ST7789_COLMOD     0x3AU
#define ST7789_PORCTRL    0xB2U
#define ST7789_GCTRL      0xB7U
#define ST7789_VCOMS      0xBBU
#define ST7789_LCMCTRL    0xC0U
#define ST7789_VDVVRHEN   0xC2U
#define ST7789_VRHS       0xC3U
#define ST7789_VDVS       0xC4U
#define ST7789_FRCTRL2    0xC6U
#define ST7789_PWCTRL1    0xD0U
#define ST7789_PVGAMCTRL  0xE0U
#define ST7789_NVGAMCTRL  0xE1U

/* MADCTL flags for landscape rotation (MX+MV) */
#define ST7789_MADCTL_MY  0x80U
#define ST7789_MADCTL_MX  0x40U
#define ST7789_MADCTL_MV  0x20U
#define ST7789_MADCTL_ML  0x10U
#define ST7789_MADCTL_BGR 0x08U
#define ST7789_MADCTL_MH  0x04U

/* Waveshare 1.69" address offsets in landscape (90-degree rotation) ---------*/
#define WS169_X_OFFSET   20U   /* 20 columns unused in ST7789 280-wide frame  */
#define WS169_Y_OFFSET    0U

/* ============================================================================
 * 5×8 ASCII font (printable characters 0x20–0x7E)
 * Index 0 = space (0x20), index 94 = '~' (0x7E)
 * ========================================================================== */
static const uint8_t s_font5x8[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
  {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
  {0x00,0x07,0x00,0x07,0x00}, /* '"' */
  {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
  {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
  {0x23,0x13,0x08,0x64,0x62}, /* '%' */
  {0x36,0x49,0x55,0x22,0x50}, /* '&' */
  {0x00,0x05,0x03,0x00,0x00}, /* ''' */
  {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
  {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
  {0x14,0x08,0x3E,0x08,0x14}, /* '*' */
  {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
  {0x00,0x50,0x30,0x00,0x00}, /* ',' */
  {0x08,0x08,0x08,0x08,0x08}, /* '-' */
  {0x00,0x60,0x60,0x00,0x00}, /* '.' */
  {0x20,0x10,0x08,0x04,0x02}, /* '/' */
  {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
  {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
  {0x42,0x61,0x51,0x49,0x46}, /* '2' */
  {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
  {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
  {0x27,0x45,0x45,0x45,0x39}, /* '5' */
  {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
  {0x01,0x71,0x09,0x05,0x03}, /* '7' */
  {0x36,0x49,0x49,0x49,0x36}, /* '8' */
  {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
  {0x00,0x36,0x36,0x00,0x00}, /* ':' */
  {0x00,0x56,0x36,0x00,0x00}, /* ';' */
  {0x08,0x14,0x22,0x41,0x00}, /* '<' */
  {0x14,0x14,0x14,0x14,0x14}, /* '=' */
  {0x00,0x41,0x22,0x14,0x08}, /* '>' */
  {0x02,0x01,0x51,0x09,0x06}, /* '?' */
  {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
  {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
  {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
  {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
  {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
  {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
  {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
  {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
  {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
  {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
  {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
  {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
  {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
  {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' */
  {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
  {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
  {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
  {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
  {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
  {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
  {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
  {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
  {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
  {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
  {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
  {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
  {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
  {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
  {0x02,0x04,0x08,0x10,0x20}, /* '\' */
  {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
  {0x04,0x02,0x01,0x02,0x04}, /* '^' */
  {0x40,0x40,0x40,0x40,0x40}, /* '_' */
  {0x00,0x01,0x02,0x04,0x00}, /* '`' */
  {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
  {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
  {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
  {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
  {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
  {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
  {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' */
  {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
  {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
  {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
  {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
  {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
  {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
  {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
  {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
  {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
  {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
  {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
  {0x48,0x54,0x54,0x54,0x20}, /* 's' */
  {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
  {0x3C,0x40,0x40,0x40,0x7C}, /* 'u' */
  {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
  {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
  {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
  {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
  {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
  {0x00,0x08,0x36,0x41,0x00}, /* '{' */
  {0x00,0x00,0x7F,0x00,0x00}, /* '|' */
  {0x00,0x41,0x36,0x08,0x00}, /* '}' */
  {0x10,0x08,0x08,0x10,0x08}, /* '~' */
};

/* ============================================================================
 * Internal line-buffer (one scan-line worth of RGB565, big-endian byte order)
 * ========================================================================== */
static uint8_t s_line_buf[OLED_WIDTH * 2U];  /* 280 * 2 = 560 bytes */

/* ============================================================================
 * Low-level GPIO helpers
 * ========================================================================== */
static inline void disp_cs_low(void)
{
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
}
static inline void disp_cs_high(void)
{
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}
static inline void disp_dc_cmd(void)   /* LOW = command */
{
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET);
}
static inline void disp_dc_data(void)  /* HIGH = data */
{
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);
}
static inline void disp_res_low(void)
{
    HAL_GPIO_WritePin(OLED_RES_PORT, OLED_RES_PIN, GPIO_PIN_RESET);
}
static inline void disp_res_high(void)
{
    HAL_GPIO_WritePin(OLED_RES_PORT, OLED_RES_PIN, GPIO_PIN_SET);
}

/* ============================================================================
 * SPI bit-bang transmit (MSB first)
 * If OLED_USE_HW_SPI is 1, switch to HAL_SPI_Transmit.
 * ========================================================================== */
/* GPIO bit-bang SPI removed - using hardware SPI2 (PB13=SCK, PB15=MOSI) */
/* Declare hspi2 from spi.c / main.c */
extern SPI_HandleTypeDef hspi2;
#define OLED_SPI_HANDLE  (&hspi2)
#define OLED_SPI_TIMEOUT 1000U

static void spi_write_byte(uint8_t byte)
{
    HAL_SPI_Transmit(OLED_SPI_HANDLE, &byte, 1, OLED_SPI_TIMEOUT);
}
static void spi_write_buf(const uint8_t *buf, uint16_t len)
{
    HAL_SPI_Transmit(OLED_SPI_HANDLE, (uint8_t *)buf, len, OLED_SPI_TIMEOUT);
}

/* ============================================================================
 * ST7789 register-level helpers
 * ========================================================================== */
static void st7789_write_cmd(uint8_t cmd)
{
    disp_dc_cmd();
    disp_cs_low();
    spi_write_byte(cmd);
    disp_cs_high();
}

static void st7789_write_data8(uint8_t data)
{
    disp_dc_data();
    disp_cs_low();
    spi_write_byte(data);
    disp_cs_high();
}

static void st7789_write_data_buf(const uint8_t *buf, uint16_t len)
{
    disp_dc_data();
    disp_cs_low();
    spi_write_buf(buf, len);
    disp_cs_high();
}

/* Set address window in landscape mode (includes Waveshare 20-px X offset) */
static void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4];

    /* Apply X-offset for Waveshare 1.69" panel */
    x1 = (uint16_t)(x1 + WS169_X_OFFSET);
    x2 = (uint16_t)(x2 + WS169_X_OFFSET);

    st7789_write_cmd(ST7789_CASET);
    data[0] = (uint8_t)(x1 >> 8U); data[1] = (uint8_t)(x1 & 0xFFU);
    data[2] = (uint8_t)(x2 >> 8U); data[3] = (uint8_t)(x2 & 0xFFU);
    st7789_write_data_buf(data, 4U);

    st7789_write_cmd(ST7789_RASET);
    data[0] = (uint8_t)(y1 >> 8U); data[1] = (uint8_t)(y1 & 0xFFU);
    data[2] = (uint8_t)(y2 >> 8U); data[3] = (uint8_t)(y2 & 0xFFU);
    st7789_write_data_buf(data, 4U);

    st7789_write_cmd(ST7789_RAMWR);
}

/* Write a single RGB565 pixel (big-endian, 2 bytes) */
static void st7789_write_pixel(uint16_t color)
{
    uint8_t px[2] = { (uint8_t)(color >> 8U), (uint8_t)(color & 0xFFU) };
    st7789_write_data_buf(px, 2U);
}

/* ============================================================================
 * ST7789V2 initialisation sequence (Waveshare 1.69" landscape)
 * ========================================================================== */
static void st7789_init_sequence(void)
{
    /* Hardware reset */
    disp_res_high();
    HAL_Delay(10U);
    disp_res_low();
    HAL_Delay(10U);
    disp_res_high();
    HAL_Delay(120U);

    st7789_write_cmd(ST7789_SWRESET);
    HAL_Delay(150U);

    st7789_write_cmd(ST7789_SLPOUT);
    HAL_Delay(10U);

    /* Colour mode: 16-bit RGB565 */
    st7789_write_cmd(ST7789_COLMOD);
    st7789_write_data8(0x55U);
    HAL_Delay(10U);

    /* MADCTL: landscape rotation (90°) – MX | MV, no BGR */
    st7789_write_cmd(ST7789_MADCTL);
    st7789_write_data8(ST7789_MADCTL_MX | ST7789_MADCTL_MV);

    /* Porch control */
    st7789_write_cmd(ST7789_PORCTRL);
    st7789_write_data8(0x0CU); st7789_write_data8(0x0CU);
    st7789_write_data8(0x00U); st7789_write_data8(0x33U);
    st7789_write_data8(0x33U);

    /* Gate control */
    st7789_write_cmd(ST7789_GCTRL);
    st7789_write_data8(0x35U);

    /* VCOM */
    st7789_write_cmd(ST7789_VCOMS);
    st7789_write_data8(0x19U);

    /* LCM control */
    st7789_write_cmd(ST7789_LCMCTRL);
    st7789_write_data8(0x2CU);

    /* VDV & VRH enable */
    st7789_write_cmd(ST7789_VDVVRHEN);
    st7789_write_data8(0x01U);

    /* VRH */
    st7789_write_cmd(ST7789_VRHS);
    st7789_write_data8(0x12U);

    /* VDV */
    st7789_write_cmd(ST7789_VDVS);
    st7789_write_data8(0x20U);

    /* Frame rate: 60 Hz */
    st7789_write_cmd(ST7789_FRCTRL2);
    st7789_write_data8(0x0FU);

    /* Power control 1 */
    st7789_write_cmd(ST7789_PWCTRL1);
    st7789_write_data8(0xA4U); st7789_write_data8(0xA1U);

    /* Positive gamma */
    st7789_write_cmd(ST7789_PVGAMCTRL);
    st7789_write_data8(0xD0U); st7789_write_data8(0x04U);
    st7789_write_data8(0x0DU); st7789_write_data8(0x11U);
    st7789_write_data8(0x13U); st7789_write_data8(0x2BU);
    st7789_write_data8(0x3FU); st7789_write_data8(0x54U);
    st7789_write_data8(0x4CU); st7789_write_data8(0x18U);
    st7789_write_data8(0x0DU); st7789_write_data8(0x0BU);
    st7789_write_data8(0x1FU); st7789_write_data8(0x23U);

    /* Negative gamma */
    st7789_write_cmd(ST7789_NVGAMCTRL);
    st7789_write_data8(0xD0U); st7789_write_data8(0x04U);
    st7789_write_data8(0x0CU); st7789_write_data8(0x11U);
    st7789_write_data8(0x13U); st7789_write_data8(0x2CU);
    st7789_write_data8(0x3FU); st7789_write_data8(0x44U);
    st7789_write_data8(0x51U); st7789_write_data8(0x2FU);
    st7789_write_data8(0x1FU); st7789_write_data8(0x1FU);
    st7789_write_data8(0x20U); st7789_write_data8(0x23U);

    /* Inversion ON (required by ST7789 for correct colour) */
    st7789_write_cmd(ST7789_INVON);

    st7789_write_cmd(ST7789_NORON);
    HAL_Delay(10U);

    st7789_write_cmd(ST7789_DISPON);
    HAL_Delay(10U);
}

/* ============================================================================
 * Public API
 * ========================================================================== */

/**
 * @brief  Initialise the Waveshare 1.69" LCD.
 */
HAL_StatusTypeDef OLED_Init(OLED_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c)
{
    (void)hi2c;  /* unused – the display is SPI */

    if (handle == NULL) {
        return HAL_ERROR;
    }

    handle->fg_color  = OLED_COLOR_WHITE;
    handle->bg_color  = OLED_COLOR_BLACK;
    handle->cursor_x  = 0U;
    handle->cursor_y  = 0U;
    handle->hi2c      = NULL;

    st7789_init_sequence();

    /* Clear to black */
    (void)OLED_Clear(handle);

    return HAL_OK;
}

/**
 * @brief  Send DISPON command.
 */
HAL_StatusTypeDef OLED_DisplayOn(OLED_HandleTypeDef *handle)
{
    (void)handle;
    st7789_write_cmd(ST7789_DISPON);
    return HAL_OK;
}

/**
 * @brief  Send DISPOFF command.
 */
HAL_StatusTypeDef OLED_DisplayOff(OLED_HandleTypeDef *handle)
{
    (void)handle;
    st7789_write_cmd(ST7789_DISPOFF);
    return HAL_OK;
}

/**
 * @brief  Fill the display with black.
 */
HAL_StatusTypeDef OLED_Clear(OLED_HandleTypeDef *handle)
{
    uint16_t bg = (handle != NULL) ? handle->bg_color : OLED_COLOR_BLACK;
    uint8_t  hi = (uint8_t)(bg >> 8U);
    uint8_t  lo = (uint8_t)(bg & 0xFFU);

    /* Pre-fill the line buffer */
    for (uint16_t x = 0U; x < OLED_WIDTH; x++) {
        s_line_buf[(2U * x)]      = hi;
        s_line_buf[(2U * x) + 1U] = lo;
    }

    st7789_set_window(0U, 0U, OLED_WIDTH - 1U, OLED_HEIGHT - 1U);

    disp_dc_data();
    disp_cs_low();
    for (uint16_t y = 0U; y < OLED_HEIGHT; y++) {
        spi_write_buf(s_line_buf, (uint16_t)sizeof(s_line_buf));
    }
    disp_cs_high();

    return HAL_OK;
}

/**
 * @brief  No-op: this driver writes directly to the display — no frame buffer.
 *         Kept for API compatibility with app_threadx.c.
 */
HAL_StatusTypeDef OLED_UpdateDisplay(OLED_HandleTypeDef *handle)
{
    (void)handle;
    return HAL_OK;
}

/**
 * @brief  Draw a single pixel.
 */
void OLED_SetPixel(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint8_t state)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) return;

    uint16_t color = (handle != NULL)
                   ? (state ? handle->fg_color : handle->bg_color)
                   : (state ? OLED_COLOR_WHITE  : OLED_COLOR_BLACK);

    st7789_set_window(x, y, x, y);
    st7789_write_pixel(color);
}

/**
 * @brief  Draw a character from the 5×8 font.
 * @param  x    Pixel x.
 * @param  y    Pixel y (top of character).
 * @param  ch   ASCII character.
 */
static void draw_char(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, char ch)
{
    if ((ch < ' ') || (ch > '~')) return;
    if ((x + OLED_FONT_W > OLED_WIDTH) || (y + OLED_FONT_H > OLED_HEIGHT)) return;

    uint8_t  idx  = (uint8_t)((uint8_t)ch - (uint8_t)' ');
    uint16_t fg   = (handle != NULL) ? handle->fg_color : OLED_COLOR_WHITE;
    uint16_t bg   = (handle != NULL) ? handle->bg_color : OLED_COLOR_BLACK;

    /* Build a 5×8-pixel block in the line buffer then flush column by column */
    for (uint8_t col = 0U; col < OLED_FONT_W; col++) {
        uint8_t font_col = s_font5x8[idx][col];

        /* Set address window for this column strip (1 pixel wide, 8 tall) */
        st7789_set_window(x + col, y, x + col, y + OLED_FONT_H - 1U);
        disp_dc_data();
        disp_cs_low();
        for (uint8_t row = 0U; row < OLED_FONT_H; row++) {
            uint16_t color = (font_col & (1U << row)) ? fg : bg;
            uint8_t  px[2] = { (uint8_t)(color >> 8U), (uint8_t)(color & 0xFFU) };
            spi_write_buf(px, 2U);
        }
        disp_cs_high();
    }
    /* One blank column gap */
    st7789_set_window(x + OLED_FONT_W, y, x + OLED_FONT_W, y + OLED_FONT_H - 1U);
    disp_dc_data();
    disp_cs_low();
    for (uint8_t row = 0U; row < OLED_FONT_H; row++) {
        uint16_t color = bg;
        uint8_t  px[2] = { (uint8_t)(color >> 8U), (uint8_t)(color & 0xFFU) };
        spi_write_buf(px, 2U);
    }
    disp_cs_high();
}

/**
 * @brief  Print an ASCII string.
 * @param  x    Pixel x of the first character.
 * @param  y    Character row (0-based; each row = OLED_FONT_H pixels).
 * @param  str  Null-terminated string.
 */
void OLED_PrintStr(OLED_HandleTypeDef *handle, uint16_t x, uint8_t y, const char *str)
{
    uint16_t pixel_y = (uint16_t)y * (uint16_t)OLED_FONT_H;
    uint16_t px      = x;

    if (str == NULL) return;

    while (*str != '\0') {
        if ((px + (OLED_FONT_W + 1U)) > OLED_WIDTH) break;
        if (pixel_y + OLED_FONT_H > OLED_HEIGHT)    break;

        draw_char(handle, px, pixel_y, *str);
        px += (OLED_FONT_W + 1U);
        str++;
    }
}

/**
 * @brief  Draw a horizontal line (foreground colour).
 */
void OLED_DrawHLine(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t width)
{
    if ((y >= OLED_HEIGHT) || (x >= OLED_WIDTH)) return;
    if (x + width > OLED_WIDTH) width = (uint16_t)(OLED_WIDTH - x);
    if (width == 0U) return;

    uint16_t fg = (handle != NULL) ? handle->fg_color : OLED_COLOR_WHITE;
    uint8_t  hi = (uint8_t)(fg >> 8U);
    uint8_t  lo = (uint8_t)(fg & 0xFFU);

    for (uint16_t i = 0U; i < width; i++) {
        s_line_buf[(2U * i)]      = hi;
        s_line_buf[(2U * i) + 1U] = lo;
    }

    st7789_set_window(x, y, (uint16_t)(x + width - 1U), y);
    st7789_write_data_buf(s_line_buf, (uint16_t)(width * 2U));
}

/**
 * @brief  Draw a vertical line (foreground colour).
 */
void OLED_DrawVLine(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t height)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) return;
    if (y + height > OLED_HEIGHT) height = (uint16_t)(OLED_HEIGHT - y);
    if (height == 0U) return;

    uint16_t fg = (handle != NULL) ? handle->fg_color : OLED_COLOR_WHITE;
    uint8_t  px[2] = { (uint8_t)(fg >> 8U), (uint8_t)(fg & 0xFFU) };

    st7789_set_window(x, y, x, (uint16_t)(y + height - 1U));
    disp_dc_data();
    disp_cs_low();
    for (uint16_t i = 0U; i < height; i++) {
        spi_write_buf(px, 2U);
    }
    disp_cs_high();
}

/**
 * @brief  Draw a rectangle outline.
 */
void OLED_DrawRect(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    OLED_DrawHLine(handle, x, y,                     width);
    OLED_DrawHLine(handle, x, (uint16_t)(y + height - 1U), width);
    OLED_DrawVLine(handle, x,                     y, height);
    OLED_DrawVLine(handle, (uint16_t)(x + width - 1U), y, height);
}

/**
 * @brief  Fill a rectangle with a given RGB565 colour.
 */
void OLED_FillRect(OLED_HandleTypeDef *handle, uint16_t x, uint16_t y,
                   uint16_t width, uint16_t height, uint16_t color)
{
    (void)handle;
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) return;
    if (x + width  > OLED_WIDTH)  width  = (uint16_t)(OLED_WIDTH  - x);
    if (y + height > OLED_HEIGHT) height = (uint16_t)(OLED_HEIGHT - y);
    if ((width == 0U) || (height == 0U)) return;

    uint8_t hi = (uint8_t)(color >> 8U);
    uint8_t lo = (uint8_t)(color & 0xFFU);

    for (uint16_t i = 0U; i < width; i++) {
        s_line_buf[(2U * i)]      = hi;
        s_line_buf[(2U * i) + 1U] = lo;
    }

    st7789_set_window(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
    disp_dc_data();
    disp_cs_low();
    for (uint16_t row = 0U; row < height; row++) {
        spi_write_buf(s_line_buf, (uint16_t)(width * 2U));
    }
    disp_cs_high();
}
