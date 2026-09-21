#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <string.h>

Model::Model() : modelListener(0), tickCount(0), haveLast(false)
{
    memset(&last, 0, sizeof(last));
}

void Model::tick()
{
    if (++tickCount < POLL_TICKS)
    {
        return;
    }
    tickCount = 0;

    AppState now;
    AppState_Get(&now);

    if (!haveLast || memcmp(&now, &last, sizeof(now)) != 0)
    {
        last = now;
        haveLast = true;
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
