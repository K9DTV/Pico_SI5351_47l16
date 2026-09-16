#include "vfo.h"
#include <Wire.h>

void display_init()
{
    // Correct SH1106 initialization for 128x64 display
    if (!display.begin(0x3C,true)) {
        Serial.println("Display init failed");
    }
}

void miniDisp_init()
{
    // Correct SSD1306 initialization for 64x32 display
    if (!miniDisp.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Display init failed");
    }
}

void miniDisp_update()
{
    miniDisp.clearDisplay();
    miniDisp.setTextColor(SSD1306_WHITE);
    miniDisp.setTextSize(1);

    // Auto-step indicator
    miniDisp.setCursor(0, 8);
    miniDisp.print(autoStepEnabled ? "Auto ON" : "Auto OFF");

    // Frequency readout
    miniDisp.setCursor(0, 0);;
    miniDisp.printf("%lu.%06lu ",
        (unsigned long)(frequency / 1000000),
        (unsigned long)(frequency % 1000000));

    // Step readout
    uint64_t step = stepSizes[stepIndex];

    miniDisp.setCursor(0, 16);
    if (step < 1000ULL)
        miniDisp.printf("Step %lu Hz", (unsigned long)step);
    else if (step < 1000000)
        miniDisp.printf("Step %lu KHz", (unsigned long)(step / 1000));
    else
        miniDisp.printf("Step %lu MHz", (unsigned long)(step / 1000000));

    miniDisp.display();
}    

void display_update()
{
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    // Auto-step indicator
    display.setCursor(0, 8);
    display.print(autoStepEnabled ? "Auto ON" : "Auto OFF");

    // Frequency readout
    display.setCursor(0, 0);
    display.printf("%lu.%06luMHz",
        (unsigned long)(frequency / 1000000),
        (unsigned long)(frequency % 1000000));

    // Step readout
    uint64_t step = stepSizes[stepIndex];

    display.setCursor(0, 16);
    if (step < 1000ULL)
        display.printf("Step %lu Hz", (unsigned long)step);
    else if (step < 1000000)
        display.printf("Step %lu KHz", (unsigned long)(step / 1000));
    else
        display.printf("Step %lu MHz", (unsigned long)(step / 1000000));

    display.display();
}

// Flash Message msg
void display_flashMessage(const char* msg)
{
    for (int i = 0; i < 2; i++)
    {
        display.setCursor(0, 24);
        display.print(msg);
        display.display();
        delay(180);

        display.fillRect(0, 24, 106, 10, SH110X_BLACK);
        display.display();
        delay(180);
    }
}

void markActivity()
{
    lastActivityTime = millis();

    if (isDimmed)
    {
        //display.dim(false);
        isDimmed = false;
    }
}

// Dim display after dimDelay
void display_checkDimmer()
{
    if (!isDimmed && (millis() - lastActivityTime > dimDelay))
    {
        //display.dim(true);
        isDimmed = true;
    }
}
