#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t stable; /* consecutive polls that agreed with last, counting up to the limit */
    uint8_t last;   /* last sampled level, 1 = pressed */
} Debounce_t;

/* clang-format off */
/*
 * Button debounce for an active-high button polled at a fixed rate. Pure: the caller reads the
 * pin and passes the level. A zeroed Debounce_t (static storage) is a released button.
 *
 * Debounce_Poll takes one sample (pressed = true) and returns true exactly once per press: on the
 * poll where the new level has been seen, after the change itself, for `polls` further polls in
 * a row. So with polls = 2 a press is reported on the third poll that reads pressed. A held
 * button reports nothing more; releasing and pressing again reports again. A NULL state, or
 * polls = 0, never reports.
 */
bool Debounce_Poll(Debounce_t* debounce, bool pressed, uint8_t polls);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* DEBOUNCE_H */
