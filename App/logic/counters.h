#ifndef COUNTERS_H
#define COUNTERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    volatile uint32_t frames;    /* panel flushes since the last Counters_Tick1s */
    volatile uint16_t fps;       /* flushes counted by the last Counters_Tick1s */
    volatile uint32_t heartbeat; /* monitor heartbeat counter */
} Counters_t;

/* clang-format off */
/*
 * The display frame counter and the heartbeat counter shown on the status screen. Zero in static
 * storage, which is how the firmware uses them. Pure: the caller decides which thread calls what.
 * The fields are volatile because the flush callback, the once-a-second tick and the readers run
 * in different threads; the functions do no locking, as the original code did not.
 *
 * Counters_FrameFlushed counts one flush. Counters_Tick1s stores the flush count since the last
 * tick as the frame rate (kept in 16 bits, so more than 65535 flushes in one second wrap) and
 * restarts the count. Counters_Heartbeat adds one and returns the new value (wrapping to 0 after
 * UINT32_MAX). A NULL counters pointer is ignored; the getters and Counters_Heartbeat give 0.
 */
void Counters_FrameFlushed(Counters_t* counters);
void Counters_Tick1s(Counters_t* counters);
uint32_t Counters_Heartbeat(Counters_t* counters);

uint16_t Counters_Fps(const Counters_t* counters);
uint32_t Counters_HeartbeatValue(const Counters_t* counters);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* COUNTERS_H */
