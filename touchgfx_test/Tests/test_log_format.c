#include "log_format.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- level tags and filter -------------------------------------------------------------- */

static void assert_line_starts_with_tag(LogLevelTypeDef level, const char* tag)
{
    char line[64];
    const size_t tag_length = strlen(tag);

    (void)LogFormat_Line(line, sizeof(line), level, 0U, "m");

    TEST_ASSERT_EQUAL_STRING_LEN(tag, line, tag_length);
    TEST_ASSERT_EQUAL_CHAR(' ', line[tag_length]);
}

void test_every_level_has_its_tag(void)
{
    assert_line_starts_with_tag(LOG_LEVEL_DEBUG, "[DEBUG]");
    assert_line_starts_with_tag(LOG_LEVEL_INFO, "[INFO]");
    assert_line_starts_with_tag(LOG_LEVEL_WARNING, "[WARN]");
    assert_line_starts_with_tag(LOG_LEVEL_ERROR, "[ERROR]");
    assert_line_starts_with_tag(LOG_LEVEL_CRITICAL, "[CRIT]");
}

void test_the_tag_function_returns_each_tag(void)
{
    TEST_ASSERT_EQUAL_STRING("[DEBUG]", LogFormat_LevelTag(LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL_STRING("[INFO]", LogFormat_LevelTag(LOG_LEVEL_INFO));
    TEST_ASSERT_EQUAL_STRING("[WARN]", LogFormat_LevelTag(LOG_LEVEL_WARNING));
    TEST_ASSERT_EQUAL_STRING("[ERROR]", LogFormat_LevelTag(LOG_LEVEL_ERROR));
    TEST_ASSERT_EQUAL_STRING("[CRIT]", LogFormat_LevelTag(LOG_LEVEL_CRITICAL));
    TEST_ASSERT_EQUAL_STRING("[?]", LogFormat_LevelTag((LogLevelTypeDef)5));
}

void test_an_unknown_level_gets_the_placeholder_tag(void)
{
    assert_line_starts_with_tag((LogLevelTypeDef)5, "[?]");
    assert_line_starts_with_tag((LogLevelTypeDef)255, "[?]");
}

void test_the_filter_passes_the_minimum_level_and_above(void)
{
    TEST_ASSERT_TRUE(LogFormat_ShouldLog(LOG_LEVEL_INFO, LOG_LEVEL_INFO));
    TEST_ASSERT_TRUE(LogFormat_ShouldLog(LOG_LEVEL_ERROR, LOG_LEVEL_INFO));
    TEST_ASSERT_TRUE(LogFormat_ShouldLog(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG));
    TEST_ASSERT_TRUE(LogFormat_ShouldLog(LOG_LEVEL_CRITICAL, LOG_LEVEL_CRITICAL));
}

void test_the_filter_blocks_levels_below_the_minimum(void)
{
    TEST_ASSERT_FALSE(LogFormat_ShouldLog(LOG_LEVEL_DEBUG, LOG_LEVEL_INFO));
    TEST_ASSERT_FALSE(LogFormat_ShouldLog(LOG_LEVEL_WARNING, LOG_LEVEL_ERROR));
    TEST_ASSERT_FALSE(LogFormat_ShouldLog(LOG_LEVEL_ERROR, LOG_LEVEL_CRITICAL));
}

/* ---- line layout ------------------------------------------------------------------------- */

void test_a_line_has_tag_timestamp_message_and_crlf(void)
{
    char line[64];
    const size_t length = LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 12345U, "hello");

    TEST_ASSERT_EQUAL_STRING("[INFO] [0012.345] hello\r\n", line);
    TEST_ASSERT_EQUAL_UINT(25U, length);
}

void test_the_timestamp_is_zero_padded(void)
{
    char line[64];

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_DEBUG, 0U, "x");
    TEST_ASSERT_EQUAL_STRING("[DEBUG] [0000.000] x\r\n", line);

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_DEBUG, 999U, "x");
    TEST_ASSERT_EQUAL_STRING("[DEBUG] [0000.999] x\r\n", line);

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_DEBUG, 1000U, "x");
    TEST_ASSERT_EQUAL_STRING("[DEBUG] [0001.000] x\r\n", line);
}

void test_the_seconds_field_grows_beyond_four_digits(void)
{
    char line[64];

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_ERROR, 123456789U, "x");
    TEST_ASSERT_EQUAL_STRING("[ERROR] [123456.789] x\r\n", line);

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_ERROR, UINT32_MAX, "x");
    TEST_ASSERT_EQUAL_STRING("[ERROR] [4294967.295] x\r\n", line);
}

void test_a_null_message_is_an_empty_message(void)
{
    char line[64];

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_WARNING, 5000U, NULL);
    TEST_ASSERT_EQUAL_STRING("[WARN] [0005.000] \r\n", line);
}

void test_percent_signs_in_the_message_are_not_interpreted(void)
{
    char line[64];

    (void)LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 0U, "100%s %d");
    TEST_ASSERT_EQUAL_STRING("[INFO] [0000.000] 100%s %d\r\n", line);
}

/* ---- truncation and bad arguments ---------------------------------------------------------- */

