#include "distortion.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigDistortion(){
    // No specific pins for Distortion, common pins configured in main.cpp
}

void setupDistortion(){
    Serial.println("Distortion Pedal Ready!");
}

void loopDistortion(){
    // No specific loop logic for Distortion, controls handled in main.cpp
}

void processDistortionAudio(int16_t inputSample) {
    float outputSampleFloat = (float)inputSample;

    if (effectActive) {
        const float fixedPreGainFactor = 2.3;
        const int16_t fixedDistortionThreshold = 10000; // Adjusted for 16-bit range

        float gained_input = outputSampleFloat * fixedPreGainFactor;

        if (gained_input > fixedDistortionThreshold) {
            outputSampleFloat = fixedDistortionThreshold;
        } else if (gained_input < -fixedDistortionThreshold) {
            outputSampleFloat = -fixedDistortionThreshold;
        } else {
            outputSampleFloat = gained_input;
        }

        outputSampleFloat = constrain(outputSampleFloat, -32768, 32767);
    }

    /*Apply volume control & constrain*/
    outputSampleFloat= map(outputSampleFloat, -32768, +32768,-pot2_value, pot2_value);
    int16_t finalOutputSample = (int16_t)constrain(outputSampleFloat, -32768, 32767);

    /*Write the PWM output signal*/
    OCR1AL = ((finalOutputSample+ 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}