#ifndef REVERB_H
#define REVERB_H
#include "main.h"

extern void pinConfigReverb(void);
extern void setUpReverb(void);
extern void loopReverb(void);
extern void processReverbAudio(int16_t inputSample);

#endif