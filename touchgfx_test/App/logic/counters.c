#include "counters.h"

#include <stddef.h>

void Counters_FrameFlushed(Counters_t* counters)
{
    if (counters != NULL)
    {
        counters->frames = counters->frames + 1U;
    }
}

void Counters_Tick1s(Counters_t* counters)
{
    if (counters != NULL)
    {
        counters->fps = (uint16_t)counters->frames;
        counters->frames = 0U;
    }
}

uint32_t Counters_Heartbeat(Counters_t* counters)
{
    uint32_t next = 0U;

    if (counters != NULL)
    {
        next = counters->heartbeat + 1U;
        counters->heartbeat = next;
    }

    return next;
}

uint16_t Counters_Fps(const Counters_t* counters)
{
    return (counters != NULL) ? counters->fps : 0U;
}

uint32_t Counters_HeartbeatValue(const Counters_t* counters)
{
    return (counters != NULL) ? counters->heartbeat : 0U;
}
