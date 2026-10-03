#ifndef CHANGE_DETECT_H
#define CHANGE_DETECT_H

#include <stdbool.h>
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
 * reads the application state, decides whether it differs from the last one it reported, and
 * notifies the listener. A zeroed ChangeDetect_t is the initial state (no ticks counted, nothing
 * reported yet).
 *
 * ChangeDetect_PollDue counts one tick and returns true on every period-th tick, restarting
 * the count then. A period of 0 or 1 is due on every tick.
 *
 * ChangeDetect_Changed returns true when a notification is due: the first time it is called,
 * whatever `differs` says, and afterwards whenever `differs` is true. The caller then stores the
 * state it reported. A NULL detector returns false.
 *
 * The caller compares the state field by field (not byte by byte), so padding bytes inside a
 * struct, whose value C leaves unspecified, can never look like a change.
 */
bool ChangeDetect_PollDue(ChangeDetect_t* detect, uint8_t period);
bool ChangeDetect_Changed(ChangeDetect_t* detect, bool differs);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* CHANGE_DETECT_H */
