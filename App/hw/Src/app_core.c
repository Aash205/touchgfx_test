/**
 ******************************************************************************
 * @file    app_core.c
 * @brief   LED / console ownership and the shared AppState (see app_core.h).
 ******************************************************************************
 */

#include "app_core.h"
#include "ble_app.h"
#include "counters.h"
#include "debounce.h"
#include "tx_api.h"
#include "uart_commands.h"
#include "usb_logging.h"

#define USER_BTN_PIN GPIO_PIN_13 /* Nucleo B1 (blue) on port C, active high */
#define BTN_DEBOUNCE_POLLS 2U

/* cppcheck-suppress misra-c2012-11.4 -- GPIOC is the HAL's fixed peripheral address */
static GPIO_TypeDef* const s_gpio_c = GPIOC;

static UART_CommandTypeDef s_console;
static Counters_t s_counters; /* frame and heartbeat counters (App/logic/counters) */

void AppCore_Init(UART_HandleTypeDef* console_uart)
{
    GPIO_InitTypeDef gpio = {0};
    HAL_StatusTypeDef uart_status;
    /* cppcheck-suppress misra-c2012-11.4 -- GPIOB is the HAL's fixed peripheral address */
    GPIO_TypeDef* const gpio_b = GPIOB;

    (void)UART_CMD_Init(&s_console, console_uart);
    (void)UART_CMD_RegisterLED(&s_console, s_gpio_c, GPIO_PIN_7); /* LD1 */
    (void)UART_CMD_RegisterLED(&s_console, gpio_b, GPIO_PIN_14);  /* LD3 */

    uart_status = UART_CMD_StartListening(&s_console);
    if (uart_status == HAL_OK)
    {
        (void)USB_Logging_Printf(LOG_LEVEL_INFO, "LPUART1 RX ready on PG8 at 115200 8N1");
    }
    else
    {
        (void)USB_Logging_Printf(LOG_LEVEL_ERROR, "LPUART1 RX arm failed: %d", (int)uart_status);
    }

    /* cppcheck-suppress misra-c2012-11.4 -- HAL clock-enable macro reads the RCC register address
     */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = USER_BTN_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    /* cppcheck-suppress misra-c2012-7.3 -- the HAL's GPIO_NOPULL constant */
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_gpio_c, &gpio);
}

static void poll_button(void)
{
    static Debounce_t debounce;

    const bool pressed = (HAL_GPIO_ReadPin(s_gpio_c, USER_BTN_PIN) == GPIO_PIN_SET);
    if (Debounce_Poll(&debounce, pressed, BTN_DEBOUNCE_POLLS))
    {
        AppState_ToggleLed(0); /* B1 press toggles LD1 */
    }
}

void AppCore_Process(void)
{
    (void)UART_CMD_Process(&s_console);
    UART_CMD_UpdateLEDs(&s_console);
    poll_button();
}

/* UART RX interrupt: one byte at a time into the command assembler. */
/* cppcheck-suppress constParameterPointer -- must match the HAL's weak callback prototype */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == s_console.huart)
    {
        UART_CMD_ReceiveCallback(&s_console, s_console.rx_byte);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart == s_console.huart)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        (void)UART_CMD_StartListening(&s_console);
    }
}

void AppCore_Tick1s(void)
{
    Counters_Tick1s(&s_counters);
}

uint32_t AppCore_Heartbeat(void)
{
    return Counters_Heartbeat(&s_counters);
}

/* ---- AppState (app_state.h) --------------------------------------------- */

void AppState_Get(AppState* out)
{
    for (uint8_t i = 0; i < APP_LED_COUNT; i++)
    {
        out->led[i] = (i < s_console.led_count) && (s_console.leds[i].fsm.state != LED_OFF);
    }
    out->ble_status = (uint8_t)BLE_App_GetStatus();
    /* Use the RTOS clock for application uptime. It starts when ThreadX starts and
       is independent of the HAL peripheral time base used by driver timeouts. */
    out->uptime_s = (uint32_t)(tx_time_get() / TX_TIMER_TICKS_PER_SECOND);
    out->heartbeat = Counters_HeartbeatValue(&s_counters);
    out->fps = Counters_Fps(&s_counters);
}

void AppState_SetLed(uint8_t idx, uint8_t on)
{
    if (idx < APP_LED_COUNT)
    {
        UART_CMD_SetLEDState(&s_console, idx, on ? LED_ON : LED_OFF);
    }
}

void AppState_ToggleLed(uint8_t idx)
{
    if (idx < s_console.led_count)
    {
        AppState_SetLed(idx, s_console.leds[idx].fsm.state == LED_OFF);
    }
}

void AppState_FrameFlushed(void)
{
    Counters_FrameFlushed(&s_counters);
}
