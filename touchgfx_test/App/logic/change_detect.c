#include "change_detect.h"

#include <string.h>

bool ChangeDetect_PollDue(ChangeDetect_t* detect, uint8_t period)
{
    bool due = false;

    if (detect != NULL)
    {
        if (detect->ticks < UINT8_MAX)
        {
            detect->ticks++;
        }
        if (detect->ticks >= period)
        {
            detect->ticks = 0U;
            due = true;
        }
    }

    return due;
}

bool ChangeDetect_Update(ChangeDetect_t* detect, uint8_t* last, const uint8_t* now, size_t size)
{
    bool changed = false;

    if ((detect != NULL) && (last != NULL) && (now != NULL))
    {
        if ((!detect->have_last) || (memcmp(now, last, size) != 0))
        {
            (void)memcpy(last, now, size);
            detect->have_last = true;
            changed = true;
        }
    }

    return changed;
}
