#include "ring.h"
#include "unity.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void fill_sequence(unsigned char* out, unsigned count, unsigned char first)
{
    for (unsigned i = 0U; i < count; i++)
    {
        out[i] = (unsigned char)(first + i);
    }
}

void test_push_into_empty_ring_accepts_everything(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    const unsigned char data[5] = {1U, 2U, 3U, 4U, 5U};

    TEST_ASSERT_EQUAL_UINT(5U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(5U, Ring_Count(&ring));
}

void test_push_beyond_capacity_keeps_the_oldest_bytes(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char data[12];
    unsigned char out[12] = {0};

    fill_sequence(data, sizeof(data), 10U);

    TEST_ASSERT_EQUAL_UINT(8U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(8U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 8U);
}

void test_push_into_full_ring_accepts_nothing(void)
{
    unsigned char storage[4];
    Ring_t ring = RING_INITIALIZER(storage);
    const unsigned char data[4] = {1U, 2U, 3U, 4U};

    TEST_ASSERT_EQUAL_UINT(4U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(&ring, data, 1U));
    TEST_ASSERT_EQUAL_UINT(4U, Ring_Count(&ring));
}

void test_pop_from_empty_ring_returns_nothing(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char out[4] = {0};

    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Count(&ring));
}

void test_pop_is_fifo_and_respects_the_limit(void)
{
    unsigned char storage[128];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char data[100];
    unsigned char out[64];

    fill_sequence(data, sizeof(data), 0U);
    TEST_ASSERT_EQUAL_UINT(100U, Ring_Push(&ring, data, sizeof(data)));

    TEST_ASSERT_EQUAL_UINT(64U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data[0], out, 64U);

    TEST_ASSERT_EQUAL_UINT(36U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data[64], out, 36U);

    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, out, sizeof(out)));
}

void test_indices_wrap_inside_the_buffer(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char first[6];
    unsigned char second[6];
    unsigned char out[8];
    unsigned char expected[8];

    fill_sequence(first, sizeof(first), 0U);
    fill_sequence(second, sizeof(second), 100U);

    TEST_ASSERT_EQUAL_UINT(6U, Ring_Push(&ring, first, sizeof(first)));
    TEST_ASSERT_EQUAL_UINT(4U, Ring_Pop(&ring, out, 4U));
    TEST_ASSERT_EQUAL_UINT(6U, Ring_Push(&ring, second, sizeof(second)));
    TEST_ASSERT_EQUAL_UINT(8U, Ring_Count(&ring));

    TEST_ASSERT_EQUAL_UINT(8U, Ring_Pop(&ring, out, sizeof(out)));
    (void)memcpy(&expected[0], &first[4], 2U);
    (void)memcpy(&expected[2], second, 6U);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, 8U);
}

void test_counters_survive_unsigned_overflow(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char data[6];
    unsigned char out[8];

    fill_sequence(data, sizeof(data), 50U);
    ring.head = UINT_MAX - 2U;
    ring.tail = UINT_MAX - 2U;

    TEST_ASSERT_EQUAL_UINT(6U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(6U, Ring_Count(&ring));
    TEST_ASSERT_EQUAL_UINT(6U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 6U);
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Count(&ring));
}

void test_space_freed_by_pop_can_be_refilled(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char data[8];
    unsigned char out[8];

    fill_sequence(data, sizeof(data), 0U);
    TEST_ASSERT_EQUAL_UINT(8U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(3U, Ring_Pop(&ring, out, 3U));

    TEST_ASSERT_EQUAL_UINT(3U, Ring_Push(&ring, data, 5U));
    TEST_ASSERT_EQUAL_UINT(8U, Ring_Count(&ring));
}

void test_null_and_zero_arguments_are_ignored(void)
{
    unsigned char storage[8];
    Ring_t ring = RING_INITIALIZER(storage);
    const unsigned char data[2] = {1U, 2U};
    unsigned char out[2];

    TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(&ring, NULL, 2U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(&ring, data, 0U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(NULL, data, 2U));
    TEST_ASSERT_EQUAL_UINT(2U, Ring_Push(&ring, data, 2U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, NULL, 2U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, out, 0U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(NULL, out, 2U));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Count(NULL));
    TEST_ASSERT_EQUAL_UINT(2U, Ring_Count(&ring));
}

void test_invalid_capacity_is_rejected(void)
{
    unsigned char storage[8];
    const unsigned char data[2] = {1U, 2U};
    unsigned char out[2];
    const unsigned bad_capacities[] = {0U, 3U, 6U, 2047U};

    for (unsigned i = 0U; i < sizeof(bad_capacities) / sizeof(bad_capacities[0]); i++)
    {
        Ring_t ring = RING_INITIALIZER(storage);
        ring.capacity = bad_capacities[i];

        TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(&ring, data, sizeof(data)));
        TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, out, sizeof(out)));
        TEST_ASSERT_EQUAL_UINT(0U, Ring_Count(&ring));
    }
}

