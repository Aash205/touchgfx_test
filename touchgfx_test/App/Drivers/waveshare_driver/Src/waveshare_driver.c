/**
  ******************************************************************************
  * @file    waveshare_driver.c
  * @brief   Waveshare 1.69-inch ST7789V2 LCD driver.
  ******************************************************************************
  */

#include "waveshare_driver.h"
#include "tx_api.h"
#include <stddef.h>

#define WS169_CMD_SLEEP_IN       0x10U
#define WS169_CMD_SLEEP_OUT      0x11U
#define WS169_CMD_INVERSION_ON   0x21U
#define WS169_CMD_DISPLAY_OFF    0x28U
#define WS169_CMD_DISPLAY_ON     0x29U
#define WS169_CMD_COLUMN_ADDRESS 0x2AU
#define WS169_CMD_ROW_ADDRESS    0x2BU
#define WS169_CMD_MEMORY_WRITE   0x2CU
#define WS169_CMD_MADCTL         0x36U
#define WS169_CMD_PIXEL_FORMAT   0x3AU

#define WS169_SPI_DATA_8BIT      SPI_DATASIZE_8BIT
#define WS169_SPI_DATA_16BIT     SPI_DATASIZE_16BIT
#define WS169_DMA_MAX_PIXELS     65535U
#define WS169_MAX_ROW_BYTES      (WS169_LANDSCAPE_WIDTH * 2U)

typedef enum
{
    WS169_DMA_IDLE = 0,
    WS169_DMA_PENDING,
    WS169_DMA_COMPLETE,
    WS169_DMA_FAILED
} WS169_DmaState_t;

static WS169_Config_t s_config;
static bool s_config_valid;
static WS169_Rotation_t s_rotation = WS169_ROTATION_90;
static volatile WS169_Diagnostics_t s_diagnostics;
static volatile WS169_DmaState_t s_dma_state = WS169_DMA_IDLE;

static TX_SEMAPHORE s_dma_semaphore;
static TX_MUTEX s_bus_mutex;
static bool s_semaphore_ready;
static bool s_mutex_ready;

static uint32_t ws169_enter_critical(void)
{
    uint32_t interrupt_state = __get_PRIMASK();
    __disable_irq();
    return interrupt_state;
}

static void ws169_exit_critical(uint32_t interrupt_state)
{
    __set_PRIMASK(interrupt_state);
}

static void ws169_record_status(WS169_Status_t status, uint32_t hal_error)
{
    uint32_t interrupt_state = ws169_enter_critical();

    s_diagnostics.last_status = status;
    s_diagnostics.last_hal_error = hal_error;

    if ((status == WS169_STATUS_SPI_ERROR) || (status == WS169_STATUS_SPI_BUSY))
    {
        s_diagnostics.spi_error_count++;
    }
    else if (status == WS169_STATUS_DMA_ERROR)
    {
        s_diagnostics.dma_error_count++;
    }
    else if (status == WS169_STATUS_TIMEOUT)
    {
        s_diagnostics.timeout_count++;
    }
    else
    {
        /* No diagnostic counter applies. */
    }

    ws169_exit_critical(interrupt_state);
}

static void ws169_record_successful_transfer(void)
{
    uint32_t interrupt_state = ws169_enter_critical();
    s_diagnostics.successful_transfer_count++;
    ws169_exit_critical(interrupt_state);
}

static WS169_Status_t ws169_status_from_hal(HAL_StatusTypeDef hal_status, bool dma_transfer)
{
    WS169_Status_t status;
    uint32_t hal_error = 0U;

    if (s_config_valid && (s_config.spi != NULL))
    {
        hal_error = s_config.spi->ErrorCode;
    }

    switch (hal_status)
    {
        case HAL_OK:
            status = WS169_STATUS_OK;
            break;

        case HAL_BUSY:
            status = WS169_STATUS_SPI_BUSY;
            break;

        case HAL_TIMEOUT:
            status = WS169_STATUS_TIMEOUT;
            break;

        case HAL_ERROR:
        default:
            status = dma_transfer ? WS169_STATUS_DMA_ERROR : WS169_STATUS_SPI_ERROR;
            break;
    }

    if (status != WS169_STATUS_OK)
    {
        ws169_record_status(status, hal_error);
    }

    return status;
}

