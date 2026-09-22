#include "unified.h"
#include "tx_api.h"
#include <string.h>

/*
 * Expected GPIO names from CubeMX/main.h:
 *   DISP_CS_GPIO_Port, DISP_CS_Pin  -> LCD CS
 *   DISP_DC_GPIO_Port, DISP_DC_Pin              -> LCD D/C or RS
 *   DISP_RES_GPIO_Port, DISP_RES_Pin            -> LCD RESET
 * SPI2 is used for SCK/MOSI.
 */

#if (DISPLAY_SELECTED == DISPLAY_MODEL_E20RA_FW600_N)
#define DISP_CTRL DISPLAY_CONTROLLER_ILI9342
#else
#define DISP_CTRL DISPLAY_CONTROLLER_ST7789
#endif

#if (DISPLAY_SELECTED == DISPLAY_MODEL_EA_TFT015_22AI) || \
    (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
#define DISP_ST7789_COLMOD 0x05U
#else
#define DISP_ST7789_COLMOD 0x55U
#endif

static inline void disp_cs_low(void)
{
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_RESET);
}

static inline void disp_cs_high(void)
{
    HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, GPIO_PIN_SET);
}

static inline void disp_dc_command(void)
{
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_RESET);
}

static inline void disp_dc_data(void)
{
    HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, GPIO_PIN_SET);
}

static void Display_WriteCommand(uint8_t cmd)
{
    disp_dc_command();
    disp_cs_low();
    (void)HAL_SPI_Transmit(DISPLAY_SPI_HANDLE, &cmd, 1U, DISPLAY_SPI_CMD_TIMEOUT);
    disp_cs_high();
}

static void Display_WriteData8(uint8_t data)
{
    disp_dc_data();
    disp_cs_low();
    (void)HAL_SPI_Transmit(DISPLAY_SPI_HANDLE, &data, 1U, DISPLAY_SPI_CMD_TIMEOUT);
    disp_cs_high();
}

static void Display_WriteDataBuffer(const uint8_t *data, uint32_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return;
    }

    disp_dc_data();
    disp_cs_low();

    while (size > 0U)
    {
        uint16_t chunk = (size > 0xFFFFU) ? 0xFFFFU : (uint16_t)size;
        (void)HAL_SPI_Transmit(DISPLAY_SPI_HANDLE, (uint8_t *)data, chunk, DISPLAY_SPI_DATA_TIMEOUT);
        data += chunk;
        size -= chunk;
    }

    disp_cs_high();
}

void Display_Reset(void)
{
    disp_cs_high();

#if (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
    /* Waveshare reference reset: HIGH 100 ms -> LOW 100 ms -> HIGH 100 ms */
    HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
#else
    HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(150);
#endif
}

static uint8_t ST7789_Madctl(DisplayRotation_t rotation)
{
#if (DISPLAY_SELECTED == DISPLAY_MODEL_NHD_15_240240)
    /* This value was used for the working NHD bring-up. */
    if (rotation == DISPLAY_ROT_0) return 0x10U;
#endif

#if (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
    /* Waveshare 1.69 reference values used by this project:
     * Portrait   = 0x00
     * Landscape  = 0x60 (MX | MV)
     * Keep RGB order. Do not set BGR for this module unless colours prove swapped.
     */
    switch (rotation)
    {
        case DISPLAY_ROT_0:   return 0x00U;
        case DISPLAY_ROT_90:  return 0x60U;
        case DISPLAY_ROT_180: return 0xC0U;
        case DISPLAY_ROT_270: return 0xA0U;
        default:              return 0x00U;
    }
#endif

    uint8_t madctl;

    switch (rotation)
    {
        case DISPLAY_ROT_0:   madctl = 0x00U; break;
        case DISPLAY_ROT_90:  madctl = 0x60U; break;
        case DISPLAY_ROT_180: madctl = 0xC0U; break;
        case DISPLAY_ROT_270: madctl = 0xA0U; break;
        default:              madctl = 0x00U; break;
    }

#if DISPLAY_ST7789_BGR
    madctl |= 0x08U; /* BGR bit. Enable only if red/blue are swapped. */
#endif

    return madctl;
}

