#include "sinewave.h"
#include <Arduino.h>

#define SINE_TABLE_SIZE 256

static const float SAMPLE_RATE_HZ = 1000000.0 / SAMPLE_RATE_MICROS;

static int16_t nSineTable[SINE_TABLE_SIZE];

static float phase_accumulator = 0.0;
static float frequency_step = 0.0;
static int16_t generated_sample = 0;

/*********************************************FUNCTION DEFINITIONS****************************************************/
void createSineTable(void){
    for (uint16_t nIndex = 0; nIndex < SINE_TABLE_SIZE; nIndex++) {
        nSineTable[nIndex] = (int16_t)(sin(((2.0 * PI) / SINE_TABLE_SIZE) * nIndex) * 32767.0);
    }
}

void pinConfigSinewave(void){
    // No specific pins for Sinewave, common pins configured in main.cpp
}

void setupSinewave(void){
    createSineTable();
    Serial.println("SineWave Generator Ready!");

    // Set a fixed frequency step for a perceivable sine wave
    const float fixedFrequencyHz = 440.0; // A4 note (440 Hz)
    frequency_step = (fixedFrequencyHz * SINE_TABLE_SIZE) / SAMPLE_RATE_HZ;
}

void loopSinewave(void){
    // No specific loop logic for Sinewave, controls handled in main.cpp
}

void processSinewaveAudio(int16_t inputSample) {
    float outputSampleFloat;

    if (effectActive) {
        phase_accumulator += frequency_step;

        while (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
        }
        while (phase_accumulator < 0.0) {
            phase_accumulator += SINE_TABLE_SIZE;
        }

        int idx1 = (int)phase_accumulator;
        float fraction = phase_accumulator - idx1;
        int idx2 = (idx1 + 1) % SINE_TABLE_SIZE;

        int16_t sample1 = nSineTable[idx1];
        int16_t sample2 = nSineTable[idx2];

        // Interpolate between sample1 and sample2
        generated_sample = (int16_t)(sample1 + fraction * (sample2 - sample1));
        
        outputSampleFloat = (float)generated_sample;
    } else {
        outputSampleFloat = 0.0; // Silence
        phase_accumulator = 0.0; // Reset phase
    }

    /*Apply amplitude control*/
    outputSampleFloat = map(outputSampleFloat, -32768, +32768,-pot2_value, pot2_value);
    /*Constrain to 16-bit range*/
    int16_t finalOutputSample = (int16_t)constrain(outputSampleFloat, -32768, 32767);

    /*Write the PWM output signal*/
    OCR1AL = ((finalOutputSample + 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}