#include "uart_line.h"
#include "unity.h"

#include <stddef.h>
#include <stdint.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* Feed a NUL-terminated string through the line assembler, one byte at a time. */
static void feed(char* buffer, uint8_t capacity, uint8_t* index, uint8_t* ready, const char* text)
{
    for (; *text != '\0'; text++)
    {
        UART_LineFeedByte(buffer, capacity, index, ready, (uint8_t)*text);
    }
}

void test_empty_crlf_lines_are_ignored(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    feed(buffer, sizeof(buffer), &index, &ready, "\r\n");

    TEST_ASSERT_EQUAL_UINT8(0U, index);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
}

void test_cr_terminates_a_non_empty_line(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    feed(buffer, sizeof(buffer), &index, &ready, "A\r");

    TEST_ASSERT_EQUAL_UINT8(1U, ready);
    TEST_ASSERT_EQUAL_UINT8(1U, index);
    TEST_ASSERT_EQUAL_STRING("A", buffer);
}

void test_pending_command_buffer_is_retained(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    feed(buffer, sizeof(buffer), &index, &ready, "A\rB");

    TEST_ASSERT_EQUAL_UINT8(1U, index);
    TEST_ASSERT_EQUAL_UINT8(1U, ready);
    TEST_ASSERT_EQUAL_STRING("A", buffer);
}

void test_line_fills_buffer_without_overflowing(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    feed(buffer, sizeof(buffer), &index, &ready, "0123456X");

    TEST_ASSERT_EQUAL_UINT8(7U, index);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[7]);
}

void test_full_line_can_be_terminated(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    feed(buffer, sizeof(buffer), &index, &ready, "0123456X\n");

    TEST_ASSERT_EQUAL_UINT8(1U, ready);
    TEST_ASSERT_EQUAL_STRING("0123456", buffer);
}

void test_invalid_index_is_rejected_without_buffer_access(void)
{
    char buffer[8] = {0};
    uint8_t index = 8U;
    uint8_t ready = 0U;

    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, (uint8_t)'\r');

    TEST_ASSERT_EQUAL_UINT8(8U, index);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
}

void test_zero_buffer_capacity_is_ignored(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    UART_LineFeedByte(buffer, 0U, &index, &ready, (uint8_t)'Q');

    TEST_ASSERT_EQUAL_UINT8(0U, index);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
}

void test_a_null_buffer_index_or_ready_flag_is_ignored(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    UART_LineFeedByte(NULL, sizeof(buffer), &index, &ready, (uint8_t)'A');
    UART_LineFeedByte(buffer, sizeof(buffer), NULL, &ready, (uint8_t)'A');
    UART_LineFeedByte(buffer, sizeof(buffer), &index, NULL, (uint8_t)'A');

    TEST_ASSERT_EQUAL_UINT8(0U, index);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);
}

void test_every_byte_value_is_stored_except_cr_and_lf(void)
{
    for (unsigned int value = 0U; value < 256U; value++)
    {
        char buffer[4] = {0};
        uint8_t index = 0U;
        uint8_t ready = 0U;

        UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, (uint8_t)value);
        if ((value == 13U) || (value == 10U))
        {
            TEST_ASSERT_EQUAL_UINT8(0U, index); /* an empty line is ignored */
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8(1U, index);
            TEST_ASSERT_EQUAL_UINT8((uint8_t)value, (uint8_t)buffer[0]);
        }
        TEST_ASSERT_EQUAL_UINT8(0U, ready);
    }
}

void test_a_character_arriving_at_a_full_buffer_is_dropped(void)
{
    char buffer[4] = {'a', 'b', 'c', 'X'};
    uint8_t index = 3U; /* capacity - 1: only the terminator would fit */
    uint8_t ready = 0U;

    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, (uint8_t)'d');

    TEST_ASSERT_EQUAL_UINT8(3U, index);
    TEST_ASSERT_EQUAL_CHAR('X', buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0U, ready);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_crlf_lines_are_ignored);
    RUN_TEST(test_cr_terminates_a_non_empty_line);
    RUN_TEST(test_pending_command_buffer_is_retained);
    RUN_TEST(test_line_fills_buffer_without_overflowing);
    RUN_TEST(test_full_line_can_be_terminated);
    RUN_TEST(test_invalid_index_is_rejected_without_buffer_access);
    RUN_TEST(test_zero_buffer_capacity_is_ignored);
    RUN_TEST(test_a_null_buffer_index_or_ready_flag_is_ignored);
    RUN_TEST(test_every_byte_value_is_stored_except_cr_and_lf);
    RUN_TEST(test_a_character_arriving_at_a_full_buffer_is_dropped);
    return UNITY_END();
}
