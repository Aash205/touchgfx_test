#include "unity.h"
#include "ws169_init.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* The expected sequence, written out from the datasheet-derived values of the original driver. */
typedef struct
{
    uint8_t command;
    uint8_t length;
    uint8_t data[14];
} Expected_t;

static const Expected_t expected[14] = {{0x3AU, 1U, {0x55U}},
                                        {0xB2U, 5U, {0x0BU, 0x0BU, 0x00U, 0x33U, 0x35U}},
                                        {0xB7U, 1U, {0x11U}},
                                        {0xBBU, 1U, {0x35U}},
                                        {0xC0U, 1U, {0x2CU}},
                                        {0xC2U, 1U, {0x01U}},
                                        {0xC3U, 1U, {0x0DU}},
                                        {0xC4U, 1U, {0x20U}},
                                        {0xC6U, 1U, {0x13U}},
                                        {0xD0U, 2U, {0xA4U, 0xA1U}},
                                        {0xD6U, 1U, {0xA1U}},
                                        {0xE0U,
                                         14U,
                                         {0xF0U, 0x06U, 0x0BU, 0x0AU, 0x09U, 0x26U, 0x29U, 0x33U,
                                          0x41U, 0x18U, 0x16U, 0x15U, 0x29U, 0x2DU}},
                                        {0xE1U,
                                         14U,
                                         {0xF0U, 0x04U, 0x08U, 0x08U, 0x07U, 0x03U, 0x28U, 0x32U,
                                          0x40U, 0x3BU, 0x19U, 0x18U, 0x2AU, 0x2EU}},
                                        {0xE4U, 3U, {0x25U, 0x00U, 0x00U}}};

void test_there_are_fourteen_commands(void)
{
    TEST_ASSERT_EQUAL_UINT(14U, WS169_InitCommandCount());
}

void test_every_command_has_the_exact_bytes_of_the_original_sequence(void)
{
    for (size_t i = 0U; i < 14U; i++)
    {
        WS169_InitCommand_t command;

        TEST_ASSERT_TRUE(WS169_InitCommandAt(i, &command));
        TEST_ASSERT_EQUAL_HEX8(expected[i].command, command.command);
        TEST_ASSERT_EQUAL_UINT8(expected[i].length, command.length);
        TEST_ASSERT_NOT_NULL(command.data);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(expected[i].data, command.data, expected[i].length);
    }
}

void test_an_index_past_the_end_or_a_null_output_is_refused(void)
{
    WS169_InitCommand_t command = {0x77U, NULL, 0U};

    TEST_ASSERT_FALSE(WS169_InitCommandAt(14U, &command));
    TEST_ASSERT_FALSE(WS169_InitCommandAt(SIZE_MAX, &command));
    TEST_ASSERT_EQUAL_HEX8(0x77U, command.command);
    TEST_ASSERT_FALSE(WS169_InitCommandAt(0U, NULL));
}

void test_the_commands_are_distinct_and_in_ascending_order(void)
{
    uint8_t previous = 0U;

    for (size_t i = 0U; i < WS169_InitCommandCount(); i++)
    {
        WS169_InitCommand_t command;

        TEST_ASSERT_TRUE(WS169_InitCommandAt(i, &command));
        TEST_ASSERT_TRUE(command.command > previous);
        previous = command.command;
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_there_are_fourteen_commands);
    RUN_TEST(test_every_command_has_the_exact_bytes_of_the_original_sequence);
    RUN_TEST(test_an_index_past_the_end_or_a_null_output_is_refused);
    RUN_TEST(test_the_commands_are_distinct_and_in_ascending_order);
    return UNITY_END();
}
