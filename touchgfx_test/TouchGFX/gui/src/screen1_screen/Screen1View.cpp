#include <gui/screen1_screen/Screen1View.hpp>
#include <stdio.h>

Screen1View::Screen1View()
    : ledClickedCallback(this, &Screen1View::ledClickedHandler)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    const touchgfx::colortype white = touchgfx::Color::getColorFromRGB(240, 244, 248);
    const touchgfx::colortype accent = touchgfx::Color::getColorFromRGB(64, 196, 255);

    title.setFont(Typography::LARGE);
    title.setColor(accent);
    title.setAlignment(touchgfx::CENTER);
    title.setPosition(0, 2, 280, 46);
    title.setText("Live Status");
    add(title);

    DynText* rows[4] = { &bleText, &uptimeText, &heartbeatText, &fpsText };
    for (int i = 0; i < 4; i++)
    {
        rows[i]->setFont(Typography::DEFAULT);
        rows[i]->setColor(white);
        rows[i]->setPosition(16, 54 + i * 28, 248, 26);
        add(*rows[i]);
    }

    for (uint8_t i = 0; i < APP_LED_COUNT; i++)
    {
        ledButton[i].setBoundsAndLayout(12 + i * 134, 196, 122, 40, Typography::DEFAULT);
        ledButton[i].setLabelColor(white);
        ledButton[i].setClickedCallback(ledClickedCallback);
        add(ledButton[i]);
    }

    AppState s;
    AppState_Get(&s);
    updateState(s);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::updateState(const AppState& s)
{
    static const char* const bleNames[] = { "Idle", "Init", "Ready", "Advertising", "Connected", "Paired", "Error" };
    const uint8_t bleIdx = (s.ble_status < 7) ? s.ble_status : 6;
    char line[DynText::MAX_CHARS + 1];

    snprintf(line, sizeof(line), "BLE: %s", bleNames[bleIdx]);
    bleText.setText(line);

    snprintf(line, sizeof(line), "Uptime: %02lu:%02lu:%02lu",
             (unsigned long)(s.uptime_s / 3600UL),
             (unsigned long)((s.uptime_s / 60UL) % 60UL),
             (unsigned long)(s.uptime_s % 60UL));
    uptimeText.setText(line);

    snprintf(line, sizeof(line), "Heartbeat: %lu", (unsigned long)s.heartbeat);
    heartbeatText.setText(line);

    snprintf(line, sizeof(line), "FPS: %u", (unsigned)s.fps);
    fpsText.setText(line);

    static const char* const onLabels[APP_LED_COUNT] = { "LD1 ON", "LD3 ON" };
    static const char* const offLabels[APP_LED_COUNT] = { "LD1 OFF", "LD3 OFF" };
    for (uint8_t i = 0; i < APP_LED_COUNT; i++)
    {
        const bool on = s.led[i] != 0;
        ledButton[i].setLabel(on ? onLabels[i] : offLabels[i]);
        ledButton[i].setBaseColor(on ? touchgfx::Color::getColorFromRGB(38, 166, 91)
                                     : touchgfx::Color::getColorFromRGB(58, 62, 70));
    }
}

void Screen1View::ledClickedHandler(SoftButton& button)
{
    for (uint8_t i = 0; i < APP_LED_COUNT; i++)
    {
        if (&button == &ledButton[i])
        {
            presenter->ledToggle(i);
            return;
        }
    }
}
