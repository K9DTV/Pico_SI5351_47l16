#include "vfo.h"
#include <Arduino.h>

void step_next()
{
    stepIndex = (stepIndex + 1) % NUM_STEPS;
}

void autoStepInc(uint64_t oldFreq, uint64_t newFreq)
{
    if (!autoStepEnabled) return;
    if (stepIndex >= (NUM_STEPS - 1)) return;

    uint64_t nextStep = stepSizes[stepIndex + 1];

    uint64_t oldDigit = (oldFreq / nextStep) % 10;
    uint64_t newDigit = (newFreq / nextStep) % 10;

    if (oldDigit != newDigit)
        stepIndex++;
}

void adjustFrequency(int direction)
{
    // Fixes step being subtracted from frequency when loadSettings() is called
    if (direction == 0)
    {
        si5351_set(frequency);
        return;
    }

    uint64_t step = stepSizes[stepIndex];
    uint64_t newFreq = frequency + (direction > 0 ? step : -step);

    if (newFreq >= minFreq && newFreq <= maxFreq)
    {
        if (direction > 0)
            autoStepInc(frequency, newFreq);

        frequency = (uint32_t)newFreq;
        si5351_set(frequency);
        return;
    }

    // Down-step fallback near limits
    uint8_t newStepIndex = stepIndex;

    while (newStepIndex > 0)
    {
        newStepIndex--;
        uint64_t smallerStep = stepSizes[newStepIndex];
        uint64_t testFreq = frequency + (direction > 0 ? smallerStep : -smallerStep);

        if (testFreq >= minFreq && testFreq <= maxFreq)
        {
            stepIndex = newStepIndex;
            frequency = (uint32_t)testFreq;
            si5351_set(frequency);
            return;
        }
    }

    display_flashMessage(direction > 0 ? "UPPER LIMIT" : "LOWER LIMIT");
}