#include "unity.h"
#include "ws169_flush.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- the rectangle checks ------------------------------------------------------------------ */

void test_a_rectangle_inside_the_display_is_valid(void)
{
    TEST_ASSERT_TRUE(WS169_FlushRectValid(280U, 280U, 240U, 0U, 0U, 280U, 240U));
    TEST_ASSERT_TRUE(WS169_FlushRectValid(280U, 280U, 240U, 10U, 20U, 30U, 40U));
    TEST_ASSERT_TRUE(WS169_FlushRectValid(280U, 280U, 240U, 279U, 239U, 1U, 1U));
}

void test_an_empty_rectangle_is_invalid(void)
{
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 0U, 0U, 0U, 10U));
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 0U, 0U, 10U, 0U));
}

void test_a_rectangle_that_leaves_the_display_is_invalid(void)
{
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 280U, 0U, 1U, 1U));
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 0U, 240U, 1U, 1U));
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 270U, 0U, 11U, 1U));
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 0U, 230U, 1U, 11U));
    TEST_ASSERT_FALSE(WS169_FlushRectValid(280U, 280U, 240U, 65535U, 65535U, 65535U, 65535U));
}

void test_framebuffer_rows_narrower_than_the_display_are_invalid(void)
{
    TEST_ASSERT_FALSE(WS169_FlushRectValid(279U, 280U, 240U, 0U, 0U, 10U, 10U));
    TEST_ASSERT_TRUE(WS169_FlushRectValid(300U, 280U, 240U, 0U, 0U, 10U, 10U));
}

/* ---- chunk size ---------------------------------------------------------------------------- */

void test_the_chunk_is_the_smaller_of_what_is_left_and_the_maximum(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, WS169_ChunkSize(0U, 65535U));
    TEST_ASSERT_EQUAL_UINT32(1000U, WS169_ChunkSize(1000U, 65535U));
    TEST_ASSERT_EQUAL_UINT32(65535U, WS169_ChunkSize(65535U, 65535U));
    TEST_ASSERT_EQUAL_UINT32(65535U, WS169_ChunkSize(65536U, 65535U));
    TEST_ASSERT_EQUAL_UINT32(65535U, WS169_ChunkSize(UINT32_MAX, 65535U));
    TEST_ASSERT_EQUAL_UINT32(65535U, WS169_DMA_MAX_PIXELS);
}

/* ---- the transfer plan --------------------------------------------------------------------- */

void test_the_full_screen_is_two_contiguous_chunks(void)
{
    WS169_FlushPlan_t plan;
    uint32_t offset = 99U;
    uint16_t count = 99U;

    WS169_FlushPlanInit(&plan, 280U, 0U, 0U, 280U, 240U);
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
    TEST_ASSERT_EQUAL_UINT32(0U, offset);
    TEST_ASSERT_EQUAL_UINT16(65535U, count);
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
    TEST_ASSERT_EQUAL_UINT32(65535U, offset);
    TEST_ASSERT_EQUAL_UINT16(1665U, count);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, &count));
}

void test_full_width_rows_in_the_middle_start_at_their_row(void)
{
    WS169_FlushPlan_t plan;
    uint32_t offset;
    uint16_t count;

    WS169_FlushPlanInit(&plan, 280U, 0U, 10U, 280U, 5U);
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
    TEST_ASSERT_EQUAL_UINT32(2800U, offset);
    TEST_ASSERT_EQUAL_UINT16(1400U, count);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, &count));
}

void test_a_narrower_rectangle_is_sent_one_row_at_a_time(void)
{
    WS169_FlushPlan_t plan;
    uint32_t offset;
    uint16_t count;

    WS169_FlushPlanInit(&plan, 280U, 5U, 2U, 10U, 3U);
    for (uint32_t row = 0U; row < 3U; row++)
    {
        TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
        TEST_ASSERT_EQUAL_UINT32(((2U + row) * 280U) + 5U, offset);
        TEST_ASSERT_EQUAL_UINT16(10U, count);
    }
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, &count));
}

