#include "ws169_wire.h"

static void put_pair(uint8_t* out, uint16_t first, uint16_t second)
{
    out[0] = (uint8_t)(first >> 8U);
    out[1] = (uint8_t)(first & 0xFFU);
    out[2] = (uint8_t)(second >> 8U);
    out[3] = (uint8_t)(second & 0xFFU);
}

void WS169_EncodeWindow(const WS169_Window_t* window, uint8_t* columns, uint8_t* rows)
{
    if (window != NULL)
    {
        if (columns != NULL)
        {
            put_pair(columns, window->x_start, window->x_end);
        }
        if (rows != NULL)
        {
            put_pair(rows, window->y_start, window->y_end);
        }
    }
}

size_t WS169_FillRowRGB565(uint8_t* row, size_t capacity, uint16_t width_px, uint16_t color)
{
    const size_t needed = (size_t)width_px * 2U;
    size_t written = 0U;

    if ((row != NULL) && (capacity >= needed))
    {
        for (size_t x = 0U; x < width_px; x++)
        {
            row[x * 2U] = (uint8_t)(color >> 8U);
            row[(x * 2U) + 1U] = (uint8_t)(color & 0xFFU);
        }
        written = needed;
    }

    return written;
}
