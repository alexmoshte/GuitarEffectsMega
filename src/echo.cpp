#include "echo.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigEcho(){
    // No specific pins for Echo, common pins configured in main.cpp
}

void setupEcho(){
    Serial.println("Echo Pedal Ready!");
}

void loopEcho(){
    // No specific loop logic for Echo, controls handled in main.cpp
}

void processEchoAudio(int16_t inputSample) {
    float outputSampleFloat = (float)inputSample;

    if (effectActive) {
        const int fixedEchoDelayTimeValue = 600;
        const float fixedEchoFeedbackFactor = 0.5;

        int currentDelayDepth = map(fixedEchoDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - currentDelayDepth)) % MAX_DELAY;

        float delayedSample_Centered = (float)delayBuffer[delayReadPointer];

        float newSampleForBuffer_Centered = outputSampleFloat + (delayedSample_Centered * fixedEchoFeedbackFactor);
        
        delayBuffer[delayWritePointer] = (int16_t)constrain(newSampleForBuffer_Centered, -32768, 32767);

        outputSampleFloat = outputSampleFloat + delayedSample_Centered;
        outputSampleFloat = constrain(outputSampleFloat, -32768, 32767);
    } else {
        outputSampleFloat = (float)inputSample;
        for (int i = 0; i < MAX_DELAY; i++) { delayBuffer[i] = 0; }
        delayWritePointer = 0;
    }

    delayWritePointer++;
    if (delayWritePointer >= MAX_DELAY) {
        delayWritePointer = 0;
    }

    /*Apply volume control & constrain*/
    outputSampleFloat= map(outputSampleFloat, -32768, +32768,-pot2_value, pot2_value);
    int16_t finalOutputSample = (int16_t)constrain(outputSampleFloat, -32768, 32767);

    /*Write the PWM output signal*/
    OCR1AL = ((finalOutputSample+ 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}