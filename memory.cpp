#include "vfo.h"
#include <Arduino.h>
#include <Wire.h>

// ======================================================
// 47L16 I2C Device Addresses (HS pin floating -> Default Base 0x30)
// ======================================================
static const uint8_t EERAM_I2C_CONTROL = 0x18; // Control register address (0x30 >> 1)
static const uint8_t EERAM_I2C_SRAM    = 0x50; // SRAM Array address (0xA0 >> 1)

// 47L16 Software Command Op-Codes
static const uint8_t EERAM_CMD_STORE  = 0x33;   // Force SRAM -> EEPROM
static const uint8_t EERAM_CMD_RECALL = 0xDD;   // Force EEPROM -> SRAM

// Offset in SRAM where our settings struct resides[cite: 7]
static const uint16_t SRAM_SETTINGS_ADDR = 0x0000;

// ======================================================
// Packed struct (size = 32 bytes)
// ======================================================
struct __attribute__((packed)) Settings {
    uint32_t signature;        // 4[cite: 7]
    uint16_t version;          // 2[cite: 7]
    uint16_t structSize;       // 2  (8 total)[cite: 7]
    uint32_t sequence;         // 4  (12 total)[cite: 7]
    uint64_t frequency;        // 8  (20 total)[cite: 7]
    uint8_t  stepIndex;        // 1[cite: 7]
    uint8_t  autoStepEnabled;  // 1[cite: 7]
    uint16_t reserved;         // 2  (24 total)[cite: 7]
    uint32_t tailMagic;        // 4  (28 total)[cite: 7]
    uint32_t crc32;            // 4  (32 total)[cite: 7]
};

static Settings settings;

// Helper to check if runtime values differ from local struct
bool isSettingsDirty()
{
    return (frequency       != settings.frequency ||
            stepIndex       != settings.stepIndex ||
            autoStepEnabled != settings.autoStepEnabled);
}

// ======================================================
// CRC32
// ======================================================
static uint32_t crc32_update(uint32_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; i++)
        crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
    return crc;
}

static uint32_t crc32_compute(const uint8_t* data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
        crc = crc32_update(crc, data[i]);
    return crc ^ 0xFFFFFFFF;
}

// ======================================================
// Low-Level 47L16 Hardware I/O
// ======================================================

// Issues a control register command (STORE or RECALL)
static bool eeramSendControlCommand(uint8_t command)
{
    Wire1.beginTransmission(EERAM_I2C_CONTROL);
    Wire1.write(command);
    return (Wire1.endTransmission() == 0);
}

// Write buffer to 47L16 SRAM space (Address 0x50)[cite: 7]
static bool eeramWriteSRAM(uint16_t memAddress, const uint8_t* data, size_t length)
{
    Wire1.beginTransmission(EERAM_I2C_SRAM);
    Wire1.write((uint8_t)(memAddress >> 8));   // High byte of address[cite: 7]
    Wire1.write((uint8_t)(memAddress & 0xFF)); // Low byte of address[cite: 7]
    Wire1.write(data, length);
    return (Wire1.endTransmission() == 0);
}

// Read buffer from 47L16 SRAM space (Address 0x50)[cite: 7]
static bool eeramReadSRAM(uint16_t memAddress, uint8_t* data, size_t length)
{
    Wire1.beginTransmission(EERAM_I2C_SRAM);
    Wire1.write((uint8_t)(memAddress >> 8));   // High byte of address[cite: 7]
    Wire1.write((uint8_t)(memAddress & 0xFF)); // Low byte of address[cite: 7]
    if (Wire1.endTransmission(false) != 0) {   // Repeated start[cite: 7]
        return false;
    }

    uint8_t bytesRead = Wire1.requestFrom((uint8_t)EERAM_I2C_SRAM, (uint8_t)length);
    if (bytesRead != length) return false;

    for (size_t i = 0; i < length; i++) {
        data[i] = Wire1.read();
    }
    return true;
}

// ======================================================
// Validation & Clamping
// ======================================================
static void clampSettings(Settings& s)
{
    if (s.frequency < minFreq) s.frequency = minFreq;
    if (s.frequency > maxFreq) s.frequency = maxFreq;
}

static bool validateSettings(const Settings* s)
{
    if (s->signature != SETTINGS_SIGNATURE) return false;
    if (s->version   != SETTINGS_VERSION)   return false;
    if (s->structSize != sizeof(Settings))  return false;
    if (s->tailMagic != SETTINGS_TAIL_MAGIC) return false;

    size_t len = sizeof(Settings) - sizeof(uint32_t);
    uint32_t crc = crc32_compute((const uint8_t*)s, len);
    if (crc != s->crc32) return false;

    if (s->frequency < minFreq || s->frequency > maxFreq) return false;
    if (s->stepIndex >= 8) return false;

    return true;
}

// ======================================================
// Public Memory Interface
// ======================================================

bool storeEERAM()
{
    return eeramSendControlCommand(EERAM_CMD_STORE);
}

bool recallEERAM()
{
    return eeramSendControlCommand(EERAM_CMD_RECALL);
}

// Load settings from 47L16 SRAM into application memory on boot
void loadSettings()
{
    Wire1.begin();

    Settings temp;
    if (eeramReadSRAM(SRAM_SETTINGS_ADDR, (uint8_t*)&temp, sizeof(Settings)))
    {
        if (validateSettings(&temp))
        {
            settings = temp;
            clampSettings(settings);

            frequency       = settings.frequency;
            stepIndex       = settings.stepIndex;
            autoStepEnabled = settings.autoStepEnabled;
            return;
        }
    }

    // Invalid or missing settings in SRAM -> Load defaults
    frequency       = 10000000;
    stepIndex       = 0;
    autoStepEnabled = true;

    settings.signature       = SETTINGS_SIGNATURE;
    settings.version         = SETTINGS_VERSION;
    settings.structSize      = sizeof(Settings);
    settings.sequence        = 0;
    settings.frequency       = frequency;
    settings.stepIndex       = stepIndex;
    settings.autoStepEnabled = autoStepEnabled;
    settings.tailMagic       = SETTINGS_TAIL_MAGIC;

    size_t len = sizeof(Settings) - sizeof(uint32_t);
    settings.crc32 = crc32_compute((const uint8_t*)&settings, len);

    eeramWriteSRAM(SRAM_SETTINGS_ADDR, (const uint8_t*)&settings, sizeof(Settings));
}

// Automatically write modified runtime settings to 47L16 SRAM when dirty
bool saveSettings()
{
    if (!isSettingsDirty()) return false;

    settings.signature       = SETTINGS_SIGNATURE;
    settings.version         = SETTINGS_VERSION;
    settings.structSize      = sizeof(Settings);
    settings.sequence++;
    settings.frequency       = frequency;
    settings.stepIndex       = stepIndex;
    settings.autoStepEnabled = autoStepEnabled;
    settings.tailMagic       = SETTINGS_TAIL_MAGIC;

    clampSettings(settings);

    size_t len = sizeof(Settings) - sizeof(uint32_t);
    settings.crc32 = crc32_compute((const uint8_t*)&settings, len);

    // Directly write to fast 47L16 SRAM array
    return eeramWriteSRAM(SRAM_SETTINGS_ADDR, (const uint8_t*)&settings, sizeof(Settings));
}