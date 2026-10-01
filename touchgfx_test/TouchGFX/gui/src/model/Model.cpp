#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <string.h>

Model::Model() : modelListener(0)
{
    memset(&detect, 0, sizeof(detect));
    memset(&last, 0, sizeof(last));
}

void Model::tick()
{
    if (!ChangeDetect_PollDue(&detect, POLL_TICKS))
    {
        return;
    }

    // Zero first: the snapshot is compared byte by byte, padding included, and AppState_Get
    // fills the fields only, so stale stack bytes in the padding could look like a change.
    AppState now;
    memset(&now, 0, sizeof(now));
    AppState_Get(&now);

    if (ChangeDetect_Update(&detect, reinterpret_cast<uint8_t*>(&last),
                            reinterpret_cast<const uint8_t*>(&now), sizeof(now)))
    {
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