static uint8_t ILI9342_Madctl(DisplayRotation_t rotation)
{
    /* ILI934x style bits: MY=0x80, MX=0x40, MV=0x20, BGR=0x08. */
    switch (rotation)
    {
        case DISPLAY_ROT_0:   return 0x28U; /* Landscape 320x240, BGR */
        case DISPLAY_ROT_90:  return 0x88U;
        case DISPLAY_ROT_180: return 0xE8U;
        case DISPLAY_ROT_270: return 0x48U;
        default:              return 0x28U;
    }
}

static void ST7789_InitSequence(DisplayRotation_t rotation)
{
    Display_Reset();

#if (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
    /* Waveshare 1.69 inch LCD Module, ST7789V2, 240x280.
     * This is the Waveshare-specific sequence used in their reference driver.
     * The earlier unified ZIP used the generic ST7789 sequence, which can give
     * unclear/incorrect rendering on this module.
     */

    Display_WriteCommand(0x36);       /* MADCTL */
    Display_WriteData8(ST7789_Madctl(rotation));

    Display_WriteCommand(0x3A);       /* COLMOD: RGB565 */
    Display_WriteData8(0x55);

    Display_WriteCommand(0xB2);       /* PORCTRL */
    Display_WriteData8(0x0B);
    Display_WriteData8(0x0B);
    Display_WriteData8(0x00);
    Display_WriteData8(0x33);
    Display_WriteData8(0x35);

    Display_WriteCommand(0xB7);       /* GCTRL */
    Display_WriteData8(0x11);

    Display_WriteCommand(0xBB);       /* VCOMS */
    Display_WriteData8(0x35);

    Display_WriteCommand(0xC0);       /* LCMCTRL */
    Display_WriteData8(0x2C);

    Display_WriteCommand(0xC2);       /* VDV/VRH enable */
    Display_WriteData8(0x01);

    Display_WriteCommand(0xC3);       /* VRHS */
    Display_WriteData8(0x0D);

    Display_WriteCommand(0xC4);       /* VDVS */
    Display_WriteData8(0x20);

    Display_WriteCommand(0xC6);       /* FRCTRL2 */
    Display_WriteData8(0x13);

    Display_WriteCommand(0xD0);       /* PWCTRL1 */
    Display_WriteData8(0xA4);
    Display_WriteData8(0xA1);

    Display_WriteCommand(0xD6);       /* PWCTRL2 / vendor tuning */
    Display_WriteData8(0xA1);

    Display_WriteCommand(0xE0);       /* Positive gamma */
    Display_WriteData8(0xF0);
    Display_WriteData8(0x06);
    Display_WriteData8(0x0B);
    Display_WriteData8(0x0A);
    Display_WriteData8(0x09);
    Display_WriteData8(0x26);
    Display_WriteData8(0x29);
    Display_WriteData8(0x33);
    Display_WriteData8(0x41);
    Display_WriteData8(0x18);
    Display_WriteData8(0x16);
    Display_WriteData8(0x15);
    Display_WriteData8(0x29);
    Display_WriteData8(0x2D);

    Display_WriteCommand(0xE1);       /* Negative gamma */
    Display_WriteData8(0xF0);
    Display_WriteData8(0x04);
    Display_WriteData8(0x08);
    Display_WriteData8(0x08);
    Display_WriteData8(0x07);
    Display_WriteData8(0x03);
    Display_WriteData8(0x28);
    Display_WriteData8(0x32);
    Display_WriteData8(0x40);
    Display_WriteData8(0x3B);
    Display_WriteData8(0x19);
    Display_WriteData8(0x18);
    Display_WriteData8(0x2A);
    Display_WriteData8(0x2E);

    Display_WriteCommand(0xE4);       /* Vendor tuning */
    Display_WriteData8(0x25);
    Display_WriteData8(0x00);
    Display_WriteData8(0x00);

    Display_WriteCommand(0x21);       /* INVON: required for this normally-black IPS module */

    Display_WriteCommand(0x11);       /* Sleep OUT */
    HAL_Delay(120);

    Display_SetAddressWindow(0, 0, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U);

    Display_WriteCommand(0x29);       /* Display ON */
    HAL_Delay(100);

#else
    Display_WriteCommand(0x01);       /* Software reset */
    HAL_Delay(200);

    Display_WriteCommand(0x11);       /* Sleep out */
    HAL_Delay(120);

    Display_WriteCommand(0x36);       /* MADCTL */
    Display_WriteData8(ST7789_Madctl(rotation));

    Display_WriteCommand(0x3A);       /* Pixel format */
    Display_WriteData8(DISP_ST7789_COLMOD);  /* 16-bit RGB565; EA/Waveshare accepts 0x05 */

    Display_WriteCommand(0x21);       /* Display inversion ON */

    Display_WriteCommand(0xB2);       /* Porch setting */
    Display_WriteData8(0x05); Display_WriteData8(0x05); Display_WriteData8(0x00);
    Display_WriteData8(0x33); Display_WriteData8(0x33);

    Display_WriteCommand(0xB7);       /* Gate control */
    Display_WriteData8(0x35);

#if (DISPLAY_SELECTED == DISPLAY_MODEL_EA_TFT015_22AI)
    Display_WriteCommand(0xB8);
    Display_WriteData8(0x2F); Display_WriteData8(0x2B); Display_WriteData8(0x2F);
#endif

    Display_WriteCommand(0xBB);       /* VCOM */
    Display_WriteData8(0x2B);

    Display_WriteCommand(0xC0);       /* LCM control */
    Display_WriteData8(0x2C);

    Display_WriteCommand(0xC2);       /* VDV/VRH enable */
    Display_WriteData8(0x01);

    Display_WriteCommand(0xC3);       /* VRH */
    Display_WriteData8(0x0B);

    Display_WriteCommand(0xC4);       /* VDV */
    Display_WriteData8(0x20);

    Display_WriteCommand(0xC6);       /* Frame rate */
    Display_WriteData8(0x11);

    Display_WriteCommand(0xD0);       /* Power control */
    Display_WriteData8(0xA4); Display_WriteData8(0xA1);

#if (DISPLAY_SELECTED == DISPLAY_MODEL_EA_TFT015_22AI)
    Display_WriteCommand(0xE8);
    Display_WriteData8(0x03);

    Display_WriteCommand(0xE9);
    Display_WriteData8(0x0D); Display_WriteData8(0x12); Display_WriteData8(0x00);
#endif

    Display_WriteCommand(0xE0);       /* Positive gamma */
    Display_WriteData8(0xD0); Display_WriteData8(0x06); Display_WriteData8(0x0B);
    Display_WriteData8(0x0A); Display_WriteData8(0x09); Display_WriteData8(0x05);
    Display_WriteData8(0x2E); Display_WriteData8(0x43); Display_WriteData8(0x44);
    Display_WriteData8(0x09); Display_WriteData8(0x16); Display_WriteData8(0x15);
    Display_WriteData8(0x23); Display_WriteData8(0x27);

    Display_WriteCommand(0xE1);       /* Negative gamma */
    Display_WriteData8(0xD0); Display_WriteData8(0x06); Display_WriteData8(0x0B);
    Display_WriteData8(0x09); Display_WriteData8(0x08); Display_WriteData8(0x06);
    Display_WriteData8(0x2E); Display_WriteData8(0x44); Display_WriteData8(0x44);
    Display_WriteData8(0x3A); Display_WriteData8(0x15); Display_WriteData8(0x15);
    Display_WriteData8(0x23); Display_WriteData8(0x26);

    Display_SetAddressWindow(0, 0, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U);

    Display_WriteCommand(0x13);       /* Normal display mode ON */
    HAL_Delay(10);

    Display_WriteCommand(0x29);       /* Display ON */
    HAL_Delay(100);
#endif
}

