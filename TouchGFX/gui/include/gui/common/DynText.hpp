#ifndef DYNTEXT_HPP
#define DYNTEXT_HPP

#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/FontManager.hpp>
#include <touchgfx/Color.hpp>
#include <fonts/ApplicationFontProvider.hpp>

/**
 * Header-only text widget for run-time strings. Draws straight from a FontManager font,
 * so it needs no typed-text entry (i.e. nothing that TouchGFX Designer regeneration removes).
 */
class DynText : public touchgfx::Widget
{
public:
    static const uint16_t MAX_CHARS = 32;

    DynText()
        : fontId(Typography::DEFAULT),
          color(touchgfx::Color::getColorFromRGB(255, 255, 255)),
          alignment(touchgfx::LEFT)
    {
        buf[0] = 0;
    }

    void setFont(touchgfx::FontId id)
    {
        fontId = id;
        invalidate();
    }

    void setColor(touchgfx::colortype c)
    {
        color = c;
        invalidate();
    }

    void setAlignment(touchgfx::Alignment a)
    {
        alignment = a;
        invalidate();
    }

    /** ASCII text; truncated to MAX_CHARS. Redraws only when the text changed. */
    void setText(const char* text)
    {
        Unicode::UnicodeChar next[MAX_CHARS + 1];
        Unicode::strncpy(next, text, MAX_CHARS);
        next[MAX_CHARS] = 0;
        if (Unicode::strncmp(next, buf, MAX_CHARS) != 0)
        {
            Unicode::strncpy(buf, next, MAX_CHARS);
            buf[MAX_CHARS] = 0;
            invalidate();
        }
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const
    {
        const touchgfx::Font* font = touchgfx::FontManager::getFont(fontId);
        if (font == 0 || buf[0] == 0)
        {
            return;
        }
        touchgfx::LCD::StringVisuals visuals(font, color, 255, alignment, 0,
                                             touchgfx::TEXT_ROTATE_0, touchgfx::TEXT_DIRECTION_LTR, 0);
        touchgfx::HAL::lcd().drawString(getAbsoluteRect(), invalidatedArea, visuals, buf);
    }

    virtual touchgfx::Rect getSolidRect() const
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    touchgfx::FontId fontId;
    touchgfx::colortype color;
    touchgfx::Alignment alignment;
    Unicode::UnicodeChar buf[MAX_CHARS + 1];
};

#endif // DYNTEXT_HPP
