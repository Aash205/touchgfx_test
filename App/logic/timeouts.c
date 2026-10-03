#include "timeouts.h"

uint32_t Timeout_MsToTicks(uint32_t ms, uint32_t tick_hz)
{
    const uint64_t ticks = (((uint64_t)ms * (uint64_t)tick_hz) + 999ULL) / 1000ULL;
    uint32_t result = UINT32_MAX;

    if (ticks <= (uint64_t)UINT32_MAX)
    {
        result = (uint32_t)ticks;
    }

    return result;
}

bool Timeout_Elapsed(uint32_t start, uint32_t now, uint32_t limit)
{
    return (uint32_t)(now - start) >= limit;
}
