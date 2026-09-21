#ifndef SOFTBUTTON_HPP
#define SOFTBUTTON_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/Callback.hpp>
#include <gui/common/DynText.hpp>

/**
 * Header-only button: coloured box + centred label. Also used as a state indicator
 * (setBaseColor). Fires `clicked` on release inside the button.
 */
class SoftButton : public touchgfx::Container
{
public:
    SoftButton()
        : pressed(false),
          baseColor(touchgfx::Color::getColorFromRGB(60, 60, 60)),
          pressedColor(touchgfx::Color::getColorFromRGB(255, 255, 255)),
          clicked(0)
    {
        setTouchable(true);
        add(bg);
        add(label);
        label.setAlignment(touchgfx::CENTER);
    }

    void setBoundsAndLayout(int16_t x, int16_t y, int16_t w, int16_t h, touchgfx::FontId font)
    {
        setPosition(x, y, w, h);
        bg.setPosition(0, 0, w, h);
        label.setFont(font);
        // vertically centre the label (font height is not known here; ~1/4 of h works for 20px fonts)
        label.setPosition(0, (h - 24) / 2, w, 26);
        bg.setColor(baseColor);
    }

    void setLabel(const char* text)
    {
        label.setText(text);
    }

    void setLabelColor(touchgfx::colortype c)
    {
        label.setColor(c);
    }

    void setBaseColor(touchgfx::colortype c)
    {
        baseColor = c;
        if (!pressed)
        {
            bg.setColor(baseColor);
            bg.invalidate();
        }
    }

    void setClickedCallback(touchgfx::GenericCallback<SoftButton&>& cb)
    {
        clicked = &cb;
    }

    virtual void handleClickEvent(const touchgfx::ClickEvent& event)
    {
        if (event.getType() == touchgfx::ClickEvent::PRESSED)
        {
            pressed = true;
            bg.setColor(pressedColor);
            bg.invalidate();
        }
        else if (event.getType() == touchgfx::ClickEvent::RELEASED)
        {
            bool wasPressed = pressed;
            pressed = false;
            bg.setColor(baseColor);
            bg.invalidate();
            if (wasPressed && clicked != 0 && clicked->isValid())
            {
                clicked->execute(*this);
            }
        }
    }

private:
    touchgfx::Box bg;
    DynText label;
    bool pressed;
    touchgfx::colortype baseColor;
    touchgfx::colortype pressedColor;
    touchgfx::GenericCallback<SoftButton&>* clicked;
};

#endif // SOFTBUTTON_HPP