__attribute__((unused)) static void ILI9342_InitSequence(DisplayRotation_t rotation)
{
    Display_Reset();

    Display_WriteCommand(0x01);       /* Software reset */
    HAL_Delay(150);

    Display_WriteCommand(0x28);       /* Display OFF */

    Display_WriteCommand(0xCF);
    Display_WriteData8(0x00); Display_WriteData8(0xC1); Display_WriteData8(0x30);

    Display_WriteCommand(0xED);
    Display_WriteData8(0x64); Display_WriteData8(0x03); Display_WriteData8(0x12); Display_WriteData8(0x81);

    Display_WriteCommand(0xE8);
    Display_WriteData8(0x85); Display_WriteData8(0x00); Display_WriteData8(0x78);

    Display_WriteCommand(0xCB);
    Display_WriteData8(0x39); Display_WriteData8(0x2C); Display_WriteData8(0x00); Display_WriteData8(0x34); Display_WriteData8(0x02);

    Display_WriteCommand(0xF7);
    Display_WriteData8(0x20);

    Display_WriteCommand(0xEA);
    Display_WriteData8(0x00); Display_WriteData8(0x00);

    Display_WriteCommand(0xC0);       /* Power control */
    Display_WriteData8(0x23);

    Display_WriteCommand(0xC1);       /* Power control */
    Display_WriteData8(0x10);

    Display_WriteCommand(0xC5);       /* VCOM control */
    Display_WriteData8(0x3E); Display_WriteData8(0x28);

    Display_WriteCommand(0xC7);       /* VCOM control 2 */
    Display_WriteData8(0x86);

    Display_WriteCommand(0x36);       /* MADCTL */
    Display_WriteData8(ILI9342_Madctl(rotation));

    Display_WriteCommand(0x3A);       /* RGB565 */
    Display_WriteData8(0x55);

    Display_WriteCommand(0xB1);       /* Frame rate */
    Display_WriteData8(0x00); Display_WriteData8(0x1B);

    Display_WriteCommand(0xB6);       /* Display function control */
    Display_WriteData8(0x0A); Display_WriteData8(0xA2);

    Display_WriteCommand(0xF2);
    Display_WriteData8(0x00);

    Display_WriteCommand(0x26);
    Display_WriteData8(0x01);

    Display_WriteCommand(0xE0);       /* Positive gamma */
    Display_WriteData8(0x0F); Display_WriteData8(0x31); Display_WriteData8(0x2B);
    Display_WriteData8(0x0C); Display_WriteData8(0x0E); Display_WriteData8(0x08);
    Display_WriteData8(0x4E); Display_WriteData8(0xF1); Display_WriteData8(0x37);
    Display_WriteData8(0x07); Display_WriteData8(0x10); Display_WriteData8(0x03);
    Display_WriteData8(0x0E); Display_WriteData8(0x09); Display_WriteData8(0x00);

    Display_WriteCommand(0xE1);       /* Negative gamma */
    Display_WriteData8(0x00); Display_WriteData8(0x0E); Display_WriteData8(0x14);
    Display_WriteData8(0x03); Display_WriteData8(0x11); Display_WriteData8(0x07);
    Display_WriteData8(0x31); Display_WriteData8(0xC1); Display_WriteData8(0x48);
    Display_WriteData8(0x08); Display_WriteData8(0x0F); Display_WriteData8(0x0C);
    Display_WriteData8(0x31); Display_WriteData8(0x36); Display_WriteData8(0x0F);

    Display_WriteCommand(0x11);       /* Sleep OUT */
    HAL_Delay(120);

    Display_SetAddressWindow(0, 0, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U);

    Display_WriteCommand(0x29);       /* Display ON */
    HAL_Delay(100);
}


