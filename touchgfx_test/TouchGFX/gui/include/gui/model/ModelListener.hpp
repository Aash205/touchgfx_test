#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include "app_state.h"

class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    /** Called when the shared application state (LEDs, BLE, uptime, FPS ...) changed. */
    virtual void stateChanged(const AppState& /*state*/) {}

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
