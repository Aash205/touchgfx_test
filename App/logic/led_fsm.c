#include "led_fsm.h"

#include <stddef.h>

#define LED_DEFAULT_HALF_PERIOD_MS 100U

void Led_Init(Led_t* led)
{
    if (led != NULL)
    {
        led->state = LED_OFF;
        led->last_toggle_ms = 0U;
        led->half_period_ms = LED_DEFAULT_HALF_PERIOD_MS;
    }
}

LedPinAction_t Led_Set(Led_t* led, LED_StateTypeDef state, uint32_t now_ms, bool pin_high)
{
    LedPinAction_t action = LED_PIN_HOLD;

    if (led != NULL)
    {
        led->state = state;

        if (state == LED_ON)
        {
            action = LED_PIN_HIGH;
        }
        else if (state == LED_OFF)
        {
            action = LED_PIN_LOW;
        }
        else if (state == LED_TOGGLE)
        {
            action = LED_PIN_TOGGLE;
            led->state = pin_high ? LED_OFF : LED_ON;
        }
        else if (state == LED_BLINK_SLOW)
        {
            led->half_period_ms = LED_BLINK_SLOW_HALF_PERIOD_MS;
            led->last_toggle_ms = now_ms;
        }
        else if (state == LED_BLINK_FAST)
        {
            led->half_period_ms = LED_BLINK_FAST_HALF_PERIOD_MS;
            led->last_toggle_ms = now_ms;
        }
        else
        {
            /* A value outside the enum is kept as is. */
        }
    }

    return action;
}

LedPinAction_t Led_Update(Led_t* led, uint32_t now_ms)
{
    LedPinAction_t action = LED_PIN_HOLD;

    if (led != NULL)
    {
        if (led->state == LED_ON)
        {
            action = LED_PIN_HIGH;
        }
        else if (led->state == LED_OFF)
        {
            action = LED_PIN_LOW;
        }
        else if (led->state == LED_TOGGLE)
        {
            action = LED_PIN_TOGGLE;
            led->state = LED_OFF;
        }
        else if ((led->state == LED_BLINK_SLOW) || (led->state == LED_BLINK_FAST))
        {
            if ((uint32_t)(now_ms - led->last_toggle_ms) >= led->half_period_ms)
            {
                action = LED_PIN_TOGGLE;
                led->last_toggle_ms = now_ms;
            }
        }
        else
        {
            /* A value outside the enum does nothing. */
        }
    }

    return action;
}
