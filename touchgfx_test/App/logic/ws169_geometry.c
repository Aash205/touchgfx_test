#include "ws169_geometry.h"
#include <stddef.h>

WS169_Status_t WS169_GetGeometry(WS169_Rotation_t rotation,
                                 uint16_t *width,
                                 uint16_t *height,
                                 uint8_t *madctl)
{
    WS169_Status_t status = WS169_STATUS_OK;

    if ((rotation >= WS169_ROTATION_COUNT) ||
        (width == NULL) || (height == NULL) || (madctl == NULL))
    {
        status = WS169_STATUS_INVALID_ARGUMENT;
    }
    else
    {
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
                status = WS169_STATUS_INVALID_ARGUMENT;
                break;
        }
    }

    return status;
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
    WS169_Status_t status;

    status = WS169_STATUS_INVALID_ARGUMENT;
    if (translated != NULL)
    {
        status = WS169_GetGeometry(rotation, &width, &height, &madctl);
        (void)madctl;
        if ((status == WS169_STATUS_OK) &&
            (x1 <= x2) && (y1 <= y2) && (x2 < width) && (y2 < height))
        {
            uint16_t x_offset;
            uint16_t y_offset;

            x_offset = ((rotation == WS169_ROTATION_90) ||
                        (rotation == WS169_ROTATION_270)) ? WS169_CONTROLLER_RAM_OFFSET : 0U;
            y_offset = ((rotation == WS169_ROTATION_0) ||
                        (rotation == WS169_ROTATION_180)) ? WS169_CONTROLLER_RAM_OFFSET : 0U;

            translated->x_start = (uint16_t)(x1 + x_offset);
            translated->x_end = (uint16_t)(x2 + x_offset);
            translated->y_start = (uint16_t)(y1 + y_offset);
            translated->y_end = (uint16_t)(y2 + y_offset);
        }
        else if (status == WS169_STATUS_OK)
        {
            status = WS169_STATUS_INVALID_ARGUMENT;
        }
        else
        {
            /* Preserve the geometry error status. */
        }
    }

    return status;
}
