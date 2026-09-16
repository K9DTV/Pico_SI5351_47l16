#include "vfo.h"

int encoder_getRotation()
{
    static uint8_t lastAB = 0;
    static int accum = 0;
    static uint32_t lastTime = 0;

    uint8_t a = digitalRead(ENC_A);
    uint8_t b = digitalRead(ENC_B);
    uint8_t current = (a << 1) | b;

    if (current == lastAB)
        return 0;

    uint32_t now = millis();

    uint8_t diff = lastAB ^ current;
    if (diff == 0b11)
    {
        lastAB = current;
        return 0;
    }

    int delta = 0;
    if ((lastAB == 0b00 && current == 0b01) ||
        (lastAB == 0b01 && current == 0b11) ||
        (lastAB == 0b11 && current == 0b10) ||
        (lastAB == 0b10 && current == 0b00))
        delta = +1;

    else if ((lastAB == 0b00 && current == 0b10) ||
             (lastAB == 0b10 && current == 0b11) ||
             (lastAB == 0b11 && current == 0b01) ||
             (lastAB == 0b01 && current == 0b00))
        delta = -1;

    lastAB = current;

    if (now - lastTime < 1)
        return 0;

    if ((now - lastTime) < 6 &&
        ((accum > 0 && delta < 0) || (accum < 0 && delta > 0)))
        return 0;

    lastTime = now;
    accum += delta;

    if (abs(accum) >= ENCODER_DETENT)
    {
        int out = (accum > 0 ? +1 : -1);
        accum = 0;
        return out;
    }

    return 0;
}

bool encoder_checkSwitch(bool &longPress)
{
    static bool pressed = false;
    static uint32_t pressTime = 0;
    static uint32_t lastChange = 0;

    longPress = false;

    uint32_t now = millis();
    bool raw = (digitalRead(ENC_SW) == LOW);

    if (raw != pressed && (now - lastChange) > DEBOUNCE_MS)
    {
        pressed = raw;
        lastChange = now;

        if (pressed)
            pressTime = now;
        else
        {
            if ((now - pressTime) >= longPressDelay)
                longPress = true;

            return true;
        }
    }

    return false;
}