#ifndef MODEL_HPP
#define MODEL_HPP

#include "app_state.h"
#include "change_detect.h"
#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    /** Toggle an LED (0 = LD1, 1 = LD3) through the shared application state. */
    void toggleLed(uint8_t idx);

protected:
    ModelListener* modelListener;

private:
    static const uint8_t POLL_TICKS = 10;   // ~200 ms at 50 Hz
    ChangeDetect_t detect;
    AppState last;
};

#endif // MODEL_HPP