static ULONG ws169_timeout_ticks(void)
{
    uint64_t ticks = ((uint64_t)s_config.data_timeout_ms *
                      (uint64_t)TX_TIMER_TICKS_PER_SECOND + 999ULL) / 1000ULL;

    if (ticks == 0ULL)
    {
        ticks = 1ULL;
    }
    if (ticks > (uint64_t)0xFFFFFFFFUL)
    {
        ticks = (uint64_t)0xFFFFFFFFUL;
    }

    return (ULONG)ticks;
}

static WS169_Status_t ws169_lock(void)
{
    if ((!s_mutex_ready) || (tx_thread_identify() == NULL))
    {
        return WS169_STATUS_OK;
    }

    if (tx_mutex_get(&s_bus_mutex, ws169_timeout_ticks()) != TX_SUCCESS)
    {
        ws169_record_status(WS169_STATUS_RTOS_ERROR, 0U);
        return WS169_STATUS_RTOS_ERROR;
    }

    return WS169_STATUS_OK;
}

static void ws169_unlock(void)
{
    if (s_mutex_ready && (tx_thread_identify() != NULL))
    {
        (void)tx_mutex_put(&s_bus_mutex);
    }
}

static void ws169_cs_low(void)
{
    HAL_GPIO_WritePin(s_config.cs_port, s_config.cs_pin, GPIO_PIN_RESET);
}

static void ws169_cs_high(void)
{
    HAL_GPIO_WritePin(s_config.cs_port, s_config.cs_pin, GPIO_PIN_SET);
}

static void ws169_dc_command(void)
{
    HAL_GPIO_WritePin(s_config.dc_port, s_config.dc_pin, GPIO_PIN_RESET);
}

static void ws169_dc_data(void)
{
    HAL_GPIO_WritePin(s_config.dc_port, s_config.dc_pin, GPIO_PIN_SET);
}

static WS169_Status_t ws169_transmit_blocking(const uint8_t *data,
                                              uint32_t size,
                                              uint32_t timeout_ms)
{
    if ((data == NULL) || (size == 0U))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    while (size > 0U)
    {
        uint16_t chunk = (size > 0xFFFFU) ? 0xFFFFU : (uint16_t)size;
        HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(s_config.spi,
                                                       (uint8_t *)(uintptr_t)data,
                                                       chunk,
                                                       timeout_ms);
        WS169_Status_t status = ws169_status_from_hal(hal_status, false);

        if (status != WS169_STATUS_OK)
        {
            return status;
        }

        data += chunk;
        size -= chunk;
    }

    ws169_record_successful_transfer();
    return WS169_STATUS_OK;
}

static WS169_Status_t ws169_write_command(uint8_t command)
{
    WS169_Status_t status;

    ws169_dc_command();
    ws169_cs_low();
    status = ws169_transmit_blocking(&command, 1U, s_config.command_timeout_ms);
    ws169_cs_high();

    return status;
}

static WS169_Status_t ws169_write_data(const uint8_t *data, uint32_t size)
{
    WS169_Status_t status;

    ws169_dc_data();
    ws169_cs_low();
    status = ws169_transmit_blocking(data, size, s_config.data_timeout_ms);
    ws169_cs_high();

    return status;
}

static WS169_Status_t ws169_write_command_data(uint8_t command,
                                               const uint8_t *data,
                                               uint32_t size)
{
    WS169_Status_t status = ws169_write_command(command);

    if ((status == WS169_STATUS_OK) && (size > 0U))
    {
        status = ws169_write_data(data, size);
    }

    return status;
}