static uint16_t Display_GetXOffset(DisplayRotation_t rotation)
{
#if (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
    /* Waveshare 1.69 has 240x280 active area inside 240x320 ST7789V2 RAM. */
    if ((rotation == DISPLAY_ROT_90) || (rotation == DISPLAY_ROT_270))
    {
        return 20U;
    }
    return 0U;
#else
    (void)rotation;
    return 0U;
#endif
}

static uint16_t Display_GetYOffset(DisplayRotation_t rotation)
{
#if (DISPLAY_SELECTED == DISPLAY_MODEL_WAVESHARE_169_LCD)
    /* Portrait mode needs 20-pixel Y offset because panel uses only 280 of 320 RAM rows. */
    if ((rotation == DISPLAY_ROT_0) || (rotation == DISPLAY_ROT_180))
    {
        return 20U;
    }
    return 0U;
#else
    (void)rotation;
    return 0U;
#endif
}

static DisplayRotation_t s_displayRotation = DISPLAY_DEFAULT_ROTATION;

void Display_Init(DisplayRotation_t rotation)
{
    s_displayRotation = rotation;

#if (DISPLAY_SELECTED == DISPLAY_MODEL_E20RA_FW600_N)
    ILI9342_InitSequence(rotation);
#else
    ST7789_InitSequence(rotation);
#endif
}

