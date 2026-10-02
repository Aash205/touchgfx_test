#ifndef HEALTH_FORMAT_H
#define HEALTH_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Longest possible line, every number at UINT32_MAX: a buffer of this size plus 1 never cuts it. */
#define HEALTH_FORMAT_MAX_LENGTH 133U

#ifdef __cplusplus
extern "C"
{
#endif

/* clang-format off */
/*
 * The periodic health log line of the monitor thread:
 *   HEALTH ThreadX=OK USBX=<ACTIVE|WAIT> TouchGFX_FPS=<n> BLE=<n> Display=<n> DisplayFaults=<n> Heartbeat=<n>
 * Numbers are in decimal without padding. The text is NUL-terminated and cut at capacity - 1
 * characters; the return value is the number of characters stored (0 when out is NULL or
 * capacity is 0).
 */
size_t HealthFormat_Line(char* out, size_t capacity, bool usbx_active, uint32_t fps,
                         uint32_t ble_status, uint32_t display_status, uint32_t display_faults,
                         uint32_t heartbeat);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_FORMAT_H */