void test_a_line_that_just_fits_keeps_its_crlf(void)
{
    char line[26];
    const size_t length = LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 12345U, "hello");

    TEST_ASSERT_EQUAL_UINT(25U, length);
    TEST_ASSERT_EQUAL_STRING("[INFO] [0012.345] hello\r\n", line);
}

void test_one_byte_too_small_cuts_the_line_feed(void)
{
    char line[25];
    const size_t length = LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 12345U, "hello");

    TEST_ASSERT_EQUAL_UINT(24U, length);
    TEST_ASSERT_EQUAL_STRING("[INFO] [0012.345] hello\r", line);
}

void test_a_long_message_is_truncated_and_terminated(void)
{
    char line[20];
    const size_t length = LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 12345U,
                                         "a long message that cannot fit");

    TEST_ASSERT_EQUAL_UINT(19U, length);
    TEST_ASSERT_EQUAL_STRING("[INFO] [0012.345] a", line);
}

void test_a_capacity_of_one_stores_only_the_terminator(void)
{
    char line[1] = {'x'};

    TEST_ASSERT_EQUAL_UINT(0U, LogFormat_Line(line, sizeof(line), LOG_LEVEL_INFO, 0U, "m"));
    TEST_ASSERT_EQUAL_CHAR('\0', line[0]);
}

void test_null_output_or_zero_capacity_stores_nothing(void)
{
    char line[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, LogFormat_Line(NULL, 8U, LOG_LEVEL_INFO, 0U, "m"));
    TEST_ASSERT_EQUAL_UINT(0U, LogFormat_Line(line, 0U, LOG_LEVEL_INFO, 0U, "m"));
    TEST_ASSERT_EQUAL_CHAR('x', line[0]);
}

/* ---- differential test against the original usb_logging.c code ----------------------------- */

/* The original level switch and snprintf layout, kept as the reference model. */
static const char* original_tag(LogLevelTypeDef level)
{
    const char* level_str;

    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        level_str = "[DEBUG]";
        break;
    case LOG_LEVEL_INFO:
        level_str = "[INFO]";
        break;
    case LOG_LEVEL_WARNING:
        level_str = "[WARN]";
        break;
    case LOG_LEVEL_ERROR:
        level_str = "[ERROR]";
        break;
    case LOG_LEVEL_CRITICAL:
        level_str = "[CRIT]";
        break;
    default:
        level_str = "[?]";
        break;
    }

    return level_str;
}

static size_t original_line(char* formatted, size_t capacity, LogLevelTypeDef level, unsigned ms,
                            const char* message)
{
    const int len = snprintf(formatted, capacity, "%s [%04lu.%03lu] %s\r\n", original_tag(level),
                             (unsigned long)(ms / 1000U), (unsigned long)(ms % 1000U), message);
    size_t transmitted = (size_t)len;

    if (transmitted >= capacity)
    {
        transmitted = capacity - 1U;
    }

    return transmitted;
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_matches_the_original_snprintf_layout(void)
{
    unsigned state = 4242U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        char message[300];
        char expected[300];
        char actual[300];
        const unsigned message_length = next_random(&state) % 290U;
        const size_t capacity = 1U + (next_random(&state) % 290U);
        const LogLevelTypeDef level = (LogLevelTypeDef)(next_random(&state) % 8U);
        unsigned ms = next_random(&state);
        size_t expected_length;
        size_t actual_length;

        if ((next_random(&state) & 3U) == 0U)
        {
            ms = (ms % 4U == 0U) ? UINT32_MAX : ms * 1000U;
        }
        for (unsigned i = 0U; i < message_length; i++)
        {
            message[i] = (char)(' ' + (next_random(&state) % 95U));
        }
        message[message_length] = '\0';

        expected_length = original_line(expected, capacity, level, ms, message);
        actual_length = LogFormat_Line(actual, capacity, level, ms, message);

        TEST_ASSERT_EQUAL_UINT(expected_length, actual_length);
        TEST_ASSERT_EQUAL_STRING(expected, actual);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_level_has_its_tag);
    RUN_TEST(test_the_tag_function_returns_each_tag);
    RUN_TEST(test_an_unknown_level_gets_the_placeholder_tag);
    RUN_TEST(test_the_filter_passes_the_minimum_level_and_above);
    RUN_TEST(test_the_filter_blocks_levels_below_the_minimum);
    RUN_TEST(test_a_line_has_tag_timestamp_message_and_crlf);
    RUN_TEST(test_the_timestamp_is_zero_padded);
    RUN_TEST(test_the_seconds_field_grows_beyond_four_digits);
    RUN_TEST(test_a_null_message_is_an_empty_message);
    RUN_TEST(test_percent_signs_in_the_message_are_not_interpreted);
    RUN_TEST(test_a_line_that_just_fits_keeps_its_crlf);
    RUN_TEST(test_one_byte_too_small_cuts_the_line_feed);
    RUN_TEST(test_a_long_message_is_truncated_and_terminated);
    RUN_TEST(test_a_capacity_of_one_stores_only_the_terminator);
    RUN_TEST(test_null_output_or_zero_capacity_stores_nothing);
    RUN_TEST(test_matches_the_original_snprintf_layout);
    return UNITY_END();
}
