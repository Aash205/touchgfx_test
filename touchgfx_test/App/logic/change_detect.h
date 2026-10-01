#ifndef CHANGE_DETECT_H
#define CHANGE_DETECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t ticks;  /* ticks counted since the last poll */
    bool have_last; /* a snapshot has been stored */
} ChangeDetect_t;

/* clang-format off */
/*
 * The model's poll divider and notify-on-change logic, without the state type. Pure: the caller
 * reads the application state and notifies the listener. A zeroed ChangeDetect_t is the initial
 * state (no ticks counted, no snapshot yet).
 *
 * ChangeDetect_PollDue counts one tick and returns true on every period-th tick, restarting
 * the count then. A period of 0 or 1 is due on every tick.
 *
 * ChangeDetect_Update compares the snapshot `now` with the stored copy `last` (both `size` bytes
 * long) and returns true when a notification is due: the first time, and whenever the bytes
 * differ. The snapshots are passed as bytes (a C++ caller casts its struct). In that case it also
 * stores `now` into `last`. Equal snapshots return false and change
 * nothing. NULL pointers return false and change nothing.
 *
 * The comparison is byte by byte, exactly like the memcmp it replaces: padding bytes inside a
 * struct take part. A caller that wants padding to be ignored should zero the snapshot before
 * filling it in.
 */
bool ChangeDetect_PollDue(ChangeDetect_t* detect, uint8_t period);
bool ChangeDetect_Update(ChangeDetect_t* detect, uint8_t* last, const uint8_t* now, size_t size);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* CHANGE_DETECT_H */
