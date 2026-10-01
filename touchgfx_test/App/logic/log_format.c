#include "log_format.h"

#include "text_writer.h"

#include <stddef.h>

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
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, LogFormat_LevelTag(level));
        TextWriter_PutText(&writer, " [");
        TextWriter_PutNumber(&writer, ms / 1000U, 4U);
        TextWriter_PutChar(&writer, '.');
        TextWriter_PutNumber(&writer, ms % 1000U, 3U);
        TextWriter_PutText(&writer, "] ");
        TextWriter_PutText(&writer, message);
        TextWriter_PutText(&writer, "\r\n");
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}
