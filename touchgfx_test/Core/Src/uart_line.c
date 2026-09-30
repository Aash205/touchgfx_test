#include "uart_line.h"

void UART_LineFeedByte(char *buffer,
                       uint8_t capacity,
                       uint8_t *index,
                       uint8_t *command_ready,
                       uint8_t data)
{
    if ((buffer == 0) || (index == 0) || (command_ready == 0) || (capacity == 0U))
    {
        return;
    }
    if (*index >= capacity)
    {
        return;
    }

    /* A pending command owns the buffer until the consumer releases it. */
    if (*command_ready != 0U)
    {
        return;
    }

    if ((data == '\r') || (data == '\n'))
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
}
