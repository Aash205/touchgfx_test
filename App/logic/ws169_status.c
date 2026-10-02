#include "ws169_status.h"

WS169_Status_t WS169_StatusFromHal(uint32_t hal_status, bool dma_transfer)
{
    WS169_Status_t status;

    switch (hal_status)
    {
    case WS169_HAL_OK:
        status = WS169_STATUS_OK;
        break;
    case WS169_HAL_BUSY:
        status = WS169_STATUS_SPI_BUSY;
        break;
    case WS169_HAL_TIMEOUT:
        status = WS169_STATUS_TIMEOUT;
        break;
    case WS169_HAL_ERROR:
    default:
        status = dma_transfer ? WS169_STATUS_DMA_ERROR : WS169_STATUS_SPI_ERROR;
        break;
    }

    return status;
}

WS169_Counter_t WS169_CounterOf(WS169_Status_t status)
{
    WS169_Counter_t counter = WS169_COUNTER_NONE;

    if ((status == WS169_STATUS_SPI_ERROR) || (status == WS169_STATUS_SPI_BUSY))
    {
        counter = WS169_COUNTER_SPI;
    }
    else if (status == WS169_STATUS_DMA_ERROR)
    {
        counter = WS169_COUNTER_DMA;
    }
    else if (status == WS169_STATUS_TIMEOUT)
    {
        counter = WS169_COUNTER_TIMEOUT;
    }
    else
    {
        /* No diagnostic counter applies. */
    }

    return counter;
}
