#pragma once
#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_SSD1306.h>
#include <cstdint>

// GLOBAL STATE

// All Pins used
#define sda0 0
#define scl0 1
#define sda1 26
#define scl1 27
#define BACK_PIN 11
#define CONFIRM_PIN 12
#define ENC_A 13
#define ENC_B 14
#define ENC_SW 15

// Frequency.cpp
inline uint32_t frequency = 10000000;
inline int stepIndex = 0;
inline bool autoStepEnabled = true;

inline const uint32_t stepSizes[] = {
    1, 10, 100, 1000,
    10000, 100000, 1000000, 10000000
};

inline static const int NUM_STEPS = sizeof(stepSizes) / sizeof(stepSizes[0]);

#define minFreq 5000
#define maxFreq 160000000
void adjustFrequency(int direction);
void step_next();

// Display.cpp
#define dimDelay 300000
inline Adafruit_SH1106G display(128, 64, &Wire, -1);
inline Adafruit_SSD1306 miniDisp(64, 32, &Wire1);
inline uint32_t lastActivityTime;
inline bool isDimmed = false;
void display_init();
void display_update();
void display_flashMessage(const char* msg);

void miniDisp_init();
void miniDisp_update();
void miniDisp_flashMessage(const char* msg);
void miniDisp_checkDimmer();

void display_checkDimmer();
void markActivity();

// Encoder.cpp
int encoder_getRotation();
bool encoder_checkSwitch(bool &longPress);
#define longPressDelay 350
#define ENCODER_DETENT 4
#define DEBOUNCE_MS 40

// button.cpp
bool button_backPressed();
bool button_confirmPressed();

// SI5351.cpp
#define SI5351_ADDR 0x60
#define SI5351_XTAL_FREQ 25000000ULL
inline static uint8_t output_enable_reg = 0xFF;
void si5351_init();
void si5351_set(uint64_t freq, uint8_t channel = 0);
// ============================================================================
// CALIBRATION ENTRY
// ============================================================================
#define OFFSET_AT_150MHZ 18880//18880 fake board// real board-3668
// Compiler automatically calculates the exact target XTAL (25 MHz base)
inline uint64_t CALIBRATED_XTAL = (25000000ULL + ((OFFSET_AT_150MHZ) / 6));

// Memory.cpp
void loadSettings();
bool saveSettings();
#define SETTINGS_SIGNATURE     0xDEADBEEF
#define SETTINGS_VERSION       1
#define SETTINGS_TAIL_MAGIC    0xCAFEBABE

// VFO_Project.ino
void real_setup();
void real_loop();