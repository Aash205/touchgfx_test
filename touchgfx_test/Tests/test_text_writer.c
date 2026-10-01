#include "text_writer.h"
#include "unity.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_text_and_characters_are_appended_in_order(void)
{
    char out[32];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutText(&w, "ab");
    TextWriter_PutChar(&w, 'c');
    TextWriter_PutText(&w, "de");
    TEST_ASSERT_EQUAL_UINT(5U, TextWriter_Finish(&w));
    TEST_ASSERT_EQUAL_STRING("abcde", out);
}

void test_an_empty_writer_gives_an_empty_string(void)
{
    char out[4] = {'x', 'x', 'x', 'x'};
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT(0U, TextWriter_Finish(&w));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_a_null_text_is_ignored(void)
{
    char out[8];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutText(&w, "a");
    TextWriter_PutText(&w, NULL);
    TEST_ASSERT_EQUAL_UINT(1U, TextWriter_Finish(&w));
}

void test_numbers_are_decimal_with_optional_zero_padding(void)
{
    char out[40];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutNumber(&w, 0U, 1U);
    TextWriter_PutChar(&w, ' ');
    TextWriter_PutNumber(&w, 7U, 3U);
    TextWriter_PutChar(&w, ' ');
    TextWriter_PutNumber(&w, 12345U, 2U);
    TextWriter_PutChar(&w, ' ');
    TextWriter_PutNumber(&w, 42U, 0U);
    TextWriter_PutChar(&w, ' ');
    TextWriter_PutNumber(&w, UINT32_MAX, 1U);
    (void)TextWriter_Finish(&w);
    TEST_ASSERT_EQUAL_STRING("0 007 12345 42 4294967295", out);
}

void test_padding_wider_than_the_number_keeps_all_zeros(void)
{
    char out[16];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutNumber(&w, 5U, 10U);
    (void)TextWriter_Finish(&w);
    TEST_ASSERT_EQUAL_STRING("0000000005", out);
}

void test_output_is_cut_at_capacity_minus_one_and_terminated(void)
{
    char out[6];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutText(&w, "abcdefgh");
    TEST_ASSERT_EQUAL_UINT(5U, TextWriter_Finish(&w));
    TEST_ASSERT_EQUAL_STRING("abcde", out);
}

void test_characters_after_the_cut_are_still_counted_but_not_stored(void)
{
    char out[4];
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, sizeof(out)));
    TextWriter_PutText(&w, "abcdef");
    TEST_ASSERT_EQUAL_UINT(6U, w.length);
    TEST_ASSERT_EQUAL_UINT(3U, TextWriter_Finish(&w));
}

void test_a_capacity_of_one_stores_only_the_terminator(void)
{
    char out[2] = {'x', 'x'};
    TextWriter_t w;

    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, 1U));
    TextWriter_PutText(&w, "abc");
    TEST_ASSERT_EQUAL_UINT(0U, TextWriter_Finish(&w));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
    TEST_ASSERT_EQUAL_CHAR('x', out[1]);
}

void test_nothing_is_written_past_the_capacity(void)
{
    char out[12];
    TextWriter_t w;

    memset(out, 0x55, sizeof(out));
    TEST_ASSERT_TRUE(TextWriter_Begin(&w, out, 8U));
    TextWriter_PutText(&w, "a very long text that does not fit");
    (void)TextWriter_Finish(&w);
    for (unsigned int i = 8U; i < sizeof(out); i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0x55U, (uint8_t)out[i]);
    }
}

void test_begin_refuses_a_null_buffer_a_zero_capacity_or_a_null_writer(void)
{
    char out[4];
    TextWriter_t w;

    TEST_ASSERT_FALSE(TextWriter_Begin(&w, NULL, 8U));
    TEST_ASSERT_FALSE(TextWriter_Begin(&w, out, 0U));
    TEST_ASSERT_FALSE(TextWriter_Begin(NULL, out, 4U));
}

void test_numbers_match_printf_for_many_values_and_widths(void)
{
    unsigned state = 31U;

    for (unsigned int step = 0U; step < 5000U; step++)
    {
        char expected[32];
        char actual[32];
        TextWriter_t w;
        uint32_t value;
        unsigned width;

        state = (state * 1664525U) + 1013904223U;
        value = state;
        if ((step % 3U) == 0U)
        {
            value >>= (state >> 27) & 0x1FU;
        }
        state = (state * 1664525U) + 1013904223U;
        width = (state >> 8) % 12U;

        (void)snprintf(expected, sizeof(expected), "%0*lu", (int)width, (unsigned long)value);
        TEST_ASSERT_TRUE(TextWriter_Begin(&w, actual, sizeof(actual)));
        TextWriter_PutNumber(&w, value, width);
        (void)TextWriter_Finish(&w);
        TEST_ASSERT_EQUAL_STRING(expected, actual);
    }
}

void test_a_writer_that_could_not_begin_ignores_further_calls(void)
{
    TextWriter_t writer;
    char out[4] = {'x', 'x', 'x', 'x'};

    TEST_ASSERT_FALSE(TextWriter_Begin(&writer, NULL, 8U));
    TextWriter_PutChar(&writer, 'a');
    TextWriter_PutText(&writer, "abc");
    TextWriter_PutNumber(&writer, 42U, 3U);
    TEST_ASSERT_EQUAL_UINT(0U, TextWriter_Finish(&writer));

    TEST_ASSERT_FALSE(TextWriter_Begin(&writer, out, 0U));
    TextWriter_PutText(&writer, "abc");
    TEST_ASSERT_EQUAL_UINT(0U, TextWriter_Finish(&writer));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
}

void test_a_null_writer_is_ignored_by_every_call(void)
{
    TextWriter_PutChar(NULL, 'a');
    TextWriter_PutText(NULL, "abc");
    TextWriter_PutNumber(NULL, 42U, 3U);
    TEST_ASSERT_EQUAL_UINT(0U, TextWriter_Finish(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_text_and_characters_are_appended_in_order);
    RUN_TEST(test_an_empty_writer_gives_an_empty_string);
    RUN_TEST(test_a_null_text_is_ignored);
    RUN_TEST(test_numbers_are_decimal_with_optional_zero_padding);
    RUN_TEST(test_padding_wider_than_the_number_keeps_all_zeros);
    RUN_TEST(test_output_is_cut_at_capacity_minus_one_and_terminated);
    RUN_TEST(test_characters_after_the_cut_are_still_counted_but_not_stored);
    RUN_TEST(test_a_capacity_of_one_stores_only_the_terminator);
    RUN_TEST(test_nothing_is_written_past_the_capacity);
    RUN_TEST(test_begin_refuses_a_null_buffer_a_zero_capacity_or_a_null_writer);
    RUN_TEST(test_a_writer_that_could_not_begin_ignores_further_calls);
    RUN_TEST(test_a_null_writer_is_ignored_by_every_call);
    RUN_TEST(test_numbers_match_printf_for_many_values_and_widths);
    return UNITY_END();
}
