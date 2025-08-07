#include "reverb.h"
#include <Arduino.h>
#include "main.h"

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigReverb(){
    // No specific pins for Reverb, common pins configured in main.cpp
}

void setUpReverb(){
    Serial.println("Reverb Pedal Ready!");
}

void loopReverb(){
    if (currentActiveMode == REVERB_ECHO_MODE || currentActiveMode == DELAY_MODE) {
        bool toggleState = digitalRead(TOGGLE);
        EffectMode targetMode = (toggleState == HIGH) ? REVERB_ECHO_MODE : DELAY_MODE;

        if (currentActiveMode != targetMode) {
                currentActiveMode = targetMode; // Update the global current effect mode.
                Serial.print("Reverb Sub-Mode: ");
                Serial.println((currentActiveMode == REVERB_ECHO_MODE) ? "REVERB (Echo)" : "DELAY (Repeats)");
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
        }
    }
}

void processReverbAudio(int16_t inputSample) {
    float outputSampleFloat;
    float centered_input = (float)inputSample;

    if (effectActive) {
        const int fixedDelayTimeValue = 500;
        const float fixedFeedbackValue = 0.75;
        const float fixedWetDryMix = 0.85;

        /*Map delay time to buffer size, assuming MAX_DELAY is defined appropriately*/
        delayReadOffset = map(fixedDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - delayReadOffset)) % MAX_DELAY;
        float delayedSample_Centered = (float)delayBuffer[delayReadPointer];

        float mixedSampleForBuffer_Centered;
        switch (currentActiveMode) {
            case REVERB_ECHO_MODE:
                mixedSampleForBuffer_Centered = centered_input * (1.0 - fixedFeedbackValue) + delayedSample_Centered * fixedFeedbackValue;
                outputSampleFloat = centered_input * (1.0 - fixedWetDryMix) + delayedSample_Centered * fixedWetDryMix;
                break;
            case DELAY_MODE:
                mixedSampleForBuffer_Centered = centered_input + (delayedSample_Centered * 0.5);
                outputSampleFloat = centered_input + delayedSample_Centered;
                break;
            case CLEAN_MODE:
            default:
                outputSampleFloat = centered_input;
                for (int i = 0; i < MAX_DELAY; i++) { delayBuffer[i] = 0; }
                delayWritePointer = 0;
                break;
        }
        delayBuffer[delayWritePointer] = (int16_t)constrain(mixedSampleForBuffer_Centered, -32768, 32767);
        outputSampleFloat = constrain(outputSampleFloat, -32768, 32767);
    } else {
        outputSampleFloat = centered_input;
        for (int i = 0; i < MAX_DELAY; i++) { delayBuffer[i] = 0; }
        delayWritePointer = 0;
    }

    delayWritePointer++;
    if (delayWritePointer >= MAX_DELAY) {
        delayWritePointer = 0;
    }

    int16_t finalOutputSample = (int16_t)outputSampleFloat;

    /*Scale output based on pot2_value, assuming pot2_value is in a reasonable range*/
    finalOutputSample= map(finalOutputSample, -32768, +32768,-pot2_value, pot2_value);
    finalOutputSample = constrain(finalOutputSample, -32768, 32767);

   /*Write the PWM output signal*/
    OCR1AL = ((finalOutputSample+ 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}

