#include "log_format.h"

#include <stddef.h>

/* Bounded writer: stores at most `limit` characters but keeps counting every offered one. */
typedef struct
{
    char* out;
    size_t limit;
    size_t length;
} LineWriter_t;

static void put_char(LineWriter_t* writer, char c)
{
    if (writer->length < writer->limit)
    {
        writer->out[writer->length] = c;
    }
    writer->length++;
}

static void put_text(LineWriter_t* writer, const char* text)
{
    size_t i = 0U;

    while (text[i] != '\0')
    {
        put_char(writer, text[i]);
        i++;
    }
}

/* Decimal, zero-padded on the left to at least min_digits digits. */
static void put_number(LineWriter_t* writer, uint32_t value, size_t min_digits)
{
    char digits[10];
    size_t count = 0U;
    uint32_t rest = value;

    do
    {
        digits[count] = (char)('0' + (rest % 10U));
        count++;
        rest /= 10U;
    } while (rest != 0U);

    for (size_t pad = count; pad < min_digits; pad++)
    {
        put_char(writer, '0');
    }
    while (count > 0U)
    {
        count--;
        put_char(writer, digits[count]);
    }
}

const char* LogFormat_LevelTag(LogLevelTypeDef level)
{
    const char* tag = "[?]";

    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        tag = "[DEBUG]";
        break;
    case LOG_LEVEL_INFO:
        tag = "[INFO]";
        break;
    case LOG_LEVEL_WARNING:
        tag = "[WARN]";
        break;
    case LOG_LEVEL_ERROR:
        tag = "[ERROR]";
        break;
    case LOG_LEVEL_CRITICAL:
        tag = "[CRIT]";
        break;
    default:
        break;
    }

    return tag;
}

bool LogFormat_ShouldLog(LogLevelTypeDef level, LogLevelTypeDef minimum)
{
    return level >= minimum;
}

size_t LogFormat_Line(char* out, size_t capacity, LogLevelTypeDef level, uint32_t ms,
                      const char* message)
{
    size_t stored = 0U;

    if ((out != NULL) && (capacity != 0U))
    {
        LineWriter_t writer;

        writer.out = out;
        writer.limit = capacity - 1U;
        writer.length = 0U;

        put_text(&writer, LogFormat_LevelTag(level));
        put_text(&writer, " [");
        put_number(&writer, ms / 1000U, 4U);
        put_char(&writer, '.');
        put_number(&writer, ms % 1000U, 3U);
        put_text(&writer, "] ");
        if (message != NULL)
        {
            put_text(&writer, message);
        }
        put_text(&writer, "\r\n");

        stored = (writer.length < writer.limit) ? writer.length : writer.limit;
        out[stored] = '\0';
    }

    return stored;
}
