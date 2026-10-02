#include "debounce.h"

#include <stddef.h>

bool Debounce_Poll(Debounce_t* debounce, bool pressed, uint8_t polls)
{
    bool reported = false;

    if (debounce != NULL)
    {
        const uint8_t level = pressed ? 1U : 0U;

        if (level == debounce->last)
        {
            if (debounce->stable < polls)
            {
                debounce->stable++;
                reported = (debounce->stable == polls) && pressed;
            }
        }
        else
        {
            debounce->last = level;
            debounce->stable = 0U;
        }
    }

    return reported;
}
