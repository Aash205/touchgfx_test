#ifndef WS169_WIRE_H
#define WS169_WIRE_H

#include "ws169_geometry.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Bytes of one column or row address parameter block. */
#define WS169_WINDOW_BYTES 4U

/* clang-format off */
/*
 * The bytes the ST7789 controller receives, built without the SPI.
 *
 * WS169_EncodeWindow: the parameters of the column address command (columns) and of the row
 * address command (rows), each WS169_WINDOW_BYTES long: start then end, high byte first.
 * A NULL window writes nothing; a NULL columns or rows pointer is skipped.
 *
 * WS169_FillRowRGB565: one display row of a single RGB565 colour, two bytes per pixel, high byte
 * first. Returns the number of bytes written (2 * width_px), or 0 and writes nothing when row is
 * NULL or capacity is smaller than 2 * width_px. A width of 0 writes nothing and returns 0.
 */
void WS169_EncodeWindow(const WS169_Window_t* window, uint8_t* columns, uint8_t* rows);
size_t WS169_FillRowRGB565(uint8_t* row, size_t capacity, uint16_t width_px, uint16_t color);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* WS169_WIRE_H */
