/**
  ******************************************************************************
  * @file    waveshare_driver.c
  * @brief   Waveshare 1.69-inch ST7789V2 LCD driver.
  ******************************************************************************
  */

#include "waveshare_driver.h"
#include "main.h"
#include "tx_api.h"
#include "timeouts.h"
#include "ws169_flush.h"
#include "ws169_init.h"
#include "ws169_status.h"
#include "ws169_wire.h"
#include <stddef.h>

extern SPI_HandleTypeDef hspi2;

#define WS169_CMD_SLEEP_IN       0x10U
#define WS169_CMD_SLEEP_OUT      0x11U
#define WS169_CMD_INVERSION_ON   0x21U
#define WS169_CMD_DISPLAY_OFF    0x28U
#define WS169_CMD_DISPLAY_ON     0x29U
#define WS169_CMD_COLUMN_ADDRESS 0x2AU
#define WS169_CMD_ROW_ADDRESS    0x2BU
#define WS169_CMD_MEMORY_WRITE   0x2CU
#define WS169_CMD_MADCTL         0x36U

#define WS169_SPI_DATA_8BIT      SPI_DATASIZE_8BIT
#define WS169_SPI_DATA_16BIT     SPI_DATASIZE_16BIT
#define WS169_MAX_ROW_BYTES      (WS169_LANDSCAPE_WIDTH * 2U)

_Static_assert(((uint32_t)HAL_OK == WS169_HAL_OK) && ((uint32_t)HAL_ERROR == WS169_HAL_ERROR) &&
                 ((uint32_t)HAL_BUSY == WS169_HAL_BUSY) && ((uint32_t)HAL_TIMEOUT == WS169_HAL_TIMEOUT),
               "ws169_status HAL codes must match HAL_StatusTypeDef");

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

WS169_Status_t WS169_InitBoard(WS169_Rotation_t rotation)
{
    const WS169_Config_t config = {
        .spi = &hspi2,
        /* cppcheck-suppress misra-c2012-11.4 -- HAL port macro (a fixed GPIO peripheral address) */
        .cs_port = DISP_CS_GPIO_Port,
        .cs_pin = DISP_CS_Pin,
        /* cppcheck-suppress misra-c2012-11.4 -- HAL port macro (a fixed GPIO peripheral address) */
        .dc_port = DISP_DC_GPIO_Port,
        .dc_pin = DISP_DC_Pin,
        /* cppcheck-suppress misra-c2012-11.4 -- HAL port macro (a fixed GPIO peripheral address) */
        .reset_port = DISP_RES_GPIO_Port,
        .reset_pin = DISP_RES_Pin,
        .command_timeout_ms = 100U,
        .data_timeout_ms = 1000U
    };

    return WS169_Init(&config, rotation);
}

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

    switch (WS169_CounterOf(status))
    {
        case WS169_COUNTER_SPI:
            s_diagnostics.spi_error_count++;
            break;

        case WS169_COUNTER_DMA:
            s_diagnostics.dma_error_count++;
            break;

        case WS169_COUNTER_TIMEOUT:
            s_diagnostics.timeout_count++;
            break;

        case WS169_COUNTER_NONE:
        default:
            /* No diagnostic counter applies. */
            break;
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
    const WS169_Status_t status = WS169_StatusFromHal((uint32_t)hal_status, dma_transfer);
    uint32_t hal_error = 0U;

    if (s_config_valid && (s_config.spi != NULL))
    {
        hal_error = s_config.spi->ErrorCode;
    }

    if (status != WS169_STATUS_OK)
    {
        ws169_record_status(status, hal_error);
    }

    return status;
}

