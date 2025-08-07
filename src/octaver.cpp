#include "octaver.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigOctaver() {
    // No specific pins for Octaver, common pins configured in main.cpp
}

void setupOctaver(){
    Serial.println("Octaver Pedal Ready!");
}

void loopOctaver(){
    // No specific loop logic for Octaver, controls handled in main.cpp
}

void processOctaverAudio(int16_t inputSample) {
    float outputSampleFloat = (float)inputSample;

    if (effectActive) {
        /*Simple sub-octave simulation: half-wave rectification*/
        float sub_octave = outputSampleFloat;
        if (sub_octave < 0) sub_octave = 0;
        /*Mix dry (50%) and sub-octave (50%)*/
        outputSampleFloat = (outputSampleFloat * 0.5) + (sub_octave * 0.5);
        /*Soft clipping*/
        if (outputSampleFloat > 25000.0) outputSampleFloat = 25000.0 * tanh(outputSampleFloat / 25000.0);
        if (outputSampleFloat < -25000.0) outputSampleFloat = -25000.0 * tanh(outputSampleFloat / -25000.0);
        outputSampleFloat = constrain(outputSampleFloat, -32768, 32767);
    }

    /*Apply volume control & constrain*/
    outputSampleFloat= map(outputSampleFloat, -32768, +32768,-pot2_value, pot2_value);
    int16_t finalOutputSample = (int16_t)constrain(outputSampleFloat, -32768, 32767);

    /*Write the PWM output signal*/
    OCR1AL = ((finalOutputSample+ 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}