static WS169_Status_t ws169_set_spi_data_size(uint32_t data_size)
{
    uint32_t previous_data_size;
    HAL_StatusTypeDef hal_status;

    if (s_config.spi->Init.DataSize == data_size)
    {
        return WS169_STATUS_OK;
    }

    previous_data_size = s_config.spi->Init.DataSize;
    s_config.spi->Init.DataSize = data_size;
    hal_status = HAL_SPI_Init(s_config.spi);

    if (hal_status != HAL_OK)
    {
        s_config.spi->Init.DataSize = previous_data_size;
        (void)HAL_SPI_Init(s_config.spi);
        return ws169_status_from_hal(hal_status, false);
    }

    return WS169_STATUS_OK;
}

WS169_Status_t WS169_GetGeometry(WS169_Rotation_t rotation,
                                 uint16_t *width,
                                 uint16_t *height,
                                 uint8_t *madctl)
{
    if ((rotation >= WS169_ROTATION_COUNT) ||
        (width == NULL) || (height == NULL) || (madctl == NULL))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    switch (rotation)
    {
        case WS169_ROTATION_0:
            *width = WS169_PORTRAIT_WIDTH;
            *height = WS169_PORTRAIT_HEIGHT;
            *madctl = 0x00U;
            break;

        case WS169_ROTATION_90:
            *width = WS169_LANDSCAPE_WIDTH;
            *height = WS169_LANDSCAPE_HEIGHT;
            *madctl = 0x60U;
            break;

        case WS169_ROTATION_180:
            *width = WS169_PORTRAIT_WIDTH;
            *height = WS169_PORTRAIT_HEIGHT;
            *madctl = 0xC0U;
            break;

        case WS169_ROTATION_270:
            *width = WS169_LANDSCAPE_WIDTH;
            *height = WS169_LANDSCAPE_HEIGHT;
            *madctl = 0xA0U;
            break;

        default:
            return WS169_STATUS_INVALID_ARGUMENT;
    }

    return WS169_STATUS_OK;
}

WS169_Status_t WS169_TranslateWindow(WS169_Rotation_t rotation,
                                    uint16_t x1,
                                    uint16_t y1,
                                    uint16_t x2,
                                    uint16_t y2,
                                    WS169_Window_t *translated)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;
    uint16_t x_offset;
    uint16_t y_offset;
    WS169_Status_t status;

    if (translated == NULL)
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    status = WS169_GetGeometry(rotation, &width, &height, &madctl);
    (void)madctl;
    if (status != WS169_STATUS_OK)
    {
        return status;
    }

    if ((x1 > x2) || (y1 > y2) || (x2 >= width) || (y2 >= height))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    x_offset = ((rotation == WS169_ROTATION_90) ||
                (rotation == WS169_ROTATION_270)) ? WS169_CONTROLLER_RAM_OFFSET : 0U;
    y_offset = ((rotation == WS169_ROTATION_0) ||
                (rotation == WS169_ROTATION_180)) ? WS169_CONTROLLER_RAM_OFFSET : 0U;

    translated->x_start = (uint16_t)(x1 + x_offset);
    translated->x_end = (uint16_t)(x2 + x_offset);
    translated->y_start = (uint16_t)(y1 + y_offset);
    translated->y_end = (uint16_t)(y2 + y_offset);

    return WS169_STATUS_OK;
}