static ULONG ws169_timeout_ticks(void)
{
    uint32_t ticks = Timeout_MsToTicks(s_config.data_timeout_ms, TX_TIMER_TICKS_PER_SECOND);

    if (ticks == 0U)
    {
        ticks = 1U;
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
    const uint8_t *cursor = data;
    uint32_t remaining = size;

    if ((cursor == NULL) || (remaining == 0U))
    {
        return WS169_STATUS_INVALID_ARGUMENT;
    }

    while (remaining > 0U)
    {
        uint16_t chunk = (uint16_t)WS169_ChunkSize(remaining, 0xFFFFU);
        HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(s_config.spi,
                                                       /* cppcheck-suppress misra-c2012-11.4 -- the HAL transmit calls take a non-const buffer and only read it */
                                                       (uint8_t *)(uintptr_t)cursor,
                                                       chunk,
                                                       timeout_ms);
        WS169_Status_t status = ws169_status_from_hal(hal_status, false);

        if (status != WS169_STATUS_OK)
        {
            return status;
        }

        /* cppcheck-suppress [misra-c2012-18.4, misra-c2012-10.3] -- walks the caller's buffer one chunk at a time */
        cursor += chunk;
        remaining -= chunk;
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

static WS169_Status_t ws169_set_address_window_unlocked(uint16_t x1,
                                                        uint16_t y1,
                                                        uint16_t x2,
                                                        uint16_t y2)
{
    uint8_t columns[WS169_WINDOW_BYTES];
    uint8_t rows[WS169_WINDOW_BYTES];
    WS169_Window_t window;
    WS169_Status_t status = WS169_TranslateWindow(s_rotation, x1, y1, x2, y2, &window);

    if (status != WS169_STATUS_OK)
    {
        ws169_record_status(status, 0U);
        return status;
    }

    WS169_EncodeWindow(&window, columns, rows);
    status = ws169_write_command_data(WS169_CMD_COLUMN_ADDRESS, columns, sizeof(columns));

    if (status == WS169_STATUS_OK)
    {
        status = ws169_write_command_data(WS169_CMD_ROW_ADDRESS, rows, sizeof(rows));
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
    WS169_Status_t status = ws169_reset_unlocked();

    if (status == WS169_STATUS_OK)
    {
        status = ws169_set_rotation_unlocked(rotation);
    }

    for (size_t i = 0U; (i < WS169_InitCommandCount()) && (status == WS169_STATUS_OK); i++)
    {
        WS169_InitCommand_t command;

        if (WS169_InitCommandAt(i, &command))
        {
            status = ws169_write_command_data(command.command, command.data, command.length);
        }
        else
        {
            status = WS169_STATUS_INVALID_ARGUMENT;
        }
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
    static CHAR semaphore_name[] = {'W', 'S', '1', '6', '9', ' ', 'D', 'M', 'A', '\0'};
    static CHAR mutex_name[] = {'W', 'S', '1', '6', '9', ' ', 'b', 'u', 's', '\0'};

    if (s_semaphore_ready && s_mutex_ready)
    {
        return WS169_STATUS_OK;
    }

    if (tx_semaphore_create(&s_dma_semaphore, semaphore_name, 0U) != TX_SUCCESS)
    {
        ws169_record_status(WS169_STATUS_RTOS_ERROR, 0U);
        return WS169_STATUS_RTOS_ERROR;
    }
    s_semaphore_ready = true;

    if (tx_mutex_create(&s_bus_mutex, mutex_name, TX_INHERIT) != TX_SUCCESS)
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

    if (WS169_FillRowRGB565(row, sizeof(row), width, color) == 0U)
    {
        ws169_record_status(WS169_STATUS_INVALID_ARGUMENT, 0U);
        return WS169_STATUS_INVALID_ARGUMENT;
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
                                     /* cppcheck-suppress misra-c2012-11.4 -- the HAL transmit calls take a non-const buffer and only read it */
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
                                     /* cppcheck-suppress misra-c2012-11.4 -- the HAL transmit calls take a non-const buffer and only read it */
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
    if ((framebuffer == NULL) ||
        (!WS169_FlushRectValid(framebuffer_stride_pixels, display_width, display_height,
                               x, y, width, height)))
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
        WS169_FlushPlan_t plan;
        uint32_t offset = 0U;
        uint16_t count = 0U;

        ws169_dc_data();
        ws169_cs_low();
        chip_selected = true;

        WS169_FlushPlanInit(&plan, framebuffer_stride_pixels, x, y, width, height);
        while ((status == WS169_STATUS_OK) && WS169_FlushPlanNext(&plan, &offset, &count))
        {
            status = ws169_send_pixels(&framebuffer[offset], count);
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

static bool ws169_on_spi_tx_complete(const SPI_HandleTypeDef *spi)
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

static bool ws169_on_spi_error(const SPI_HandleTypeDef *spi)
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

/* cppcheck-suppress constParameterPointer -- must match the HAL's weak callback prototype */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    (void)ws169_on_spi_tx_complete(hspi);
}

/* cppcheck-suppress constParameterPointer -- must match the HAL's weak callback prototype */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    (void)ws169_on_spi_error(hspi);
}