void Display_SetAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4];

    uint16_t x_offset = Display_GetXOffset(s_displayRotation);
    uint16_t y_offset = Display_GetYOffset(s_displayRotation);

    x1 = (uint16_t)(x1 + x_offset);
    x2 = (uint16_t)(x2 + x_offset);
    y1 = (uint16_t)(y1 + y_offset);
    y2 = (uint16_t)(y2 + y_offset);

    Display_WriteCommand(0x2A);       /* Column address set */
    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1 & 0xFFU);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2 & 0xFFU);
    Display_WriteDataBuffer(data, 4U);

    Display_WriteCommand(0x2B);       /* Page address set */
    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1 & 0xFFU);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2 & 0xFFU);
    Display_WriteDataBuffer(data, 4U);

    Display_WriteCommand(0x2C);       /* Memory write */
}

void Display_FillScreenDirect(uint16_t color)
{
    static uint8_t line[DISPLAY_WIDTH * 2U];

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFFU);

    for (uint16_t x = 0U; x < DISPLAY_WIDTH; x++)
    {
        line[(2U * x)]     = hi;
        line[(2U * x) + 1U] = lo;
    }

    Display_SetAddressWindow(0, 0, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U);

    disp_dc_data();
    disp_cs_low();

    for (uint16_t y = 0U; y < DISPLAY_HEIGHT; y++)
    {
        (void)HAL_SPI_Transmit(DISPLAY_SPI_HANDLE, line, (uint16_t)sizeof(line), DISPLAY_SPI_DATA_TIMEOUT);
    }

    disp_cs_high();
}

/* ---- DMA pixel path ------------------------------------------------------
 * ST7789 wants each RGB565 pixel MSB first. In 16-bit SPI frames the MSB of the
 * frame goes out first, so the little-endian framebuffer can be streamed by DMA
 * straight from memory with no byte swapping. Commands use 8-bit frames.
 */
#define DISPLAY_DMA_MAX_PIXELS 32768U   /* HAL DMA size is uint16_t */

static TX_SEMAPHORE s_dmaDone;
static volatile uint8_t s_rtosReady = 0U;