static WS169_Status_t ws169_set_address_window_unlocked(uint16_t x1,
                                                        uint16_t y1,
                                                        uint16_t x2,
                                                        uint16_t y2)
{
    uint8_t data[4];
    WS169_Window_t window;
    WS169_Status_t status = WS169_TranslateWindow(s_rotation, x1, y1, x2, y2, &window);

    if (status != WS169_STATUS_OK)
    {
        ws169_record_status(status, 0U);
        return status;
    }

    data[0] = (uint8_t)(window.x_start >> 8);
    data[1] = (uint8_t)(window.x_start & 0xFFU);
    data[2] = (uint8_t)(window.x_end >> 8);
    data[3] = (uint8_t)(window.x_end & 0xFFU);
    status = ws169_write_command_data(WS169_CMD_COLUMN_ADDRESS, data, sizeof(data));

    if (status == WS169_STATUS_OK)
    {
        data[0] = (uint8_t)(window.y_start >> 8);
        data[1] = (uint8_t)(window.y_start & 0xFFU);
        data[2] = (uint8_t)(window.y_end >> 8);
        data[3] = (uint8_t)(window.y_end & 0xFFU);
        status = ws169_write_command_data(WS169_CMD_ROW_ADDRESS, data, sizeof(data));
    }

    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command(WS169_CMD_MEMORY_WRITE);
    }

    return status;
}

static WS169_Status_t ws169_set_rotation_unlocked(WS169_Rotation_t rotation)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;
    WS169_Status_t status = WS169_GetGeometry(rotation, &width, &height, &madctl);

    (void)width;
    (void)height;

    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command_data(WS169_CMD_MADCTL, &madctl, 1U);
    }
    if (status == WS169_STATUS_OK)
    {
        s_rotation = rotation;
    }

    return status;
}

static WS169_Status_t ws169_reset_unlocked(void)
{
    if (!s_config_valid)
    {
        return WS169_STATUS_NOT_INITIALIZED;
    }

    ws169_cs_high();
    HAL_GPIO_WritePin(s_config.reset_port, s_config.reset_pin, GPIO_PIN_SET);
    HAL_Delay(100U);
    HAL_GPIO_WritePin(s_config.reset_port, s_config.reset_pin, GPIO_PIN_RESET);
    HAL_Delay(100U);
    HAL_GPIO_WritePin(s_config.reset_port, s_config.reset_pin, GPIO_PIN_SET);
    HAL_Delay(100U);

    return WS169_STATUS_OK;
}

static WS169_Status_t ws169_initialize_controller(WS169_Rotation_t rotation)
{
    static const uint8_t pixel_format[] = {0x55U};
    static const uint8_t porch[] = {0x0BU, 0x0BU, 0x00U, 0x33U, 0x35U};
    static const uint8_t gate_control[] = {0x11U};
    static const uint8_t vcom[] = {0x35U};
    static const uint8_t lcm_control[] = {0x2CU};
    static const uint8_t vdv_vrh_enable[] = {0x01U};
    static const uint8_t vrh_set[] = {0x0DU};
    static const uint8_t vdv_set[] = {0x20U};
    static const uint8_t frame_rate[] = {0x13U};
    static const uint8_t power[] = {0xA4U, 0xA1U};
    static const uint8_t power_control[] = {0xA1U};
    static const uint8_t gamma_positive[] = {
        0xF0U, 0x06U, 0x0BU, 0x0AU, 0x09U, 0x26U, 0x29U,
        0x33U, 0x41U, 0x18U, 0x16U, 0x15U, 0x29U, 0x2DU
    };
    static const uint8_t gamma_negative[] = {
        0xF0U, 0x04U, 0x08U, 0x08U, 0x07U, 0x03U, 0x28U,
        0x32U, 0x40U, 0x3BU, 0x19U, 0x18U, 0x2AU, 0x2EU
    };
    static const uint8_t vendor_e4[] = {0x25U, 0x00U, 0x00U};
    static const struct
    {
        uint8_t command;
        const uint8_t *data;
        uint8_t length;
    } sequence[] = {
        {0x3AU, pixel_format, (uint8_t)sizeof(pixel_format)},
        {0xB2U, porch, (uint8_t)sizeof(porch)},
        {0xB7U, gate_control, (uint8_t)sizeof(gate_control)},
        {0xBBU, vcom, (uint8_t)sizeof(vcom)},
        {0xC0U, lcm_control, (uint8_t)sizeof(lcm_control)},
        {0xC2U, vdv_vrh_enable, (uint8_t)sizeof(vdv_vrh_enable)},
        {0xC3U, vrh_set, (uint8_t)sizeof(vrh_set)},
        {0xC4U, vdv_set, (uint8_t)sizeof(vdv_set)},
        {0xC6U, frame_rate, (uint8_t)sizeof(frame_rate)},
        {0xD0U, power, (uint8_t)sizeof(power)},
        {0xD6U, power_control, (uint8_t)sizeof(power_control)},
        {0xE0U, gamma_positive, (uint8_t)sizeof(gamma_positive)},
        {0xE1U, gamma_negative, (uint8_t)sizeof(gamma_negative)},
        {0xE4U, vendor_e4, (uint8_t)sizeof(vendor_e4)}
    };
    WS169_Status_t status = ws169_reset_unlocked();

    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_rotation_unlocked(rotation);
    }

    for (uint32_t i = 0U;
         (i < (uint32_t)(sizeof(sequence) / sizeof(sequence[0]))) &&
         (status == WS169_STATUS_OK);
         i++)
    {
        status = ws169_write_command_data(sequence[i].command,
                                          sequence[i].data,
                                          sequence[i].length);
    }

    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command(WS169_CMD_INVERSION_ON);
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command(WS169_CMD_SLEEP_OUT);
        HAL_Delay(120U);
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_address_window_unlocked(0U,
                                                   0U,
                                                   (uint16_t)(WS169_GetWidth() - 1U),
                                                   (uint16_t)(WS169_GetHeight() - 1U));
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command(WS169_CMD_DISPLAY_ON);
        HAL_Delay(100U);
    }

    return status;
}

