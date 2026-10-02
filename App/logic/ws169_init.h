#ifndef WS169_INIT_H
#define WS169_INIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t command;
    const uint8_t* data; /* parameter bytes, length of them */
    uint8_t length;
} WS169_InitCommand_t;

/* clang-format off */
/*
 * The ST7789V2 power, voltage and gamma set-up commands of the Waveshare 1.69 inch panel, in the
 * order they are sent after the reset and the MADCTL (rotation) command and before the
 * inversion, sleep-out, address window and display-on commands, which stay in the driver
 * because they carry delays.
 *
 * WS169_InitCommandCount: how many commands there are.
 * WS169_InitCommandAt: stores command `index` (0 based) and returns true, or returns false when
 * index is out of range or command is NULL.
 */
size_t WS169_InitCommandCount(void);
bool WS169_InitCommandAt(size_t index, WS169_InitCommand_t* command);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* WS169_INIT_H */
