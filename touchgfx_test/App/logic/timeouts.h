#ifndef TIMEOUTS_H
#define TIMEOUTS_H

#include <stdbool.h>
#include <stdint.h>

/* clang-format off */
/*
 * Convert a duration in milliseconds to RTOS ticks, rounding up so a non-zero duration never
 * becomes a zero-tick (no-wait) timeout. A zero duration gives zero ticks. The result saturates
 * at UINT32_MAX instead of wrapping. tick_hz is the RTOS tick rate (ticks per second).
 */
/* clang-format on */
uint32_t Timeout_MsToTicks(uint32_t ms, uint32_t tick_hz);

/*
 * True when at least limit has passed since start, for free-running unsigned counters (for
 * example HAL_GetTick milliseconds). Correct across the counter wrapping, as long as the real
 * elapsed time is below 2^32.
 */
bool Timeout_Elapsed(uint32_t start, uint32_t now, uint32_t limit);

#endif /* TIMEOUTS_H */
