#include "cmd_parse.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- parsing ------------------------------------------------------------------------------- */

static void assert_led(const char* line, uint8_t led_count, uint8_t index, LED_StateTypeDef action)
{
    const Cmd_t cmd = Cmd_Parse(line, led_count);

    TEST_ASSERT_EQUAL_INT_MESSAGE(CMD_LED, cmd.kind, line);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(index, cmd.led_index, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(action, cmd.action, line);
}

static void assert_kind(const char* line, uint8_t led_count, CmdKind_t kind)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(kind, Cmd_Parse(line, led_count).kind, line);
}

void test_led_commands_with_every_action(void)
{
    assert_led("LED0 ON", 2U, 0U, LED_ON);
    assert_led("LED1 OFF", 2U, 1U, LED_OFF);
    assert_led("LED0 TOGGLE", 2U, 0U, LED_TOGGLE);
    assert_led("LED1 BLINK", 2U, 1U, LED_BLINK_SLOW);
    assert_led("LED0 FAST", 2U, 0U, LED_BLINK_FAST);
}

void test_the_led_number_is_the_character_right_after_led(void)
{
    assert_led("LED3ON", 4U, 3U, LED_ON);
    assert_led("LED2X OFF", 3U, 2U, LED_OFF);
}

void test_the_action_is_found_anywhere_in_the_line_in_a_fixed_order(void)
{
    assert_led("LED0 BLINK ONCE", 2U, 0U, LED_ON);
    assert_led("LED0 FAST BLINK", 2U, 0U, LED_BLINK_FAST);
    assert_led("LED0 TOGGLE OFF", 2U, 0U, LED_OFF);
    assert_led("LED0 X TOGGLE", 2U, 0U, LED_TOGGLE);
}

void test_an_led_without_an_action_is_an_invalid_command(void)
{
    assert_kind("LED0", 2U, CMD_INVALID_ACTION);
    assert_kind("LED1 DANCE", 2U, CMD_INVALID_ACTION);
    assert_kind("LED0 on", 2U, CMD_INVALID_ACTION);
}

void test_an_led_number_that_is_missing_not_a_digit_or_too_big_is_an_invalid_led(void)
{
    assert_kind("LED", 2U, CMD_INVALID_LED);
    assert_kind("LED ON", 2U, CMD_INVALID_LED);
    assert_kind("LEDS", 2U, CMD_INVALID_LED);
    assert_kind("LED2 ON", 2U, CMD_INVALID_LED);
    assert_kind("LED9 ON", 2U, CMD_INVALID_LED);
    assert_kind("LED0 ON", 0U, CMD_INVALID_LED);
}

void test_status_ble_and_help_are_prefix_matches(void)
{
    assert_kind("STATUS", 2U, CMD_STATUS);
    assert_kind("STATUSX", 2U, CMD_STATUS);
    assert_kind("BLE", 2U, CMD_BLE);
    assert_kind("BLEFOO", 2U, CMD_BLE);
    assert_kind("HELP", 2U, CMD_HELP);
    assert_kind("HELPME", 2U, CMD_HELP);
}

void test_a_truncated_command_word_is_unknown(void)
{
    assert_kind("STATU", 2U, CMD_UNKNOWN);
    assert_kind("STATUX", 2U, CMD_UNKNOWN);
    assert_kind("BL", 2U, CMD_UNKNOWN);
    assert_kind("HEL", 2U, CMD_UNKNOWN);
    assert_kind("HELX", 2U, CMD_UNKNOWN);
    assert_kind("LE", 2U, CMD_UNKNOWN);
}

void test_the_led_prefix_wins_over_the_other_commands(void)
{
    assert_kind("LEDSTATUS", 2U, CMD_INVALID_LED);
}

void test_unknown_empty_lower_case_and_null_lines(void)
{
    assert_kind("", 2U, CMD_UNKNOWN);
    assert_kind("HELLO", 2U, CMD_UNKNOWN);
    assert_kind("led0 on", 2U, CMD_UNKNOWN);
    assert_kind("status", 2U, CMD_UNKNOWN);
    assert_kind(" STATUS", 2U, CMD_UNKNOWN);
    assert_kind(NULL, 2U, CMD_UNKNOWN);
}

