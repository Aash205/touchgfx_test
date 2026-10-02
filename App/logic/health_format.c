#include "health_format.h"

#include "text_writer.h"

size_t HealthFormat_Line(char* out, size_t capacity, bool usbx_active, uint32_t fps,
                         uint32_t ble_status, uint32_t display_status, uint32_t display_faults,
                         uint32_t heartbeat)
{
    TextWriter_t writer;
    size_t stored = 0U;

    if (TextWriter_Begin(&writer, out, capacity))
    {
        TextWriter_PutText(&writer, "HEALTH ThreadX=OK USBX=");
        TextWriter_PutText(&writer, usbx_active ? "ACTIVE" : "WAIT");
        TextWriter_PutText(&writer, " TouchGFX_FPS=");
        TextWriter_PutNumber(&writer, fps, 1U);
        TextWriter_PutText(&writer, " BLE=");
        TextWriter_PutNumber(&writer, ble_status, 1U);
        TextWriter_PutText(&writer, " Display=");
        TextWriter_PutNumber(&writer, display_status, 1U);
        TextWriter_PutText(&writer, " DisplayFaults=");
        TextWriter_PutNumber(&writer, display_faults, 1U);
        TextWriter_PutText(&writer, " Heartbeat=");
        TextWriter_PutNumber(&writer, heartbeat, 1U);
        stored = TextWriter_Finish(&writer);
    }

    return stored;
}