WS169_Status_t WS169_Init(const WS169_Config_t *config, WS169_Rotation_t rotation)
{
    WS169_Status_t status;
    bool lock_acquired;

    if ((config == NULL) || (config->spi == NULL) ||
        (config->cs_port == NULL) || (config->dc_port == NULL) ||
        (config->reset_port == NULL) ||
        (config->command_timeout_ms == 0U) || (config->data_timeout_ms == 0U) ||
        (rotation >= WS169_ROTATION_COUNT))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    s_config = *config;
    s_config_valid = true;
    s_diagnostics.initialized = false;
    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);

    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_spi_data_size(WS169_SPI_DATA_8BIT);
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_initialize_controller(rotation);
    }

    if (status == WS169_STATUS_OK)
    {
        s_diagnostics.initialized = true;
        ws169_record_status(WS169_STATUS_OK, 0U);
    }

    if (lock_acquired)
    {
        ws169_unlock();
    }
    return status;
}

WS169_Status_t WS169_RtosInit(void)
{
    if (s_semaphore_ready && s_mutex_ready)
    {
        return WS169_STATUS_OK;
    }

    if (tx_semaphore_create(&s_dma_semaphore, (CHAR *)"WS169 DMA", 0U) != TX_SUCCESS)
    {
        ws169_record_status(WS169_STATUS_RTOS_ERROR, 0U);
        return WS169_STATUS_RTOS_ERROR;
    }
    s_semaphore_ready = true;

    if (tx_mutex_create(&s_bus_mutex, (CHAR *)"WS169 bus", TX_INHERIT) != TX_SUCCESS)
    {
        (void)tx_semaphore_delete(&s_dma_semaphore);
        s_semaphore_ready = false;
        ws169_record_status(WS169_STATUS_RTOS_ERROR, 0U);
        return WS169_STATUS_RTOS_ERROR;
    }
    s_mutex_ready = true;
    {
        uint32_t interrupt_state = ws169_enter_critical();
        s_diagnostics.rtos_ready = true;
        ws169_exit_critical(interrupt_state);
    }

    return WS169_STATUS_OK;
}

