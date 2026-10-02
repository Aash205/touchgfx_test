#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <string.h>

Model::Model() : modelListener(0)
{
    memset(&detect, 0, sizeof(detect));
    memset(&last, 0, sizeof(last));
}

// Field by field on purpose: comparing the raw bytes would include struct padding, whose value C
// leaves unspecified, and an unchanged state could then look changed.
static bool sameState(const AppState& a, const AppState& b)
{
    for (uint8_t i = 0; i < APP_LED_COUNT; i++)
    {
        if (a.led[i] != b.led[i])
        {
            return false;
        }
    }
    return (a.ble_status == b.ble_status) && (a.uptime_s == b.uptime_s) &&
           (a.heartbeat == b.heartbeat) && (a.fps == b.fps);
}

void Model::tick()
{
    if (!ChangeDetect_PollDue(&detect, POLL_TICKS))
    {
        return;
    }

    AppState now;
    AppState_Get(&now);

    if (ChangeDetect_Changed(&detect, !sameState(now, last)))
    {
        last = now;
        if (modelListener)
        {
            modelListener->stateChanged(now);
        }
    }
}

void Model::toggleLed(uint8_t idx)
{
    AppState_ToggleLed(idx);
}
