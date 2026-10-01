#include "ws169_flush.h"

#include <stddef.h>

bool WS169_FlushRectValid(uint16_t stride, uint16_t display_width, uint16_t display_height,
                          uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    return (width != 0U) && (height != 0U) && (stride >= display_width) && (x < display_width) &&
           (y < display_height) && (width <= (uint16_t)(display_width - x)) &&
           (height <= (uint16_t)(display_height - y));
}

uint32_t WS169_ChunkSize(uint32_t remaining, uint32_t max)
{
    return (remaining > max) ? max : remaining;
}

void WS169_FlushPlanInit(WS169_FlushPlan_t* plan, uint16_t stride, uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height)
{
    if (plan != NULL)
    {
        plan->stride = stride;
        plan->width = width;
        plan->contiguous = (x == 0U) && (width == stride);
        if (plan->contiguous)
        {
            plan->next = (uint32_t)y * stride;
            plan->remaining = (uint32_t)width * height;
        }
        else
        {
            plan->next = ((uint32_t)y * stride) + x;
            plan->remaining = 0U;
            if (width != 0U)
            {
                plan->remaining = height;
            }
        }
    }
}

bool WS169_FlushPlanNext(WS169_FlushPlan_t* plan, uint32_t* offset, uint16_t* count)
{
    bool more = false;

    if ((plan != NULL) && (offset != NULL) && (count != NULL) && (plan->remaining != 0U))
    {
        *offset = plan->next;
        if (plan->contiguous)
        {
            const uint32_t chunk = WS169_ChunkSize(plan->remaining, WS169_DMA_MAX_PIXELS);

            *count = (uint16_t)chunk;
            plan->next += chunk;
            plan->remaining -= chunk;
        }
        else
        {
            *count = plan->width;
            plan->next += plan->stride;
            plan->remaining--;
        }
        more = true;
    }

    return more;
}
