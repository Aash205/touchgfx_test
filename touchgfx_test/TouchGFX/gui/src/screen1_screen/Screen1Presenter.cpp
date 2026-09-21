#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{
}

void Screen1Presenter::activate()
{
    AppState s;
    AppState_Get(&s);
    view.updateState(s);
}

void Screen1Presenter::deactivate()
{
}

void Screen1Presenter::stateChanged(const AppState& state)
{
    view.updateState(state);
}

void Screen1Presenter::ledToggle(uint8_t idx)
{
    model->toggleLed(idx);
}
