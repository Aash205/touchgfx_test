#include "uart_line.h"

#include <stdbool.h>
#include <stddef.h>

#define LINE_CR 13U
#define LINE_LF 10U

void UART_LineFeedByte(char* buffer, uint8_t capacity, uint8_t* index, uint8_t* command_ready,
                       uint8_t data)
{
    const bool usable =
        (buffer != NULL) && (index != NULL) && (command_ready != NULL) && (capacity != 0U);

    /* A pending command owns the buffer until the consumer releases it. */
    if (usable && (*index < capacity) && (*command_ready == 0U))
    {
        if ((data == LINE_CR) || (data == LINE_LF))
        {
            if (*index > 0U)
            {
                buffer[*index] = '\0';
                *command_ready = 1U;
            }
        }
        else if (*index < (uint8_t)(capacity - 1U))
        {
            buffer[*index] = (char)data;
            (*index)++;
        }
        else
        {
            /* The buffer is full: the character is dropped. */
        }
    }
}
