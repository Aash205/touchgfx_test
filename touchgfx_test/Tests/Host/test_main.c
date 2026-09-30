#include "ws169_geometry.h"
#include "uart_line.h"
#include <stdio.h>
#include <stdint.h>

static unsigned int s_checks;
static unsigned int s_failures;

#define CHECK_CASE(name, expression) \
    do { \
        s_checks++; \
        if (expression) { \
            (void)printf("PASS: %s\n", name); \
        } else { \
            s_failures++; \
            (void)printf("FAIL: %s\n", name); \
        } \
    } while (0)

static int window_is(WS169_Rotation_t rotation,
                     uint16_t x1,
                     uint16_t y1,
                     uint16_t x2,
                     uint16_t y2,
                     uint16_t expected_x1,
                     uint16_t expected_y1,
                     uint16_t expected_x2,
                     uint16_t expected_y2)
{
    WS169_Window_t actual;
    if (WS169_TranslateWindow(rotation, x1, y1, x2, y2, &actual) != WS169_STATUS_OK)
    {
        return 0;
    }
    return (actual.x_start == expected_x1) && (actual.y_start == expected_y1) &&
           (actual.x_end == expected_x2) && (actual.y_end == expected_y2);
}

static void test_geometry(void)
{
    static const struct
    {
        WS169_Rotation_t rotation;
        uint16_t width;
        uint16_t height;
        uint8_t madctl;
    } cases[] = {
        {WS169_ROTATION_0, 240U, 280U, 0x00U},
        {WS169_ROTATION_90, 280U, 240U, 0x60U},
        {WS169_ROTATION_180, 240U, 280U, 0xC0U},
        {WS169_ROTATION_270, 280U, 240U, 0xA0U}
    };

    for (unsigned int i = 0U; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t madctl = 0U;
        int result = (WS169_GetGeometry(cases[i].rotation, &width, &height, &madctl) ==
                      WS169_STATUS_OK) && (width == cases[i].width) &&
                     (height == cases[i].height) && (madctl == cases[i].madctl);
        char label[48];
        (void)snprintf(label, sizeof(label), "rotation %u geometry", i * 90U);
        CHECK_CASE(label, result);
    }
}

static void test_window_boundaries(void)
{
    CHECK_CASE("rotation 0 full bounds",
               window_is(WS169_ROTATION_0, 0U, 0U, 239U, 279U, 0U, 20U, 239U, 299U));
    CHECK_CASE("rotation 90 full bounds",
               window_is(WS169_ROTATION_90, 0U, 0U, 279U, 239U, 20U, 0U, 299U, 239U));
    CHECK_CASE("rotation 180 full bounds",
               window_is(WS169_ROTATION_180, 0U, 0U, 239U, 279U, 0U, 20U, 239U, 299U));
    CHECK_CASE("rotation 270 full bounds",
               window_is(WS169_ROTATION_270, 0U, 0U, 279U, 239U, 20U, 0U, 299U, 239U));
    CHECK_CASE("single pixel at origin",
               window_is(WS169_ROTATION_90, 0U, 0U, 0U, 0U, 20U, 0U, 20U, 0U));
    CHECK_CASE("single pixel at far corner",
               window_is(WS169_ROTATION_90, 279U, 239U, 279U, 239U,
                         299U, 239U, 299U, 239U));
    CHECK_CASE("one-pixel-wide edge window",
               window_is(WS169_ROTATION_0, 239U, 0U, 239U, 279U,
                         239U, 20U, 239U, 299U));
}

static void test_invalid_arguments(void)
{
    uint16_t width = 0U;
    uint16_t height = 0U;
    uint8_t madctl = 0U;
    WS169_Window_t window;

    CHECK_CASE("rotation count rejected",
               WS169_GetGeometry(WS169_ROTATION_COUNT, &width, &height, &madctl) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("large invalid rotation rejected",
               WS169_GetGeometry((WS169_Rotation_t)255, &width, &height, &madctl) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("null geometry width rejected",
               WS169_GetGeometry(WS169_ROTATION_0, NULL, &height, &madctl) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("null geometry height rejected",
               WS169_GetGeometry(WS169_ROTATION_0, &width, NULL, &madctl) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("null geometry MADCTL rejected",
               WS169_GetGeometry(WS169_ROTATION_0, &width, &height, NULL) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("null translated window rejected",
               WS169_TranslateWindow(WS169_ROTATION_0, 0U, 0U, 0U, 0U, NULL) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("invalid window rotation rejected",
               WS169_TranslateWindow(WS169_ROTATION_COUNT, 0U, 0U, 0U, 0U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("reversed x range rejected",
               WS169_TranslateWindow(WS169_ROTATION_90, 10U, 0U, 9U, 0U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("reversed y range rejected",
               WS169_TranslateWindow(WS169_ROTATION_90, 0U, 10U, 0U, 9U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("x exactly at width rejected",
               WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, 280U, 0U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("y exactly at height rejected",
               WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, 0U, 240U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
    CHECK_CASE("maximum uint16 coordinate rejected",
               WS169_TranslateWindow(WS169_ROTATION_90, 0U, 0U, UINT16_MAX, 0U, &window) ==
               WS169_STATUS_INVALID_ARGUMENT);
}

static void test_uart_line_assembly(void)
{
    char buffer[8] = {0};
    uint8_t index = 0U;
    uint8_t ready = 0U;

    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, '\r');
    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, '\n');
    CHECK_CASE("empty CR/LF lines ignored", (index == 0U) && (ready == 0U));

    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, 'A');
    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, '\r');
    CHECK_CASE("CR terminates non-empty line", (ready == 1U) && (index == 1U) &&
               (buffer[0] == 'A') && (buffer[1] == '\0'));

    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, 'B');
    CHECK_CASE("pending command buffer is retained", (index == 1U) && (buffer[0] == 'A'));

    index = 0U;
    ready = 0U;
    for (uint8_t i = 0U; i < 7U; i++)
    {
        UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, (uint8_t)('0' + i));
    }
    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, 'X');
    CHECK_CASE("line fills buffer without overflowing", (index == 7U) && (ready == 0U) &&
               (buffer[7] == '\0'));
    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, '\n');
    CHECK_CASE("full line can be terminated", (ready == 1U) && (buffer[7] == '\0'));

    index = 8U;
    ready = 0U;
    UART_LineFeedByte(buffer, sizeof(buffer), &index, &ready, '\r');
    CHECK_CASE("invalid index rejected without buffer access", (index == 8U) && (ready == 0U));

    index = 0U;
    ready = 0U;
    UART_LineFeedByte(buffer, 0U, &index, &ready, 'Q');
    CHECK_CASE("zero buffer capacity ignored", (index == 0U) && (ready == 0U));
}

int main(void)
{
    test_geometry();
    test_window_boundaries();
    test_invalid_arguments();
    test_uart_line_assembly();
    (void)printf("\nHost firmware tests: %u/%u passed, %u failed\n",
                 s_checks - s_failures, s_checks, s_failures);
    return (s_failures == 0U) ? 0 : 1;
}
