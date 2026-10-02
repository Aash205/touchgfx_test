#include "ring.h"

#include <stdbool.h>
#include <stddef.h>

static bool ring_is_valid(const Ring_t* ring)
{
    return (ring != NULL) && (ring->buffer != NULL) && (ring->capacity != 0U) &&
           ((ring->capacity & (ring->capacity - 1U)) == 0U);
}

unsigned Ring_Push(Ring_t* ring, const unsigned char* data, unsigned size)
{
    unsigned n = 0U;

    if (ring_is_valid(ring) && (data != NULL))
    {
        while ((n < size) && ((ring->head - ring->tail) < ring->capacity))
        {
            ring->buffer[ring->head & (ring->capacity - 1U)] = data[n];
            n++;
            ring->head++;
        }
    }

    return n;
}

unsigned Ring_Pop(Ring_t* ring, unsigned char* out, unsigned max)
{
    unsigned n = 0U;

    if (ring_is_valid(ring) && (out != NULL))
    {
        while ((ring->tail != ring->head) && (n < max))
        {
            out[n] = ring->buffer[ring->tail & (ring->capacity - 1U)];
            n++;
            ring->tail++;
        }
    }

    return n;
}

unsigned Ring_Count(const Ring_t* ring)
{
    unsigned count = 0U;

    if (ring_is_valid(ring))
    {
        count = ring->head - ring->tail;
    }

    return count;
}