/* ---- reply formatting ---------------------------------------------------------------------- */

void test_led_replies(void)
{
    char out[64];

    TEST_ASSERT_EQUAL_UINT(10U, Cmd_FormatLedReply(out, sizeof(out), 0U, LED_ON));
    TEST_ASSERT_EQUAL_STRING("LED0: ON\r\n", out);
    (void)Cmd_FormatLedReply(out, sizeof(out), 1U, LED_OFF);
    TEST_ASSERT_EQUAL_STRING("LED1: OFF\r\n", out);
    (void)Cmd_FormatLedReply(out, sizeof(out), 1U, LED_BLINK_SLOW);
    TEST_ASSERT_EQUAL_STRING("LED1: BLINK\r\n", out);
    (void)Cmd_FormatLedReply(out, sizeof(out), 0U, LED_BLINK_FAST);
    TEST_ASSERT_EQUAL_STRING("LED0: BLINK FAST\r\n", out);
    (void)Cmd_FormatLedReply(out, sizeof(out), 3U, LED_TOGGLE);
    TEST_ASSERT_EQUAL_STRING("LED3: TOGGLE\r\n", out);
    (void)Cmd_FormatLedReply(out, sizeof(out), 0U, (LED_StateTypeDef)9);
    TEST_ASSERT_EQUAL_STRING("LED0: UNKNOWN\r\n", out);
}

void test_status_reply_lists_every_led(void)
{
    char out[128];
    const LED_StateTypeDef states[4] = {LED_ON, LED_OFF, LED_BLINK_SLOW, LED_BLINK_FAST};

    (void)Cmd_FormatStatus(out, sizeof(out), states, 4U);
    TEST_ASSERT_EQUAL_STRING("=== Status ===\r\nLED0: ON\r\nLED1: OFF\r\nLED2: BLINK_SLOW\r\n"
                             "LED3: BLINK_FAST\r\n",
                             out);
}

void test_status_reply_with_no_leds_is_just_the_header(void)
{
    char out[128];

    (void)Cmd_FormatStatus(out, sizeof(out), NULL, 0U);
    TEST_ASSERT_EQUAL_STRING("=== Status ===\r\n", out);
    (void)Cmd_FormatStatus(out, sizeof(out), NULL, 3U);
    TEST_ASSERT_EQUAL_STRING("=== Status ===\r\n", out);
}

void test_status_names_toggle_and_unknown_states(void)
{
    char out[128];
    const LED_StateTypeDef states[2] = {LED_TOGGLE, (LED_StateTypeDef)7};

    (void)Cmd_FormatStatus(out, sizeof(out), states, 2U);
    TEST_ASSERT_EQUAL_STRING("=== Status ===\r\nLED0: TOGGLE\r\nLED1: UNKNOWN\r\n", out);
}

void test_ble_status_reply(void)
{
    char out[64];

    (void)Cmd_FormatBleStatus(out, sizeof(out), 3U);
    TEST_ASSERT_EQUAL_STRING("BLE status: 3\r\n", out);
    (void)Cmd_FormatBleStatus(out, sizeof(out), 0U);
    TEST_ASSERT_EQUAL_STRING("BLE status: 0\r\n", out);
    (void)Cmd_FormatBleStatus(out, sizeof(out), 4294967295U);
    TEST_ASSERT_EQUAL_STRING("BLE status: 4294967295\r\n", out);
}

void test_replies_are_truncated_and_terminated(void)
{
    char out[8];

    TEST_ASSERT_EQUAL_UINT(7U, Cmd_FormatLedReply(out, sizeof(out), 0U, LED_BLINK_FAST));
    TEST_ASSERT_EQUAL_STRING("LED0: B", out);
    TEST_ASSERT_EQUAL_UINT(7U, Cmd_FormatBleStatus(out, sizeof(out), 3U));
    TEST_ASSERT_EQUAL_STRING("BLE sta", out);
    TEST_ASSERT_EQUAL_UINT(7U, Cmd_FormatStatus(out, sizeof(out), NULL, 0U));
    TEST_ASSERT_EQUAL_STRING("=== Sta", out);
}