WS169_Status_t WS169_Reset(void)
{
    WS169_Status_t status = ws169_lock();
    bool lock_acquired = (status == WS169_STATUS_OK);

    if (lock_acquired)
    {
        status = ws169_reset_unlocked();
        ws169_unlock();
    }
    return status;
}

WS169_Status_t WS169_SetRotation(WS169_Rotation_t rotation)
{
    WS169_Status_t status;
    bool lock_acquired;

    if ((!s_config_valid) || (rotation >= WS169_ROTATION_COUNT))
    {
        return s_config_valid ? WS169_STATUS_INVALID_ARGUMENT : WS169_STATUS_NOT_INITIALIZED;
    }

    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);
    if (lock_acquired)
    {
        status = ws169_set_rotation_unlocked(rotation);
        ws169_unlock();
    }
    return status;
}

static WS169_Status_t ws169_simple_command(uint8_t command, uint32_t delay_ms)
{
    WS169_Status_t status;
    bool lock_acquired;

    if (!s_diagnostics.initialized)
    {
        return WS169_STATUS_NOT_INITIALIZED;
    }

    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);
    if (lock_acquired)
    {
        status = ws169_write_command(command);
        if ((status == WS169_STATUS_OK) && (delay_ms > 0U))
        {
            HAL_Delay(delay_ms);
        }
        ws169_unlock();
    }

    return status;
}

WS169_Status_t WS169_DisplayOn(void)
{
    return ws169_simple_command(WS169_CMD_DISPLAY_ON, 0U);
}

WS169_Status_t WS169_DisplayOff(void)
{
    return ws169_simple_command(WS169_CMD_DISPLAY_OFF, 0U);
}

WS169_Status_t WS169_SleepIn(void)
{
    return ws169_simple_command(WS169_CMD_SLEEP_IN, 5U);
}

WS169_Status_t WS169_SleepOut(void)
{
    return ws169_simple_command(WS169_CMD_SLEEP_OUT, 120U);
}

WS169_Status_t WS169_SetAddressWindow(uint16_t x1,
                                      uint16_t y1,
                                      uint16_t x2,
                                      uint16_t y2)
{
    WS169_Status_t status;
    bool lock_acquired;

    if (!s_diagnostics.initialized)
    {
        return WS169_STATUS_NOT_INITIALIZED;
    }

    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);
    if (lock_acquired)
    {
        status = ws169_set_address_window_unlocked(x1, y1, x2, y2);
        ws169_unlock();
    }
    return status;
}

WS169_Status_t WS169_FillScreenRGB565(uint16_t color)
{
    static uint8_t row[WS169_MAX_ROW_BYTES];
    WS169_Status_t status;
    uint16_t width;
    uint16_t height;
    bool lock_acquired;

    if (!s_diagnostics.initialized)
    {
        return WS169_STATUS_NOT_INITIALIZED;
    }

    width = WS169_GetWidth();
    height = WS169_GetHeight();

    for (uint16_t x = 0U; x < width; x++)
    {
        row[(uint32_t)x * 2U] = (uint8_t)(color >> 8);
        row[((uint32_t)x * 2U) + 1U] = (uint8_t)(color & 0xFFU);
    }

    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);
    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_spi_data_size(WS169_SPI_DATA_8BIT);
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_address_window_unlocked(0U, 0U,
                                                   (uint16_t)(width - 1U),
                                                   (uint16_t)(height - 1U));
    }
    if (status == WS169_STATUS_OK)
    {
        ws169_dc_data();
        ws169_cs_low();
        for (uint16_t y = 0U; (y < height) && (status == WS169_STATUS_OK); y++)
        {
            status = ws169_transmit_blocking(row,
                                             (uint32_t)width * 2U,
                                             s_config.data_timeout_ms);
        }
        ws169_cs_high();
    }

    if (lock_acquired)
    {
        ws169_unlock();
    }
    return status;
}

