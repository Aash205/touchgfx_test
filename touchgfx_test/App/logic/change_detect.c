#include "change_detect.h"

#include <stddef.h>

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

bool ChangeDetect_Changed(ChangeDetect_t* detect, bool differs)
{
    bool changed = false;

    if (detect != NULL)
    {
        changed = (!detect->have_last) || differs;
        detect->have_last = true;
    }

    return changed;
}
