#include "table_dispatch.h"
#include "unity.h"

#include <stddef.h>
#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* The shape of the BlueNRG event tables: a 16-bit code and a handler pointer. */
typedef uint8_t (*Handler_t)(uint8_t* data);

typedef struct
{
    uint16_t evt_code;
    Handler_t process;
} Entry_t;

static uint8_t handler(uint8_t* data)
{
    (void)data;
    return 0U;
}

static const Entry_t plain_table[7] = {{0x0005U, handler}, {0x0008U, handler}, {0x000CU, handler},
                                       {0x0010U, handler}, {0x0013U, handler}, {0x001AU, handler},
                                       {0x0030U, handler}};

static size_t find_in(const Entry_t* table, size_t count, uint16_t code)
{
    return TableDispatch_Find((const uint8_t*)table, sizeof(table[0]), count,
                              offsetof(Entry_t, evt_code), code);
}

/* TableDispatch_Find reads the code as two bytes, least significant first. */
void test_the_machine_is_little_endian_as_table_dispatch_assumes(void)
{
    const uint16_t probe = 0x0102U;

    TEST_ASSERT_EQUAL_UINT8(0x02U, ((const uint8_t*)&probe)[0]);
}

/* ---- route --------------------------------------------------------------------------------- */

void test_the_two_special_event_codes_select_their_sub_tables(void)
{
    TEST_ASSERT_EQUAL_INT(TABLE_DISPATCH_LE_META, TableDispatch_Route(0x3EU));
    TEST_ASSERT_EQUAL_INT(TABLE_DISPATCH_VENDOR, TableDispatch_Route(0xFFU));
}

void test_every_other_event_code_uses_the_plain_table(void)
{
    for (unsigned int code = 0U; code < 256U; code++)
    {
        if ((code != 0x3EU) && (code != 0xFFU))
        {
            TEST_ASSERT_EQUAL_INT(TABLE_DISPATCH_PLAIN, TableDispatch_Route((uint8_t)code));
        }
    }
}

/* ---- find ---------------------------------------------------------------------------------- */

void test_find_returns_the_index_of_the_matching_entry(void)
{
    TEST_ASSERT_EQUAL_UINT(0U, find_in(plain_table, 7U, 0x0005U));
    TEST_ASSERT_EQUAL_UINT(3U, find_in(plain_table, 7U, 0x0010U));
    TEST_ASSERT_EQUAL_UINT(6U, find_in(plain_table, 7U, 0x0030U));
}

void test_find_reports_a_missing_code(void)
{
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND, find_in(plain_table, 7U, 0x0006U));
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND, find_in(plain_table, 7U, 0xFFFFU));
}

void test_find_only_looks_at_the_given_number_of_entries(void)
{
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND, find_in(plain_table, 3U, 0x0010U));
    TEST_ASSERT_EQUAL_UINT(2U, find_in(plain_table, 3U, 0x000CU));
}

void test_find_returns_the_first_of_two_equal_codes(void)
{
    static const Entry_t twice[3] = {{0x0001U, handler}, {0x0002U, handler}, {0x0002U, handler}};

    TEST_ASSERT_EQUAL_UINT(1U, find_in(twice, 3U, 0x0002U));
}

void test_find_reads_a_code_that_is_not_the_first_member(void)
{
    typedef struct
    {
        Handler_t process;
        uint16_t evt_code;
    } Reversed_t;
    static const Reversed_t reversed[3] = {
        {handler, 0x0A0AU}, {handler, 0x0B0BU}, {handler, 0x0C0CU}};

    TEST_ASSERT_EQUAL_UINT(2U, TableDispatch_Find((const uint8_t*)reversed, sizeof(reversed[0]), 3U,
                                                  offsetof(Reversed_t, evt_code), 0x0C0CU));
}

void test_find_rejects_unusable_tables(void)
{
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND,
                           TableDispatch_Find(NULL, sizeof(Entry_t), 7U, 0U, 0x0005U));
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND, find_in(plain_table, 0U, 0x0005U));
    /* an entry too small for a 16-bit code, or an offset that leaves no room for it */
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND,
                           TableDispatch_Find((const uint8_t*)plain_table, 1U, 7U, 0U, 0x0005U));
    TEST_ASSERT_EQUAL_UINT(TABLE_DISPATCH_NOT_FOUND,
                           TableDispatch_Find((const uint8_t*)plain_table, 4U, 7U, 3U, 0x0005U));
    /* a table size that overflows size_t */
    TEST_ASSERT_EQUAL_UINT(
        TABLE_DISPATCH_NOT_FOUND,
        TableDispatch_Find((const uint8_t*)plain_table, 16U, SIZE_MAX / 8U, 0U, 0x0005U));
}

/* ---- differential test against the original loops ------------------------------------------ */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_find_matches_the_original_loop_for_random_tables_and_codes(void)
{
    Entry_t table[43];
    unsigned rng = 77U;

    for (unsigned step = 0U; step < 5000U; step++)
    {
        const size_t count = 1U + (next_random(&rng) % 43U);
        const uint16_t code = (uint16_t)(next_random(&rng) % 64U);
        size_t original = TABLE_DISPATCH_NOT_FOUND;

        for (size_t i = 0U; i < count; i++)
        {
            table[i].evt_code = (uint16_t)(next_random(&rng) % 64U);
            table[i].process = handler;
        }
        /* the original: for each entry, if the code matches use it and break */
        for (size_t i = 0U; i < count; i++)
        {
            if (table[i].evt_code == code)
            {
                original = i;
                break;
            }
        }

        TEST_ASSERT_EQUAL_UINT(original, find_in(table, count, code));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_machine_is_little_endian_as_table_dispatch_assumes);
    RUN_TEST(test_the_two_special_event_codes_select_their_sub_tables);
    RUN_TEST(test_every_other_event_code_uses_the_plain_table);
    RUN_TEST(test_find_returns_the_index_of_the_matching_entry);
    RUN_TEST(test_find_reports_a_missing_code);
    RUN_TEST(test_find_only_looks_at_the_given_number_of_entries);
    RUN_TEST(test_find_returns_the_first_of_two_equal_codes);
    RUN_TEST(test_find_reads_a_code_that_is_not_the_first_member);
    RUN_TEST(test_find_rejects_unusable_tables);
    RUN_TEST(test_find_matches_the_original_loop_for_random_tables_and_codes);
    return UNITY_END();
}
