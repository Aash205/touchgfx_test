#ifndef RING_H
#define RING_H

/*
 * Byte ring buffer with free-running head and tail counters.
 *
 * - Single consumer; producers
 * must be serialised by the caller (for example by disabling
 *   interrupts around Ring_Push).
 *
 * - The capacity must be a non-zero power of two. head and tail are never wrapped to the
 *
 * capacity: they count up and overflow naturally, and (head - tail) is the stored byte count.
 * -
 * Pure logic: no RTOS or HAL types. The caller owns the storage.
 */

typedef struct
{
    unsigned char* buffer;  /* caller-owned storage */
    unsigned capacity;      /* size of buffer in bytes; non-zero power of two */
    volatile unsigned head; /* written by producers */
    volatile unsigned tail; /* written by the consumer */
} Ring_t;

/* Static initialiser for a ring over a statically sized array: Ring_t r = RING_INITIALIZER(array);
 */
#define RING_INITIALIZER(storage) {(storage), (unsigned)sizeof(storage), 0U, 0U}

/* Append up to size bytes; returns how many were accepted (the rest are dropped). */
unsigned Ring_Push(Ring_t* ring, const unsigned char* data, unsigned size);

/* Remove up to max bytes in FIFO order into out; returns how many were removed. */
unsigned Ring_Pop(Ring_t* ring, unsigned char* out, unsigned max);

/* Number of bytes currently stored. */
unsigned Ring_Count(const Ring_t* ring);

#endif /* RING_H */
