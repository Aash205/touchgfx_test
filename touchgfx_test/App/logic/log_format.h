#ifndef LOG_FORMAT_H
#define LOG_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL
} LogLevelTypeDef;

/* "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[CRIT]", or "[?]" for a value outside the enum. */
const char* LogFormat_LevelTag(LogLevelTypeDef level);

/* True when a message of this level passes a filter set to the minimum level. */
bool LogFormat_ShouldLog(LogLevelTypeDef level, LogLevelTypeDef minimum);

/* clang-format off */
/*
 * Build one log line, "<tag> [ssss.mmm] <message>\r\n", from a millisecond timestamp.
 * The tag is "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]" or "[CRIT]", or "[?]" for a level
 * outside the enum. The output is always NUL-terminated and truncated to capacity - 1
 * characters. Returns the number of characters stored (without the NUL): 0 when out is NULL
 * or capacity is 0. A NULL message is an empty message.
 */
/* clang-format on */
size_t LogFormat_Line(char* out, size_t capacity, LogLevelTypeDef level, uint32_t ms,
                      const char* message);

#endif /* LOG_FORMAT_H */
