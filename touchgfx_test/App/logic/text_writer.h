#ifndef TEXT_WRITER_H
#define TEXT_WRITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    char* out;     /* destination buffer */
    size_t limit;  /* characters that fit, without the terminator */
    size_t length; /* characters offered so far, stored or not */
} TextWriter_t;

/* clang-format off */
/*
 * Bounded text builder shared by the formatters (log_format, cmd_parse, ui_format). No stdio.
 * It stores at most capacity - 1 characters, keeps counting the ones that did not fit, and
 * always leaves a NUL-terminated string behind.
 *
 * TextWriter_Begin starts a writer over out and returns false (writing nothing, so the caller
 * stores nothing) when out is NULL or capacity is 0. TextWriter_PutChar and TextWriter_PutText
 * append; a NULL text is ignored. TextWriter_PutNumber appends value in decimal, padded on the
 * left with zeros to at least min_digits digits (0 or 1 means no padding). TextWriter_Finish
 * terminates the string and returns the number of characters stored, without the NUL.
 */
bool TextWriter_Begin(TextWriter_t* writer, char* out, size_t capacity);
void TextWriter_PutChar(TextWriter_t* writer, char c);
void TextWriter_PutText(TextWriter_t* writer, const char* text);
void TextWriter_PutNumber(TextWriter_t* writer, uint32_t value, size_t min_digits);
size_t TextWriter_Finish(const TextWriter_t* writer);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* TEXT_WRITER_H */
