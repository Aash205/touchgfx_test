#include "text_writer.h"

#define UINT32_MAX_DIGITS 10U

bool TextWriter_Begin(TextWriter_t* writer, char* out, size_t capacity)
{
    bool usable = false;

    if ((writer != NULL) && (out != NULL) && (capacity != 0U))
    {
        writer->out = out;
        writer->limit = capacity - 1U;
        writer->length = 0U;
        usable = true;
    }

    return usable;
}

void TextWriter_PutChar(TextWriter_t* writer, char c)
{
    if (writer->length < writer->limit)
    {
        writer->out[writer->length] = c;
    }
    writer->length++;
}

void TextWriter_PutText(TextWriter_t* writer, const char* text)
{
    if (text != NULL)
    {
        size_t i = 0U;

        while (text[i] != '\0')
        {
            TextWriter_PutChar(writer, text[i]);
            i++;
        }
    }
}

void TextWriter_PutNumber(TextWriter_t* writer, uint32_t value, size_t min_digits)
{
    char digits[UINT32_MAX_DIGITS];
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
        TextWriter_PutChar(writer, '0');
    }
    while (count > 0U)
    {
        count--;
        TextWriter_PutChar(writer, digits[count]);
    }
}

size_t TextWriter_Finish(const TextWriter_t* writer)
{
    const size_t stored = (writer->length < writer->limit) ? writer->length : writer->limit;

    writer->out[stored] = '\0';

    return stored;
}