void test_a_capacity_of_one_stores_only_the_terminator(void)
{
    char out[1] = {'x'};

    TEST_ASSERT_EQUAL_UINT(0U, Cmd_FormatLedReply(out, sizeof(out), 0U, LED_ON));
    TEST_ASSERT_EQUAL_CHAR('\0', out[0]);
}

void test_null_output_or_zero_capacity_stores_nothing(void)
{
    char out[4] = {'x', 'x', 'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, Cmd_FormatLedReply(NULL, 8U, 0U, LED_ON));
    TEST_ASSERT_EQUAL_UINT(0U, Cmd_FormatStatus(NULL, 8U, NULL, 0U));
    TEST_ASSERT_EQUAL_UINT(0U, Cmd_FormatBleStatus(NULL, 8U, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, Cmd_FormatLedReply(out, 0U, 0U, LED_ON));
    TEST_ASSERT_EQUAL_CHAR('x', out[0]);
}

void test_fixed_texts(void)
{
    TEST_ASSERT_EQUAL_STRING("ERROR: Invalid LED\r\n", Cmd_ErrorText(CMD_INVALID_LED));
    TEST_ASSERT_EQUAL_STRING("ERROR: Invalid command\r\n", Cmd_ErrorText(CMD_INVALID_ACTION));
    TEST_ASSERT_EQUAL_STRING("ERROR: Unknown command\r\n", Cmd_ErrorText(CMD_UNKNOWN));
    TEST_ASSERT_NULL(Cmd_ErrorText(CMD_LED));
    TEST_ASSERT_NULL(Cmd_ErrorText(CMD_STATUS));
    TEST_ASSERT_NULL(Cmd_ErrorText(CMD_BLE));
    TEST_ASSERT_NULL(Cmd_ErrorText(CMD_HELP));
}

void test_help_lines(void)
{
    TEST_ASSERT_EQUAL_STRING("Commands:\r\n", Cmd_HelpLine(0U));
    TEST_ASSERT_EQUAL_STRING("  LED<N> ON/OFF/TOGGLE/BLINK/FAST\r\n", Cmd_HelpLine(1U));
    TEST_ASSERT_EQUAL_STRING("  BLE   (status)\r\n", Cmd_HelpLine(2U));
    TEST_ASSERT_EQUAL_STRING("  STATUS\r\n", Cmd_HelpLine(3U));
    TEST_ASSERT_EQUAL_STRING("  HELP\r\n", Cmd_HelpLine(4U));
    TEST_ASSERT_NULL(Cmd_HelpLine(5U));
    TEST_ASSERT_NULL(Cmd_HelpLine(1000U));
}

/* ---- differential test against the original UART_CMD_ParseCommand --------------------------- */

/*
 * What the original console sent for one line, concatenated, plus the LED call it made. The
 *
 * original code, with the HAL calls replaced by recording and the responses collected.
 */
typedef struct
{
    char sent[512];
    int led_call; /* -1 when no SetLEDState call was made */
    LED_StateTypeDef led_action;
} OriginalResult_t;

static void send(OriginalResult_t* result, const char* response)
{
    (void)strncat(result->sent, response, sizeof(result->sent) - strlen(result->sent) - 1U);
}

static void original_get_status(const LED_StateTypeDef* states, uint8_t led_count, char* status_str)
{
    char led_status[96] = "";
    size_t used = 0U;

    for (uint8_t i = 0U; i < led_count; i++)
    {
        char buf[24];
        const char* state_str;

        switch (states[i])
        {
        case LED_ON:
            state_str = "ON";
            break;
        case LED_OFF:
            state_str = "OFF";
            break;
        case LED_TOGGLE:
            state_str = "TOGGLE";
            break;
        case LED_BLINK_SLOW:
            state_str = "BLINK_SLOW";
            break;
        case LED_BLINK_FAST:
            state_str = "BLINK_FAST";
            break;
        default:
            state_str = "UNKNOWN";
            break;
        }

        int len = snprintf(buf, sizeof(buf), "LED%u: %s\r\n", (unsigned)i, state_str);
        if ((len > 0) && ((size_t)len < sizeof(buf)) && (used < sizeof(led_status)))
        {
            int appended = snprintf(&led_status[used], sizeof(led_status) - used, "%s", buf);
            if ((appended > 0) && ((size_t)appended < (sizeof(led_status) - used)))
            {
                used += (size_t)appended;
            }
        }
    }

    snprintf(status_str, 128, "=== Status ===\r\n%s", led_status);
}

static void original_parse(OriginalResult_t* result, const char* cmd, uint8_t led_count,
                           const LED_StateTypeDef* states, int ble_status)
{
    char response[128];

    result->sent[0] = '\0';
    result->led_call = -1;

    if (strncmp(cmd, "LED", 3) == 0)
    {
        int led_num = (int)(cmd[3] - '0');
        if ((cmd[3] < '0') || (cmd[3] > '9') || (led_num < 0) || (led_num >= (int)led_count))
        {
            send(result, "ERROR: Invalid LED\r\n");
            return;
        }

        if (strstr(cmd, "ON") != NULL)
        {
            result->led_call = led_num;
            result->led_action = LED_ON;
            snprintf(response, sizeof(response), "LED%d: ON\r\n", led_num);
        }
        else if (strstr(cmd, "OFF") != NULL)
        {
            result->led_call = led_num;
            result->led_action = LED_OFF;
            snprintf(response, sizeof(response), "LED%d: OFF\r\n", led_num);
        }
        else if (strstr(cmd, "FAST") != NULL)
        {
            result->led_call = led_num;
            result->led_action = LED_BLINK_FAST;
            snprintf(response, sizeof(response), "LED%d: BLINK FAST\r\n", led_num);
        }
        else if (strstr(cmd, "BLINK") != NULL)
        {
            result->led_call = led_num;
            result->led_action = LED_BLINK_SLOW;
            snprintf(response, sizeof(response), "LED%d: BLINK\r\n", led_num);
        }
        else if (strstr(cmd, "TOGGLE") != NULL)
        {
            result->led_call = led_num;
            result->led_action = LED_TOGGLE;
            snprintf(response, sizeof(response), "LED%d: TOGGLE\r\n", led_num);
        }
        else
        {
            send(result, "ERROR: Invalid command\r\n");
            return;
        }
        send(result, response);
    }
    else if (strncmp(cmd, "STATUS", 6) == 0)
    {
        original_get_status(states, led_count, response);
        send(result, response);
    }
    else if (strncmp(cmd, "BLE", 3) == 0)
    {
        snprintf(response, sizeof(response), "BLE status: %d\r\n", ble_status);
        send(result, response);
    }
    else if (strncmp(cmd, "HELP", 4) == 0)
    {
        send(result, "Commands:\r\n");
        send(result, "  LED<N> ON/OFF/TOGGLE/BLINK/FAST\r\n");
        send(result, "  BLE   (status)\r\n");
        send(result, "  STATUS\r\n");
        send(result, "  HELP\r\n");
    }
    else
    {
        send(result, "ERROR: Unknown command\r\n");
    }
}

/* The same line through the new parser and formatters, in the order the firmware uses them. */
static void new_parse(OriginalResult_t* result, const char* line, uint8_t led_count,
                      const LED_StateTypeDef* states, unsigned ble_status)
{
    const Cmd_t cmd = Cmd_Parse(line, led_count);
    char response[128];

    result->sent[0] = '\0';
    result->led_call = -1;

    if (cmd.kind == CMD_LED)
    {
        result->led_call = (int)cmd.led_index;
        result->led_action = cmd.action;
        (void)Cmd_FormatLedReply(response, sizeof(response), cmd.led_index, cmd.action);
        send(result, response);
    }
    else if (cmd.kind == CMD_STATUS)
    {
        (void)Cmd_FormatStatus(response, sizeof(response), states, led_count);
        send(result, response);
    }
    else if (cmd.kind == CMD_BLE)
    {
        (void)Cmd_FormatBleStatus(response, sizeof(response), ble_status);
        send(result, response);
    }
    else if (cmd.kind == CMD_HELP)
    {
        const char* text = Cmd_HelpLine(0U);

        for (unsigned i = 1U; text != NULL; i++)
        {
            send(result, text);
            text = Cmd_HelpLine(i);
        }
    }
    else
    {
        send(result, Cmd_ErrorText(cmd.kind));
    }
}

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_matches_the_original_console_over_random_lines(void)
{
    static const char* const words[] = {"LED",   "STATUS", "BLE", "HELP", "ON", "OFF", "FAST",
                                        "BLINK", "TOGGLE", " ",   "0",    "1",  "2",   "3",
                                        "9",     "x",      "led", "\t",   "S",  "O",   "N"};
    const unsigned word_count = (unsigned)(sizeof(words) / sizeof(words[0]));
    unsigned state = 2024U;

    for (unsigned step = 0U; step < 20000U; step++)
    {
        char line[64];
        const uint8_t led_count = (uint8_t)(next_random(&state) % 5U);
        LED_StateTypeDef states[4];
        OriginalResult_t expected;
        OriginalResult_t actual;
        const unsigned tokens = next_random(&state) % 5U;
        const unsigned ble_status = next_random(&state) % 7U;

        line[0] = '\0';
        if ((next_random(&state) % 3U) != 0U)
        {
            (void)strcat(line, "LED");
        }
        for (unsigned t = 0U; t < tokens; t++)
        {
            (void)strcat(line, words[next_random(&state) % word_count]);
        }
        for (unsigned i = 0U; i < 4U; i++)
        {
            states[i] = (LED_StateTypeDef)(next_random(&state) % 6U);
        }

        original_parse(&expected, line, led_count, states, (int)ble_status);
        new_parse(&actual, line, led_count, states, ble_status);

        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.sent, actual.sent, line);
        TEST_ASSERT_EQUAL_INT_MESSAGE(expected.led_call, actual.led_call, line);
        if (expected.led_call >= 0)
        {
            TEST_ASSERT_EQUAL_INT_MESSAGE(expected.led_action, actual.led_action, line);
        }
    }
}

void test_the_status_reply_matches_the_original_for_every_state_combination(void)
{
    for (uint8_t count = 0U; count <= 4U; count++)
    {
        unsigned combinations = 1U;

        for (uint8_t i = 0U; i < count; i++)
        {
            combinations *= 6U;
        }
        for (unsigned combo = 0U; combo < combinations; combo++)
        {
            LED_StateTypeDef states[4] = {LED_OFF, LED_OFF, LED_OFF, LED_OFF};
            unsigned rest = combo;
            char expected[128];
            char actual[128];

            for (uint8_t i = 0U; i < count; i++)
            {
                states[i] = (LED_StateTypeDef)(rest % 6U);
                rest /= 6U;
            }
            original_get_status(states, count, expected);
            (void)Cmd_FormatStatus(actual, sizeof(actual), states, count);
            TEST_ASSERT_EQUAL_STRING(expected, actual);
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_commands_with_every_action);
    RUN_TEST(test_the_led_number_is_the_character_right_after_led);
    RUN_TEST(test_the_action_is_found_anywhere_in_the_line_in_a_fixed_order);
    RUN_TEST(test_an_led_without_an_action_is_an_invalid_command);
    RUN_TEST(test_an_led_number_that_is_missing_not_a_digit_or_too_big_is_an_invalid_led);
    RUN_TEST(test_status_ble_and_help_are_prefix_matches);
    RUN_TEST(test_a_truncated_command_word_is_unknown);
    RUN_TEST(test_the_led_prefix_wins_over_the_other_commands);
    RUN_TEST(test_unknown_empty_lower_case_and_null_lines);
    RUN_TEST(test_led_replies);
    RUN_TEST(test_status_reply_lists_every_led);
    RUN_TEST(test_status_reply_with_no_leds_is_just_the_header);
    RUN_TEST(test_status_names_toggle_and_unknown_states);
    RUN_TEST(test_ble_status_reply);
    RUN_TEST(test_replies_are_truncated_and_terminated);
    RUN_TEST(test_a_capacity_of_one_stores_only_the_terminator);
    RUN_TEST(test_null_output_or_zero_capacity_stores_nothing);
    RUN_TEST(test_fixed_texts);
    RUN_TEST(test_help_lines);
    RUN_TEST(test_matches_the_original_console_over_random_lines);
    RUN_TEST(test_the_status_reply_matches_the_original_for_every_state_combination);
    return UNITY_END();
}