void Display_RtosInit(void)
{
    if (tx_semaphore_create(&s_dmaDone, (CHAR *)"Display DMA", 0U) == TX_SUCCESS)
    {
        s_rtosReady = 1U;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if ((hspi == DISPLAY_SPI_HANDLE) && (s_rtosReady != 0U))
    {
        (void)tx_semaphore_put(&s_dmaDone);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if ((hspi == DISPLAY_SPI_HANDLE) && (s_rtosReady != 0U))
    {
        (void)tx_semaphore_put(&s_dmaDone);
    }
}

static void Display_SetSpiDataSize(uint32_t dataSize)
{
    if (DISPLAY_SPI_HANDLE->Init.DataSize != dataSize)
    {
        DISPLAY_SPI_HANDLE->Init.DataSize = dataSize;
        (void)HAL_SPI_Init(DISPLAY_SPI_HANDLE);
    }
}

static void Display_SendPixels(const uint16_t *pixels, uint16_t count)
{
    if (s_rtosReady == 0U)
    {
        /* Before the kernel runs (or if the semaphore failed): plain blocking transfer. */
        (void)HAL_SPI_Transmit(DISPLAY_SPI_HANDLE, (uint8_t *)pixels, count, DISPLAY_SPI_DATA_TIMEOUT);
        return;
    }

    if (HAL_SPI_Transmit_DMA(DISPLAY_SPI_HANDLE, (uint8_t *)pixels, count) == HAL_OK)
    {
        if (tx_semaphore_get(&s_dmaDone, ((DISPLAY_SPI_DATA_TIMEOUT * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U)) != TX_SUCCESS)
        {
            (void)HAL_SPI_Abort(DISPLAY_SPI_HANDLE);
        }
    }
}

/*
 * Push rows [y, y+height) to the panel. x/width are ignored on purpose: whole rows
 * are contiguous in the framebuffer, so one DMA burst needs no CPU copy.
 */
void Display_FlushRectRGB565(const uint16_t *framebuffer,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height)
{
    (void)x;
    (void)width;

    if ((framebuffer == NULL) || (y >= DISPLAY_HEIGHT) || (height == 0U))
    {
        return;
    }

    if ((y + height) > DISPLAY_HEIGHT)
    {
        height = (uint16_t)(DISPLAY_HEIGHT - y);
    }

    Display_SetAddressWindow(0U, y,
                             (uint16_t)(DISPLAY_WIDTH - 1U),
                             (uint16_t)(y + height - 1U));

    disp_dc_data();
    disp_cs_low();
    Display_SetSpiDataSize(SPI_DATASIZE_16BIT);

    const uint16_t *src = &framebuffer[(uint32_t)y * DISPLAY_WIDTH];
    uint32_t remaining = (uint32_t)height * DISPLAY_WIDTH;

    while (remaining > 0U)
    {
        uint16_t n = (remaining > DISPLAY_DMA_MAX_PIXELS) ? (uint16_t)DISPLAY_DMA_MAX_PIXELS
                                                          : (uint16_t)remaining;
        Display_SendPixels(src, n);
        src += n;
        remaining -= n;
    }

    Display_SetSpiDataSize(SPI_DATASIZE_8BIT);
    disp_cs_high();
}

void Display_TestLandscapePattern(void)
{
    static uint8_t line[280U * 2U];

    Display_SetAddressWindow(0, 0, 279, 239);

    disp_dc_data();
    disp_cs_low();

    for (uint16_t y = 0; y < 240; y++)
    {
        for (uint16_t x = 0; x < 280; x++)
        {
            uint16_t color;

            if (x < 70)
                color = DISPLAY_COLOR_RED;
            else if (x < 140)
                color = DISPLAY_COLOR_GREEN;
            else if (x < 210)
                color = DISPLAY_COLOR_BLUE;
            else
                color = DISPLAY_COLOR_YELLOW;

            /* White border */
            if ((x == 0) || (x == 279) ||
                (y == 0) || (y == 239))
            {
                color = DISPLAY_COLOR_WHITE;
            }

            line[(x * 2U)]     = (uint8_t)(color >> 8);
            line[(x * 2U) + 1] = (uint8_t)(color & 0xFFU);
        }

        HAL_SPI_Transmit(DISPLAY_SPI_HANDLE,
                         line,
                         sizeof(line),
                         DISPLAY_SPI_DATA_TIMEOUT);
    }

    disp_cs_high();
}

uint16_t Display_GetWidth(void)
{
    return DISPLAY_WIDTH;
}

uint16_t Display_GetHeight(void)
{
    return DISPLAY_HEIGHT;
}

DisplayController_t Display_GetController(void)
{
    return DISP_CTRL;
}