void test_a_rectangle_at_x_zero_that_is_narrower_than_the_rows_is_still_row_by_row(void)
{
    WS169_FlushPlan_t plan;
    uint32_t offset;
    uint16_t count;

    WS169_FlushPlanInit(&plan, 280U, 0U, 0U, 100U, 2U);
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
    TEST_ASSERT_EQUAL_UINT32(0U, offset);
    TEST_ASSERT_EQUAL_UINT16(100U, count);
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
    TEST_ASSERT_EQUAL_UINT32(280U, offset);
}

void test_an_empty_or_null_plan_has_no_transfers(void)
{
    WS169_FlushPlan_t plan;
    uint32_t offset;
    uint16_t count;

    WS169_FlushPlanInit(&plan, 280U, 0U, 0U, 0U, 5U);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, &count));
    WS169_FlushPlanInit(&plan, 280U, 1U, 0U, 5U, 0U);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, &count));
    WS169_FlushPlanInit(NULL, 280U, 0U, 0U, 1U, 1U);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(NULL, &offset, &count));
    WS169_FlushPlanInit(&plan, 280U, 0U, 0U, 1U, 1U);
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, NULL, &count));
    TEST_ASSERT_FALSE(WS169_FlushPlanNext(&plan, &offset, NULL));
    TEST_ASSERT_TRUE(WS169_FlushPlanNext(&plan, &offset, &count));
}

/* ---- differential tests against the original driver code ----------------------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

/* The original WS169_FlushRectRGB565 argument checks (the framebuffer NULL check stays in the
 * driver), as a "valid" flag. */
static bool original_valid(uint16_t stride, uint16_t display_width, uint16_t display_height,
                           uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    return !((width == 0U) || (height == 0U) || (stride < display_width) || (x >= display_width) ||
             (y >= display_height) || (width > (uint16_t)(display_width - x)) ||
             (height > (uint16_t)(display_height - y)));
}

void test_validation_matches_the_original_checks(void)
{
    unsigned rng = 31U;

    for (unsigned step = 0U; step < 50000U; step++)
    {
        const uint16_t dw = (uint16_t)(1U + (next_random(&rng) % 300U));
        const uint16_t dh = (uint16_t)(1U + (next_random(&rng) % 300U));
        const uint16_t stride =
            (uint16_t)((next_random(&rng) % 3U == 0U) ? (next_random(&rng) % 400U) : dw);
        const bool wild = (next_random(&rng) % 8U) == 0U;
        const uint16_t x =
            wild ? (uint16_t)next_random(&rng) : (uint16_t)(next_random(&rng) % (dw + 2U));
        const uint16_t y =
            wild ? (uint16_t)next_random(&rng) : (uint16_t)(next_random(&rng) % (dh + 2U));
        const uint16_t w =
            wild ? (uint16_t)next_random(&rng) : (uint16_t)(next_random(&rng) % (dw + 2U));
        const uint16_t h =
            wild ? (uint16_t)next_random(&rng) : (uint16_t)(next_random(&rng) % (dh + 2U));

        TEST_ASSERT_EQUAL(original_valid(stride, dw, dh, x, y, w, h),
                          WS169_FlushRectValid(stride, dw, dh, x, y, w, h));
    }
}

/* One transfer: the pixel offset and count. */
typedef struct
{
    uint32_t offset;
    uint16_t count;
} Transfer_t;

/* The original transfer loops of WS169_FlushRectRGB565, recording what it would send. */
static unsigned original_transfers(Transfer_t* out, unsigned capacity, uint16_t stride, uint16_t x,
                                   uint16_t y, uint16_t width, uint16_t height)
{
    unsigned n = 0U;

    if ((x == 0U) && (width == stride))
    {
        uint32_t base = (uint32_t)y * stride;
        uint32_t offset = 0U;
        uint32_t remaining = (uint32_t)width * height;

        while (remaining > 0U)
        {
            uint16_t count = (remaining > 65535U) ? (uint16_t)65535U : (uint16_t)remaining;

            if (n < capacity)
            {
                out[n].offset = base + offset;
                out[n].count = count;
            }
            n++;
            offset += count;
            remaining -= count;
        }
    }
    else
    {
        for (uint16_t row = 0U; row < height; row++)
        {
            if (n < capacity)
            {
                out[n].offset = (((uint32_t)y + row) * stride) + x;
                out[n].count = width;
            }
            n++;
        }
    }
    return n;
}

