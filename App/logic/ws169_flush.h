#ifndef WS169_FLUSH_H
#define WS169_FLUSH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* One DMA transfer moves at most this many 16-bit pixels. */
#define WS169_DMA_MAX_PIXELS 65535U

typedef struct
{
    uint32_t next;      /* pixel offset of the next transfer */
    uint32_t remaining; /* pixels (contiguous plan) or rows (row plan) still to send */
    uint16_t stride;    /* framebuffer pixels per row */
    uint16_t width;     /* pixels per row transfer in a row plan */
    bool contiguous;    /* true: whole-width rectangle sent as one block, in chunks */
} WS169_FlushPlan_t;

/* clang-format off */
/*
 * Flushing a rectangle of the framebuffer to the panel: the checks and the transfer plan.
 *
 * WS169_FlushRectValid: true when the rectangle (x, y, width, height) is non-empty and lies
 * inside the display (display_width x display_height) and the framebuffer rows are at least as
 * wide as the display (stride >= display_width). Overflow-safe for every uint16_t value.
 *
 * WS169_ChunkSize: min(remaining, max), the size of the next transfer chunk.
 *
 * WS169_FlushPlanInit / WS169_FlushPlanNext: the transfers that send the rectangle, in order.
 * When the rectangle starts at x = 0 and is as wide as the framebuffer rows (width == stride) it
 * is contiguous in memory and is sent as one block from offset y * stride in chunks of at most
 * WS169_DMA_MAX_PIXELS. Otherwise it is sent one row per transfer: `width` pixels from offset
 * (y + row) * stride + x. WS169_FlushPlanNext stores the pixel offset (from the start of the
 * framebuffer) and the pixel count of the next transfer and returns true, or returns false when
 * the plan is finished (or an argument is NULL). An empty rectangle gives no transfers.
 * The plan assumes the rectangle passed WS169_FlushRectValid for the display: for a rectangle
 * that reaches outside it the offsets are not meaningful (and can wrap).
 */
bool WS169_FlushRectValid(uint16_t stride, uint16_t display_width, uint16_t display_height,
                          uint16_t x, uint16_t y, uint16_t width, uint16_t height);
uint32_t WS169_ChunkSize(uint32_t remaining, uint32_t max);
void WS169_FlushPlanInit(WS169_FlushPlan_t* plan, uint16_t stride, uint16_t x, uint16_t y,
                         uint16_t width, uint16_t height);
bool WS169_FlushPlanNext(WS169_FlushPlan_t* plan, uint32_t* offset, uint16_t* count);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* WS169_FLUSH_H */