void test_null_storage_is_rejected(void)
{
    Ring_t ring = {NULL, 8U, 0U, 0U};
    const unsigned char data[2] = {1U, 2U};
    unsigned char out[2];

    TEST_ASSERT_EQUAL_UINT(0U, Ring_Push(&ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Pop(&ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT(0U, Ring_Count(&ring));
}

void test_count_tracks_pushes_and_pops(void)
{
    unsigned char storage[16];
    Ring_t ring = RING_INITIALIZER(storage);
    unsigned char data[10];
    unsigned char out[16];

    fill_sequence(data, sizeof(data), 0U);

    (void)Ring_Push(&ring, data, 10U);
    TEST_ASSERT_EQUAL_UINT(10U, Ring_Count(&ring));
    (void)Ring_Pop(&ring, out, 4U);
    TEST_ASSERT_EQUAL_UINT(6U, Ring_Count(&ring));
    (void)Ring_Push(&ring, data, 10U);
    TEST_ASSERT_EQUAL_UINT(16U, Ring_Count(&ring));
}

/* ---- differential test against the original usb_cdc_log.c algorithm ---------------------- */

#define REF_SIZE 16U

/* The original loops from usb_cdc_log.c, kept verbatim in shape as the reference model. */
typedef struct
{
    unsigned char ring[REF_SIZE];
    unsigned head;
    unsigned tail;
} RefRing_t;

static unsigned ref_push(RefRing_t* r, const unsigned char* data, unsigned size)
{
    unsigned n = 0U;

    while ((n < size) && ((r->head - r->tail) < REF_SIZE))
    {
        r->ring[r->head & (REF_SIZE - 1U)] = data[n];
        n++;
        r->head++;
    }

    return n;
}

static unsigned ref_pop(RefRing_t* r, unsigned char* out, unsigned max)
{
    unsigned n = 0U;

    while ((r->tail != r->head) && (n < max))
    {
        out[n] = r->ring[r->tail & (REF_SIZE - 1U)];
        n++;
        r->tail++;
    }

    return n;
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

static void run_differential(unsigned start_counter)
{
    unsigned char storage[REF_SIZE];
    Ring_t ring = RING_INITIALIZER(storage);
    RefRing_t ref;
    unsigned state = 12345U;

    (void)memset(&ref, 0, sizeof(ref));
    ring.head = start_counter;
    ring.tail = start_counter;
    ref.head = start_counter;
    ref.tail = start_counter;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        unsigned char data[40];
        unsigned char out_a[40];
        unsigned char out_b[40];
        const unsigned amount = next_random(&state) % 41U;

        if ((next_random(&state) & 1U) != 0U)
        {
            for (unsigned i = 0U; i < amount; i++)
            {
                data[i] = (unsigned char)next_random(&state);
            }
            TEST_ASSERT_EQUAL_UINT(ref_push(&ref, data, amount), Ring_Push(&ring, data, amount));
        }
        else
        {
            const unsigned got = Ring_Pop(&ring, out_a, amount);

            TEST_ASSERT_EQUAL_UINT(ref_pop(&ref, out_b, amount), got);
            if (got > 0U)
            {
                TEST_ASSERT_EQUAL_UINT8_ARRAY(out_b, out_a, got);
            }
        }

        TEST_ASSERT_EQUAL_UINT(ref.head - ref.tail, Ring_Count(&ring));
        TEST_ASSERT_EQUAL_UINT(ref.head, ring.head);
        TEST_ASSERT_EQUAL_UINT(ref.tail, ring.tail);
    }
}

void test_matches_the_original_algorithm(void)
{
    run_differential(0U);
}

void test_matches_the_original_algorithm_across_counter_overflow(void)
{
    run_differential(UINT_MAX - 5U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_push_into_empty_ring_accepts_everything);
    RUN_TEST(test_push_beyond_capacity_keeps_the_oldest_bytes);
    RUN_TEST(test_push_into_full_ring_accepts_nothing);
    RUN_TEST(test_pop_from_empty_ring_returns_nothing);
    RUN_TEST(test_pop_is_fifo_and_respects_the_limit);
    RUN_TEST(test_indices_wrap_inside_the_buffer);
    RUN_TEST(test_counters_survive_unsigned_overflow);
    RUN_TEST(test_space_freed_by_pop_can_be_refilled);
    RUN_TEST(test_null_and_zero_arguments_are_ignored);
    RUN_TEST(test_invalid_capacity_is_rejected);
    RUN_TEST(test_null_storage_is_rejected);
    RUN_TEST(test_count_tracks_pushes_and_pops);
    RUN_TEST(test_matches_the_original_algorithm);
    RUN_TEST(test_matches_the_original_algorithm_across_counter_overflow);
    return UNITY_END();
}