static WS169_Status_t ws169_send_pixels(const uint16_t *pixels, uint16_t count)
{
    HAL_StatusTypeDef hal_status;

    if ((pixels == NULL) || (count == 0U))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    if ((!s_semaphore_ready) || (tx_thread_identify() == NULL))
    {
        hal_status = HAL_SPI_Transmit(s_config.spi,
                                     (uint8_t *)(uintptr_t)pixels,
                                     count,
                                     s_config.data_timeout_ms);
        if (hal_status == HAL_OK)
        {
            ws169_record_successful_transfer();
        }
        return ws169_status_from_hal(hal_status, false);
    }

    while (tx_semaphore_get(&s_dma_semaphore, TX_NO_WAIT) == TX_SUCCESS)
    {
        /* Drain a stale signal left by an aborted transfer. */
    }

    s_dma_state = WS169_DMA_PENDING;
    hal_status = HAL_SPI_Transmit_DMA(s_config.spi,
                                     (uint8_t *)(uintptr_t)pixels,
                                     count);
    if (hal_status != HAL_OK)
    {
        s_dma_state = WS169_DMA_IDLE;
        return ws169_status_from_hal(hal_status, true);
    }

    if (tx_semaphore_get(&s_dma_semaphore, ws169_timeout_ticks()) != TX_SUCCESS)
    {
        /* Ignore any abort callback: the timeout is the primary fault. */
        s_dma_state = WS169_DMA_IDLE;
        (void)HAL_SPI_Abort(s_config.spi);
        ws169_record_status(WS169_STATUS_TIMEOUT, s_config.spi->ErrorCode);
        return WS169_STATUS_TIMEOUT;
    }

    if (s_dma_state != WS169_DMA_COMPLETE)
    {
        s_dma_state = WS169_DMA_IDLE;
        return WS169_STATUS_DMA_ERROR;
    }

    s_dma_state = WS169_DMA_IDLE;
    ws169_record_successful_transfer();
    return WS169_STATUS_OK;
}

WS169_Status_t WS169_FlushRectRGB565(const uint16_t *framebuffer,
                                    uint16_t framebuffer_stride_pixels,
                                    uint16_t x,
                                    uint16_t y,
                                    uint16_t width,
                                    uint16_t height)
{
    WS169_Status_t status;
    WS169_Status_t restore_status;
    uint16_t display_width = WS169_GetWidth();
    uint16_t display_height = WS169_GetHeight();
    bool lock_acquired;
    bool chip_selected = false;

    if (!s_diagnostics.initialized)
    {
        return WS169_STATUS_NOT_INITIALIZED;
    }
    if ((framebuffer == NULL) || (width == 0U) || (height == 0U) ||
        (framebuffer_stride_pixels < display_width) ||
        (x >= display_width) || (y >= display_height) ||
        (width > (uint16_t)(display_width - x)) ||
        (height > (uint16_t)(display_height - y)))
    {
        ws169_record_status(WS169_STATUS_INVALID_ARGUMENT, 0U);
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    status = ws169_lock();
    lock_acquired = (status == WS169_STATUS_OK);
    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_address_window_unlocked(x,
                                                   y,
                                                   (uint16_t)(x + width - 1U),
                                                   (uint16_t)(y + height - 1U));
    }
    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_spi_data_size(WS169_SPI_DATA_16BIT);
    }
    if (status == WS169_STATUS_OK)
    {
        ws169_dc_data();
        ws169_cs_low();
        chip_selected = true;
    }

    if (status == WS169_STATUS_OK)
    {
        if ((x == 0U) && (width == framebuffer_stride_pixels))
        {
            const uint16_t *source = &framebuffer[(uint32_t)y * framebuffer_stride_pixels];
            uint32_t remaining = (uint32_t)width * height;

            while ((remaining > 0U) && (status == WS169_STATUS_OK))
            {
                uint16_t count = (remaining > WS169_DMA_MAX_PIXELS) ?
                                 (uint16_t)WS169_DMA_MAX_PIXELS : (uint16_t)remaining;
                status = ws169_send_pixels(source, count);
                source += count;
                remaining -= count;
            }
        }
        else
        {
            for (uint16_t row = 0U; (row < height) && (status == WS169_STATUS_OK); row++)
            {
                const uint16_t *source = &framebuffer[
                    ((uint32_t)y + row) * framebuffer_stride_pixels + x];
                status = ws169_send_pixels(source, width);
            }
        }
    }

    restore_status = WS169_STATUS_OK;
    if (lock_acquired)
    {
        if (chip_selected)
        {
            ws169_cs_high();
        }
        restore_status = ws169_set_spi_data_size(WS169_SPI_DATA_8BIT);
        ws169_unlock();
    }

    if (status == WS169_STATUS_OK)
    {
        status = restore_status;
    }
    return status;
}

