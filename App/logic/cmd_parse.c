#include "cmd_parse.h"

#include "text_writer.h"

#include <string.h>

#define HELP_LINE_COUNT 5U

/* The LED number for a digit character: true when c is '0'..'9' and below led_count. */
static bool led_number(char c, uint8_t led_count, uint8_t* index)
{
    static const char digits[] = "0123456789";
    bool found = false;

    for (uint8_t i = 0U; (i < led_count) && (i < 10U) && !found; i++)
    {
        if (c == digits[i])
        {
            *index = i;
            found = true;
        }
    }

    return found;
}

static bool is_action(const char* line, const char* word)
{
    return strstr(line, word) != NULL;
}

Cmd_t Cmd_Parse(const char* line, uint8_t led_count)
{
    Cmd_t cmd;

    cmd.kind = CMD_UNKNOWN;
    cmd.led_index = 0U;
    cmd.action = LED_OFF;

    if (line == NULL)
    {
        /* nothing to parse */
    }
    else if (strncmp(line, "LED", 3U) == 0)
    {
        uint8_t index = 0U;

        if (!led_number(line[3], led_count, &index))
        {
            cmd.kind = CMD_INVALID_LED;
        }
        else
        {
            cmd.kind = CMD_LED;
            cmd.led_index = index;

            if (is_action(line, "ON"))
            {
                cmd.action = LED_ON;
            }
            else if (is_action(line, "OFF"))
            {
                cmd.action = LED_OFF;
            }
            else if (is_action(line, "FAST"))
            {
                cmd.action = LED_BLINK_FAST;
            }
            else if (is_action(line, "BLINK"))
            {
                cmd.action = LED_BLINK_SLOW;
            }
            else if (is_action(line, "TOGGLE"))
            {
                cmd.action = LED_TOGGLE;
            }
            else
            {
                cmd.kind = CMD_INVALID_ACTION;
            }
        }
    }
    else if (strncmp(line, "STATUS", 6U) == 0)
    {
        cmd.kind = CMD_STATUS;
    }
    else if (strncmp(line, "BLE", 3U) == 0)
    {
        cmd.kind = CMD_BLE;
    }
    else if (strncmp(line, "HELP", 4U) == 0)
    {
        cmd.kind = CMD_HELP;
    }
    else
    {
        /* CMD_UNKNOWN */
    }

    return cmd;
}

size_t Cmd_FormatLedReply(char* out, size_t capacity, uint8_t led_index, LED_StateTypeDef action)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        const char* text = "UNKNOWN";

        switch (action)
        {
        case LED_ON:
            text = "ON";
            break;
        case LED_OFF:
            text = "OFF";
            break;
        case LED_BLINK_FAST:
            text = "BLINK FAST";
            break;
        case LED_BLINK_SLOW:
            text = "BLINK";
            break;
        case LED_TOGGLE:
            text = "TOGGLE";
            break;
        default:
            break;
        }

        TextWriter_PutText(&writer, "LED");
        TextWriter_PutNumber(&writer, led_index, 1U);
        TextWriter_PutText(&writer, ": ");
        TextWriter_PutText(&writer, text);
        TextWriter_PutText(&writer, "\r\n");
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

size_t Cmd_FormatStatus(char* out, size_t capacity, const LED_StateTypeDef* states, uint8_t count)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "=== Status ===\r\n");

        for (uint8_t i = 0U; (states != NULL) && (i < count); i++)
        {
            const char* text = "UNKNOWN";

            switch (states[i])
            {
            case LED_ON:
                text = "ON";
                break;
            case LED_OFF:
                text = "OFF";
                break;
            case LED_TOGGLE:
                text = "TOGGLE";
                break;
            case LED_BLINK_SLOW:
                text = "BLINK_SLOW";
                break;
            case LED_BLINK_FAST:
                text = "BLINK_FAST";
                break;
            default:
                break;
            }

            TextWriter_PutText(&writer, "LED");
            TextWriter_PutNumber(&writer, i, 1U);
            TextWriter_PutText(&writer, ": ");
            TextWriter_PutText(&writer, text);
            TextWriter_PutText(&writer, "\r\n");
        }
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

size_t Cmd_FormatBleStatus(char* out, size_t capacity, unsigned status)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "BLE status: ");
        TextWriter_PutNumber(&writer, status, 1U);
        TextWriter_PutText(&writer, "\r\n");
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

const char* Cmd_ErrorText(CmdKind_t kind)
{
    const char* text = NULL;

    if (kind == CMD_INVALID_LED)
    {
        text = "ERROR: Invalid LED\r\n";
    }
    else if (kind == CMD_INVALID_ACTION)
    {
        text = "ERROR: Invalid command\r\n";
    }
    else if (kind == CMD_UNKNOWN)
    {
        text = "ERROR: Unknown command\r\n";
    }
    else
    {
        /* no error text */
    }

    return text;
}

const char* Cmd_HelpLine(unsigned index)
{
    static const char* const lines[HELP_LINE_COUNT] = {
        "Commands:\r\n",        "  LED<N> ON/OFF/TOGGLE/BLINK/FAST\r\n",
        "  BLE   (status)\r\n", "  STATUS\r\n",
        "  HELP\r\n",
    };
    const char* text = NULL;

    if (index < HELP_LINE_COUNT)
    {
        text = lines[index];
    }

    return text;
}
