#ifndef LED_FSM_H
#define LED_FSM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    LED_OFF,
    LED_ON,
    LED_TOGGLE,
    LED_BLINK_SLOW,
    LED_BLINK_FAST
} LED_StateTypeDef;

/* What the caller must do to the LED pin after a state change or a periodic update. */
typedef enum
{
    LED_PIN_HOLD,
    LED_PIN_LOW,
    LED_PIN_HIGH,
    LED_PIN_TOGGLE
} LedPinAction_t;

#define LED_BLINK_SLOW_HALF_PERIOD_MS 500U
#define LED_BLINK_FAST_HALF_PERIOD_MS 100U

typedef struct
{
    LED_StateTypeDef state;
    uint32_t last_toggle_ms; /* tick of the last blink toggle (or of entering a blink state) */
    uint32_t half_period_ms; /* blink toggles when this many ms have passed */
} Led_t;

/* clang-format off */
/*
 * State machine of one LED. Pure: it never touches a pin. Every call returns the pin action the
 * caller applies (write low, write high, toggle, or nothing), so the pin level is the only state
 * outside this struct.
 */

/* State OFF, half period 100 ms, last toggle 0. A NULL led is ignored. */
void Led_Init(Led_t* led);

/*
 * Enter a state. ON and OFF drive the pin; TOGGLE toggles it and then settles on ON or OFF
 * according to the level the pin had before the toggle (pin_high); BLINK_SLOW / BLINK_FAST set
 * the half period (500 / 100 ms), restart the blink timer at now_ms and leave the pin alone.
 * A value outside the enum is stored as is and leaves the pin alone. A NULL led gives HOLD.
 */
LedPinAction_t Led_Set(Led_t* led, LED_StateTypeDef state, uint32_t now_ms, bool pin_high);

/*
 * Periodic update. ON and OFF re-assert the pin level on every call; TOGGLE toggles once and
 * becomes OFF; the blink states toggle when now_ms - last_toggle_ms (wrap-safe) has reached the
 * half period and then restart the timer at now_ms. A NULL led gives HOLD.
 */
LedPinAction_t Led_Update(Led_t* led, uint32_t now_ms);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* LED_FSM_H */