uint16_t WS169_GetWidth(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;
    (void)WS169_GetGeometry(s_rotation, &width, &height, &madctl);
    return width;
}

uint16_t WS169_GetHeight(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;
    (void)WS169_GetGeometry(s_rotation, &width, &height, &madctl);
    return height;
}

WS169_Rotation_t WS169_GetRotation(void)
{
    return s_rotation;
}

WS169_Status_t WS169_GetLastStatus(void)
{
    uint32_t interrupt_state = ws169_enter_critical();
    WS169_Status_t status = s_diagnostics.last_status;
    ws169_exit_critical(interrupt_state);
    return status;
}

void WS169_GetDiagnostics(WS169_Diagnostics_t *diagnostics)
{
    uint32_t interrupt_state;

    if (diagnostics == NULL)
    {
        return;
    }

    interrupt_state = ws169_enter_critical();
    diagnostics->last_status = s_diagnostics.last_status;
    diagnostics->last_hal_error = s_diagnostics.last_hal_error;
    diagnostics->successful_transfer_count = s_diagnostics.successful_transfer_count;
    diagnostics->spi_error_count = s_diagnostics.spi_error_count;
    diagnostics->dma_error_count = s_diagnostics.dma_error_count;
    diagnostics->timeout_count = s_diagnostics.timeout_count;
    diagnostics->initialized = s_diagnostics.initialized;
    diagnostics->rtos_ready = s_diagnostics.rtos_ready;
    ws169_exit_critical(interrupt_state);
}

void WS169_ClearDiagnostics(void)
{
    uint32_t interrupt_state = ws169_enter_critical();
    bool initialized = s_diagnostics.initialized;
    bool rtos_ready = s_diagnostics.rtos_ready;

    s_diagnostics.last_status = WS169_STATUS_OK;
    s_diagnostics.last_hal_error = 0U;
    s_diagnostics.successful_transfer_count = 0U;
    s_diagnostics.spi_error_count = 0U;
    s_diagnostics.dma_error_count = 0U;
    s_diagnostics.timeout_count = 0U;
    s_diagnostics.initialized = initialized;
    s_diagnostics.rtos_ready = rtos_ready;
    ws169_exit_critical(interrupt_state);
}

bool WS169_OnSpiTxComplete(SPI_HandleTypeDef *spi)
{
    if ((!s_config_valid) || (spi != s_config.spi) || (s_dma_state != WS169_DMA_PENDING))
    {
        return false;
    }

    s_dma_state = WS169_DMA_COMPLETE;
    if (s_semaphore_ready)
    {
        (void)tx_semaphore_put(&s_dma_semaphore);
    }

    return true;
}

bool WS169_OnSpiError(SPI_HandleTypeDef *spi)
{
    if ((!s_config_valid) || (spi != s_config.spi) || (s_dma_state != WS169_DMA_PENDING))
    {
        return false;
    }

    s_dma_state = WS169_DMA_FAILED;
    ws169_record_status(WS169_STATUS_DMA_ERROR, spi->ErrorCode);
    if (s_semaphore_ready)
    {
        (void)tx_semaphore_put(&s_dma_semaphore);
    }

    return true;
}
