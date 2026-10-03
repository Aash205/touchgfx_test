#include "ui_format.h"

#include "text_writer.h"

#define BLE_NAME_COUNT 7U
#define SECONDS_PER_HOUR 3600U
#define SECONDS_PER_MINUTE 60U

size_t UiFormat_BleLine(char* out, size_t capacity, uint8_t ble_status)
{
    static const char* const names[BLE_NAME_COUNT] = {
        "Idle", "Init", "Ready", "Advertising", "Connected", "Paired", "Error",
    };
    const uint8_t index = (ble_status < BLE_NAME_COUNT) ? ble_status : (BLE_NAME_COUNT - 1U);
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "BLE: ");
        TextWriter_PutText(&writer, names[index]);
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

size_t UiFormat_UptimeLine(char* out, size_t capacity, uint32_t uptime_s)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "Uptime: ");
        TextWriter_PutNumber(&writer, uptime_s / SECONDS_PER_HOUR, 2U);
        TextWriter_PutChar(&writer, ':');
        TextWriter_PutNumber(&writer, (uptime_s / SECONDS_PER_MINUTE) % SECONDS_PER_MINUTE, 2U);
        TextWriter_PutChar(&writer, ':');
        TextWriter_PutNumber(&writer, uptime_s % SECONDS_PER_MINUTE, 2U);
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

size_t UiFormat_HeartbeatLine(char* out, size_t capacity, uint32_t heartbeat)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "Heartbeat: ");
        TextWriter_PutNumber(&writer, heartbeat, 1U);
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

size_t UiFormat_FpsLine(char* out, size_t capacity, uint16_t fps)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "FPS: ");
        TextWriter_PutNumber(&writer, fps, 1U);
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}

const char* UiFormat_LedLabel(uint8_t index, bool on)
{
    static const char* const on_labels[UI_FORMAT_LED_COUNT] = {"LD1 ON", "LD3 ON"};
    static const char* const off_labels[UI_FORMAT_LED_COUNT] = {"LD1 OFF", "LD3 OFF"};
    const char* label = "";

    if (index < UI_FORMAT_LED_COUNT)
    {
        label = on ? on_labels[index] : off_labels[index];
    }

    return label;
}
