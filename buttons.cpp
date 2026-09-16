#include "vfo.h"

bool checkButton(int pin, bool &locked, uint32_t &lastTime)
{
    uint32_t now = millis();
    bool pressed = (digitalRead(pin) == LOW);

    if (pressed)
    {
        if (!locked && (now - lastTime > DEBOUNCE_MS))
        {
            locked = true;
            lastTime = now;
            return true;
        }
    }
    else
    {
        if (locked && (now - lastTime > DEBOUNCE_MS))
        {
            locked = false;
            lastTime = now;
        }
    }
    return false;
}

bool backLocked = false;
uint32_t backLastTime = 0;

bool confirmLocked = false;
uint32_t confirmLastTime = 0;

bool button_backPressed()
{
    return checkButton(BACK_PIN, backLocked, backLastTime);
}

bool button_confirmPressed()
{
    return checkButton(CONFIRM_PIN, confirmLocked, confirmLastTime);
}
