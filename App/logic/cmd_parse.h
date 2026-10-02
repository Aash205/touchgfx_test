#ifndef CMD_PARSE_H
#define CMD_PARSE_H

#include "led_fsm.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    CMD_LED, /* LED<N> <action>: led_index and action are set */
    CMD_STATUS,
    CMD_BLE,
    CMD_HELP,
    CMD_UNKNOWN,       /* not one of the commands, or no line */
    CMD_INVALID_LED,   /* LED prefix, but the digit is missing, not a digit, or >= led_count */
    CMD_INVALID_ACTION /* LED<N> without a recognised action */
} CmdKind_t;

typedef struct
{
    CmdKind_t kind;
    uint8_t led_index;       /* valid for CMD_LED */
    LED_StateTypeDef action; /* valid for CMD_LED */
} Cmd_t;

/* clang-format off */
/*
 * Console command parser and reply formatter for the LPUART1 console. Pure functions: sending the
 * reply and driving the LED stay in the firmware. Matching is case-sensitive.
 *
 * Cmd_Parse follows the original console exactly:
 *  - The command word is a prefix match, tested in the order LED, STATUS, BLE, HELP, so "LEDS",
 *    "STATUSX" and "BLEFOO" are an LED command, STATUS and BLE.
 *  - For LED, the character right after "LED" is the one-digit LED number and must be below
 *    led_count. The action is then found anywhere in the line, not only after the number, in
 *    this order: "ON", "OFF", "FAST", "BLINK", "TOGGLE" (so "LED0 BLINK ONCE" is ON).
 *  - A NULL line is CMD_UNKNOWN.
 */
Cmd_t Cmd_Parse(const char* line, uint8_t led_count);

/*
 * Reply formatters. Each stores a NUL-terminated string truncated to capacity - 1 characters and
 * returns the number of characters stored (0 when out is NULL or capacity is 0).
 *
 * Cmd_FormatLedReply: "LED<n>: ON|OFF|BLINK|BLINK FAST|TOGGLE\r\n" ("BLINK" is the slow blink,
 * a value outside the enum prints "UNKNOWN").
 * Cmd_FormatStatus: "=== Status ===\r\n" then one "LED<n>: <state>\r\n" per LED, where the state
 * is ON, OFF, TOGGLE, BLINK_SLOW, BLINK_FAST or UNKNOWN. The firmware has at most 4 LEDs.
 * Cmd_FormatBleStatus: "BLE status: <n>\r\n".
 */
size_t Cmd_FormatLedReply(char* out, size_t capacity, uint8_t led_index, LED_StateTypeDef action);
size_t Cmd_FormatStatus(char* out, size_t capacity, const LED_StateTypeDef* states, uint8_t count);
size_t Cmd_FormatBleStatus(char* out, size_t capacity, unsigned status);

/*
 * Fixed texts. Cmd_ErrorText: the reply for CMD_INVALID_LED ("ERROR: Invalid LED\r\n"),
 * CMD_INVALID_ACTION ("ERROR: Invalid command\r\n") and CMD_UNKNOWN ("ERROR: Unknown command\r\n");
 * NULL for every other kind. Cmd_HelpLine: line 0 to 4 of the HELP reply, each ending in "\r\n",
 * NULL from index 5 on; the firmware sends them one after the other.
 */
const char* Cmd_ErrorText(CmdKind_t kind);
const char* Cmd_HelpLine(unsigned index);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* CMD_PARSE_H */