void test_the_plan_matches_the_original_loops_for_random_rectangles(void)
{
    static Transfer_t expected[300];
    unsigned rng = 41U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        const uint16_t stride =
            (uint16_t)((next_random(&rng) % 2U == 0U) ? 280U : (280U + (next_random(&rng) % 40U)));
        const uint16_t x =
            (uint16_t)((next_random(&rng) % 3U == 0U) ? 0U : (next_random(&rng) % 280U));
        const uint16_t y = (uint16_t)(next_random(&rng) % 240U);
        const uint16_t width =
            (uint16_t)((next_random(&rng) % 3U == 0U) ? stride : (1U + (next_random(&rng) % 280U)));
        const uint16_t height = (uint16_t)(1U + (next_random(&rng) % 240U));
        const unsigned count = original_transfers(expected, 300U, stride, x, y, width, height);
        WS169_FlushPlan_t plan;
        uint32_t offset = 0U;
        uint16_t pixels = 0U;
        unsigned i = 0U;

        WS169_FlushPlanInit(&plan, stride, x, y, width, height);
        while (WS169_FlushPlanNext(&plan, &offset, &pixels))
        {
            TEST_ASSERT_TRUE(i < count);
            TEST_ASSERT_EQUAL_UINT32(expected[i].offset, offset);
            TEST_ASSERT_EQUAL_UINT16(expected[i].count, pixels);
            i++;
        }
        TEST_ASSERT_EQUAL_UINT(count, i);
    }
}

/* Whatever the plan is, every pixel of the rectangle is sent exactly once and no other pixel. */
void test_the_plan_covers_every_pixel_of_the_rectangle_exactly_once(void)
{
    static uint8_t sent[280U * 240U];
    unsigned rng = 51U;

    for (unsigned step = 0U; step < 2000U; step++)
    {
        const uint16_t x =
            (uint16_t)((next_random(&rng) % 3U == 0U) ? 0U : (next_random(&rng) % 280U));
        const uint16_t y = (uint16_t)(next_random(&rng) % 240U);
        const uint16_t width =
            (uint16_t)((next_random(&rng) % 3U == 0U) ? 280U
                                                      : (1U + (next_random(&rng) % (280U - x))));
        const uint16_t height = (uint16_t)(1U + (next_random(&rng) % (240U - y)));
        const uint16_t w = (uint16_t)((x + width > 280U) ? (280U - x) : width);
        WS169_FlushPlan_t plan;
        uint32_t offset;
        uint16_t pixels;

        memset(sent, 0, sizeof(sent));
        WS169_FlushPlanInit(&plan, 280U, x, y, w, height);
        while (WS169_FlushPlanNext(&plan, &offset, &pixels))
        {
            for (uint32_t p = 0U; p < pixels; p++)
            {
                sent[offset + p]++;
            }
        }
        for (uint32_t row = 0U; row < 240U; row++)
        {
            for (uint32_t col = 0U; col < 280U; col++)
            {
                const bool inside = (row >= y) && (row < (uint32_t)y + height) && (col >= x) &&
                                    (col < (uint32_t)x + w);

                TEST_ASSERT_EQUAL_UINT8(inside ? 1U : 0U, sent[(row * 280U) + col]);
            }
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_a_rectangle_inside_the_display_is_valid);
    RUN_TEST(test_an_empty_rectangle_is_invalid);
    RUN_TEST(test_a_rectangle_that_leaves_the_display_is_invalid);
    RUN_TEST(test_framebuffer_rows_narrower_than_the_display_are_invalid);
    RUN_TEST(test_the_chunk_is_the_smaller_of_what_is_left_and_the_maximum);
    RUN_TEST(test_the_full_screen_is_two_contiguous_chunks);
    RUN_TEST(test_full_width_rows_in_the_middle_start_at_their_row);
    RUN_TEST(test_a_narrower_rectangle_is_sent_one_row_at_a_time);
    RUN_TEST(test_a_rectangle_at_x_zero_that_is_narrower_than_the_rows_is_still_row_by_row);
    RUN_TEST(test_an_empty_or_null_plan_has_no_transfers);
    RUN_TEST(test_validation_matches_the_original_checks);
    RUN_TEST(test_the_plan_matches_the_original_loops_for_random_rectangles);
    RUN_TEST(test_the_plan_covers_every_pixel_of_the_rectangle_exactly_once);
    return UNITY_END();
}
