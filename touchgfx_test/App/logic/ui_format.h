#ifndef UI_FORMAT_H
#define UI_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UI_FORMAT_LED_COUNT 2U /* LEDs with a label; the GUI checks it against APP_LED_COUNT */

#ifdef __cplusplus
extern "C"
{
#endif

/* clang-format off */
/*
 * The texts of the live status screen. Pure functions: the widgets that show them stay in the
 * TouchGFX view. Each line formatter stores a NUL-terminated string truncated to capacity - 1
 * characters and returns the number of characters stored (0 when out is NULL or capacity is 0).
 *
 * UiFormat_BleLine: "BLE: <name>" with the name Idle, Init, Ready, Advertising, Connected, Paired
 * or Error for BLE status 0 to 6; every larger value is shown as Error.
 * UiFormat_UptimeLine: "Uptime: HH:MM:SS", hours with at least two digits and no upper limit
 * (so more than 99 hours gives three or more digits).
 * UiFormat_HeartbeatLine: "Heartbeat: <n>". UiFormat_FpsLine: "FPS: <n>".
 *
 * UiFormat_LedLabel: the button label of LED `index` (0 = LD1, 1 = LD3): "LD1 ON", "LD1 OFF",
 * "LD3 ON" or "LD3 OFF". An index of UI_FORMAT_LED_COUNT or more gives an empty string, never NULL.
 */
size_t UiFormat_BleLine(char* out, size_t capacity, uint8_t ble_status);
size_t UiFormat_UptimeLine(char* out, size_t capacity, uint32_t uptime_s);
size_t UiFormat_HeartbeatLine(char* out, size_t capacity, uint32_t heartbeat);
size_t UiFormat_FpsLine(char* out, size_t capacity, uint16_t fps);
const char* UiFormat_LedLabel(uint8_t index, bool on);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* UI_FORMAT_H */
