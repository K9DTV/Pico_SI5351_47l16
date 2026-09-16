#include <Wire.h>
#include "vfo.h"

void real_setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("This is a test of USB serial");

    // I2C port 0 (SI5351 and SH1106 )
    Wire.setSDA(sda0);
    Wire.setSCL(scl0);
    Wire.begin();
    Wire.setClock(800000);

    // I2C port 1 (47L16 and SSD1306)
    Wire1.setSDA(sda1);
    Wire1.setSCL(scl1);
    Wire1.begin();
    Wire1.setClock(800000);

    si5351_init();      // Start si5351
    loadSettings();     // Recall setting from SRAM on boot


    adjustFrequency(0); // Set si5351 to recalled values (0 = no change to step)

    display_init();     // Start display
    display_update();   // Update display to recalled values

    miniDisp_init();     // Start display
    miniDisp_update();   // Update display to recalled values

    lastActivityTime = millis(); // set lastActivityTime to millis() that is zero

    // Setup all Pins used by encoder board (active-low, so use pull-ups)
    int pins[] = {ENC_A, ENC_B, ENC_SW, CONFIRM_PIN, BACK_PIN};
    for (int p : pins) pinMode(p, INPUT_PULLUP);
}

void real_loop()
{
    int rot = encoder_getRotation();
    if (rot != 0)
    {
        markActivity();
        adjustFrequency(rot);
        saveSettings(); // Automatically write to SRAM when frequency changes via encoder rotation
        display_update();
        miniDisp_update();
    }

    bool longPress = false;
    if (encoder_checkSwitch(longPress))
    {
        markActivity();

        if (longPress)
            autoStepEnabled = !autoStepEnabled;
        else
            step_next();

        saveSettings(); // Automatically write to SRAM when autoStep or stepIndex changes via switch
        display_update();
        miniDisp_update();
    }

    if (button_backPressed())
    {
        markActivity();
        loadSettings();
        adjustFrequency(0);
        display_update();
        miniDisp_update();
        display_flashMessage("SETTINGS RECALLED");
    }

    if (button_confirmPressed())
    {
        markActivity();
        // Manual save trigger via confirm button (keeps manual capability intact while auto-saving)
        saveSettings();
        display_flashMessage("SETTINGS SAVED");
    }

    display_checkDimmer();
}
