#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/common/DynText.hpp>
#include <gui/common/SoftButton.hpp>
#include "app_state.h"

/**
 * Live status dashboard. Widgets are created in code (not in Designer) so that
 * regenerating Screen1ViewBase does not remove them.
 */
class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}

    virtual void setupScreen();
    virtual void tearDownScreen();

    void updateState(const AppState& state);

protected:
    DynText title;
    DynText bleText;
    DynText uptimeText;
    DynText heartbeatText;
    DynText fpsText;
    DynText testText;
    SoftButton ledButton[APP_LED_COUNT];

    touchgfx::Callback<Screen1View, SoftButton&> ledClickedCallback;
    void ledClickedHandler(SoftButton& button);
};

#endif // SCREEN1VIEW_HPP
