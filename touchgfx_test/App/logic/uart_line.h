#ifndef UART_LINE_H
#define UART_LINE_H

#include <stdint.h>

void UART_LineFeedByte(char* buffer, uint8_t capacity, uint8_t* index, uint8_t* command_ready,
                       uint8_t data);

#endif
