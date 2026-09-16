#include "vfo.h"
#include <Wire.h>

static void siWriteBlock(uint8_t baseReg, uint8_t *values, uint8_t length)
{
    Wire.beginTransmission(SI5351_ADDR);
    Wire.write(baseReg);
    for (uint8_t i = 0; i < length; i++) {
        Wire.write(values[i]);
    }
    Wire.endTransmission();
}

static void siWrite(uint8_t reg, uint8_t value)
{
    siWriteBlock(reg, &value, 1);
}

static bool initialized = false;
static uint32_t last_ms_div = 0;

void si5351_init()
{
    siWrite(3, 0xFF);   // Disable outputs during init
    siWrite(17, 0x80);  // Power down CLK1
    siWrite(18, 0x80);  // Power down CLK2
    siWrite(16, 0x0F);  // CLK0 ON, Fractional MS0, PLLA source, 8mA
    siWrite(183, 0xC0); // 10 pF crystal load
    siWrite(177, 0xAC); // Soft reset PLLA & PLLB
    siWrite(3, 0xFE);   // Enable CLK0 output only

    initialized = true;
    last_ms_div = 0;
}

void si5351_calibrate_at_150mhz(int32_t error_hz)
{
    // Scaling ratio: 150 MHz / 25 MHz = 6:1
    int32_t xtal_correction = (error_hz + (error_hz >= 0 ? 3 : -3)) / 6;
    CALIBRATED_XTAL = 25000000ULL - xtal_correction;
}

void si5351_set(uint64_t freq, uint8_t channel)
{
    if (channel != 0 || freq == 0) return;
    if (!initialized) si5351_init();

    // 1. Determine R-divider for low frequencies (< 500 kHz)
    uint8_t r_div_bit = 0;
    uint32_t r_div = 1;
    while (freq * r_div < 500000ULL && r_div < 128) {
        r_div <<= 1;
        r_div_bit++;
    }

    uint64_t target_freq = freq * r_div;

    // 2. Select MultiSynth divider based on frequency boundaries
    uint32_t ms_div;
    if (target_freq >= 100000000ULL) {
        // High Speed VHF Mode: Force DIVBY4 mode for >= 100 MHz
        ms_div = 4;
    } else {
        // Standard Mode: Target ~750 MHz VCO
        ms_div = 750000000UL / target_freq;
        if (ms_div % 2 != 0) ms_div++; // Keep even
        if (ms_div < 6) ms_div = 6;
        if (ms_div > 1800) ms_div = 1800;
    }

    uint64_t vco_freq = target_freq * (uint64_t)ms_div;
    uint64_t xtal = CALIBRATED_XTAL; 

    // 3. High-Precision PLLA Feedback Math
    uint32_t a = (uint32_t)(vco_freq / xtal);
    uint64_t rem = vco_freq % xtal;

    uint32_t c = 1048575UL; 
    uint32_t b = (uint32_t)(((rem * (uint64_t)c) + (xtal / 2ULL)) / xtal);

    uint64_t num = 128ULL * b;
    uint32_t P1a = 128 * a + (uint32_t)(num / c) - 512;
    uint32_t P2a = (uint32_t)(num % c);
    uint32_t P3a = c;

    uint8_t pllData[8];
    pllData[0] = (P3a >> 8) & 0xFF;
    pllData[1] = P3a & 0xFF;
    pllData[2] = (P1a >> 16) & 0x03;
    pllData[3] = (P1a >> 8) & 0xFF;
    pllData[4] = P1a & 0xFF;
    pllData[5] = ((P3a >> 12) & 0xF0) | ((P2a >> 16) & 0x0F);
    pllData[6] = (P2a >> 8) & 0xFF;
    pllData[7] = P2a & 0xFF;

    siWriteBlock(26, pllData, 8);

    // 4. Set Register 16 Mode & Write MultiSynth0 Register
    uint8_t msData[8];

    if (ms_div == 4) {
        // CRITICAL FIX: Set Register 16 to Integer Mode (Bit 6 = 1 -> 0x4F)
        siWrite(16, 0x4F); 

        // DIVBY4 Mode Setup (AN619 Section 3.2)
        msData[0] = 0;
        msData[1] = 1;
        msData[2] = 0x0C | ((r_div_bit & 0x07) << 4); // Set DIVBY4 (0x0C)
        msData[3] = 0;
        msData[4] = 0;
        msData[5] = 0;
        msData[6] = 0;
        msData[7] = 0;
    } else {
        // Set Register 16 to Fractional Mode (Bit 6 = 0 -> 0x0F)
        siWrite(16, 0x0F);

        uint32_t P1b = 128 * ms_div - 512;
        uint32_t P2b = 0;
        uint32_t P3b = 1;

        msData[0] = (P3b >> 8) & 0xFF;                                       
        msData[1] = P3b & 0xFF;                                             
        msData[2] = ((P1b >> 16) & 0x03) | ((r_div_bit & 0x07) << 4);        
        msData[3] = (P1b >> 8) & 0xFF;                                       
        msData[4] = P1b & 0xFF;                                             
        msData[5] = ((P3b >> 12) & 0xF0) | ((P2b >> 16) & 0x0F);            
        msData[6] = (P2b >> 8) & 0xFF;                                       
        msData[7] = P2b & 0xFF;                                             
    }

    siWriteBlock(42, msData, 8);
    
    // Always reset PLLA on every write above 100 MHz or divider boundary change
    if (ms_div != last_ms_div || ms_div == 4) {
        siWrite(177, 0x20); // Soft reset PLLA
        last_ms_div = ms_div;
    }
}