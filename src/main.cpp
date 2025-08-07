#include <Arduino.h>
#include "main.h"
#include "reverb.h"
#include "echo.h"
#include "octaver.h"
#include "distortion.h"
#include "sinewave.h"

int16_t input; // Will hold the raw signed 16-bit input sample
unsigned int ADC_low, ADC_high; // For direct ADC read in ISR
int counter=0;
volatile int pot2_value = 10000;


uint16_t delayBuffer[MAX_DELAY]; 
uint32_t delayWritePointer = 0;
uint32_t delayReadOffset;
uint32_t delayDepth;

volatile bool effectActive = false; 
volatile EffectMode currentActiveMode = NORMAL_MODE; // Initially set, will be updated by setup
volatile EffectMode lastSelectedMode = NORMAL_MODE;  // Stores the last non-CLEAN effect mode

const long SAMPLE_RATE_MICROS = 50;

void setup() {
  Serial.begin(9600);

  pinConfig();
  adcSetup();
  pmwSetup();
  
  /*Clear delay buffer*/
  for (int i = 0; i < MAX_DELAY; i++) {
        delayBuffer[i] = 0;
  }

  /*Initialize specific effect modules*/
  #ifdef OCTAVER
  setupOctaver();
  lastSelectedMode = OCTAVER_MODE;
  #endif

  #ifdef REVERB
  setUpReverb();
  lastSelectedMode = REVERB_ECHO_MODE; 
  #endif

  #ifdef ECHO
  setupEcho();
  lastSelectedMode = ECHO_MODE;
  #endif

  #ifdef DISTORTION
  setupDistortion();
  lastSelectedMode = DISTORTION_MODE;
  #endif

  #ifdef SINEWAVE
  setupSinewave();
  lastSelectedMode = SINEWAVE_MODE;
  #endif

/*Initial state after setup: go to lastSelectedMode unless FOOTSWITCH is pressed for CLEAN*/
  lastSelectedMode = NORMAL_MODE; 
  currentActiveMode = lastSelectedMode; 
  effectActive = true;

  Serial.println("Arduino Audio Pedal Ready!");
}

void loop() {
/*FOOTSWITCH is pressed (LOW), force CLEAN_MODE. Otherwise, run the last selected effect*/
if (digitalRead(FOOTSWITCH) == LOW) { 
    currentActiveMode = CLEAN_MODE; 
    effectActive = false;
    digitalWrite(LED, LOW);
} 
else 
{ 
    currentActiveMode = lastSelectedMode; // Revert to last selected effect
    effectActive = true; // Indicate effect is active
    digitalWrite(LED, HIGH); // LED ON when effect is active
}

volumeControl();

/*EFFECT SELECTION */
bool buttonA3Pressed = (digitalRead(SELECT_OCTAVER_BUTTON) == LOW);
bool buttonA4Pressed = (digitalRead(SELECT_NORMAL_BUTTON) == LOW);
bool buttonA5Pressed = (digitalRead(SELECT_REVERB_BUTTON) == LOW);
bool buttonA6Pressed = (digitalRead(SELECT_ECHO_BUTTON) == LOW);
bool buttonA7Pressed = (digitalRead(SELECT_DISTORTION_BUTTON) == LOW);
bool buttonA8Pressed = (digitalRead(SELECT_SINEWAVE_BUTTON) == LOW);

/*If any selection button is pressed, it takes precedence over FOOTSWITCH and activates its effect momentarily*/
if (buttonA3Pressed || buttonA4Pressed || buttonA5Pressed || buttonA6Pressed || buttonA7Pressed || buttonA8Pressed) {
    
    if (buttonA3Pressed) {
        #ifdef OCTAVER
        lastSelectedMode = OCTAVER_MODE;
        currentActiveMode = OCTAVER_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }

    if (buttonA4Pressed ) {
        #ifdef NORMAL
        lastSelectedMode = NORMAL_MODE; 
        currentActiveMode = NORMAL_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }
    
    if (buttonA5Pressed ) {
        #ifdef REVERB
        lastSelectedMode = REVERB_ECHO_MODE;
        currentActiveMode = REVERB_ECHO_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }
    
    if (buttonA6Pressed ) {
        #ifdef ECHO
        lastSelectedMode = ECHO_MODE;
        currentActiveMode = ECHO_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }

    if (buttonA7Pressed) {
        #ifdef DISTORTION
        lastSelectedMode = DISTORTION_MODE;
        currentActiveMode = DISTORTION_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }

    if (buttonA8Pressed) {
        #ifdef SINEWAVE
        lastSelectedMode = SINEWAVE_MODE;
        currentActiveMode = SINEWAVE_MODE;
        #endif
        effectActive = true;
        digitalWrite(LED, HIGH);
    }

    /*Clear delay buffer to prevent sound artifacts from previous modes*/
    for (int i = 0; i < MAX_DELAY; i++) {
        delayBuffer[i] = 0;
    }
    delayWritePointer = 0;

} 
else 
{ 
    // Nothing to do here explicitly, as it was handled already.
}

/*Effect-specific loop functions*/
#ifdef REVERB
if (currentActiveMode == REVERB_ECHO_MODE || currentActiveMode == DELAY_MODE) {
    loopReverb();
}
#endif

#ifdef ECHO
if (currentActiveMode == ECHO_MODE) {
    loopEcho();
}
#endif

#ifdef OCTAVER
if (currentActiveMode == OCTAVER_MODE) {
    loopOctaver();
}
#endif

#ifdef DISTORTION
if (currentActiveMode == DISTORTION_MODE) {
    loopDistortion();
}
#endif

#ifdef SINEWAVE
if (currentActiveMode == SINEWAVE_MODE) {
    loopSinewave();
}
#endif
}

/*Timer 1 input capture event ISR*/
ISR(TIMER1_CAPT_vect) 
{
  /*Get ADC data*/
  ADC_low = ADCL; // You need to fetch the low byte first
  ADC_high = ADCH;

  /*Construct the input sumple summing the ADC low and high byte*/
  input = ((ADC_high << 8) | ADC_low) + 0x8000; // Make a signed 16b value

  /*All DSP happens here*/
    switch (currentActiveMode) {
        case NORMAL_MODE:
            processNormalAudio(input);
            break;
        case REVERB_ECHO_MODE:
        case DELAY_MODE:
            processReverbAudio(input);
            break;
        case ECHO_MODE:
            processEchoAudio(input);
            break;
        case OCTAVER_MODE:
            processOctaverAudio(input);
            break;
        case DISTORTION_MODE:
            processDistortionAudio(input);
            break;
        case SINEWAVE_MODE:
            processSinewaveAudio(input); // Pass raw input, though generator may ignore
            break;
        case CLEAN_MODE: // Explicit CLEAN_MODE selected via effect bypass logic or momentary release
        default:
         /*Simple pass-through*/
        int output_val_clean = input;
      
        /*Volume scaling*/
        output_val_clean = map(output_val_clean, -32768, +32768,-pot2_value, pot2_value);

        /*Write the PWM output signal*/
        OCR1AL = ((output_val_clean + 0x8000) >> 8); // convert to unsigned, send out high byte
        OCR1BL = output_val_clean; // send out low byte
        break;
    }
}

void processNormalAudio(int16_t input_val) {
    int16_t output_val = input_val;

    /* volume scaling*/
    output_val = map(output_val, -32768, +32768,-pot2_value, pot2_value);

    /*Write the PWM output signal*/
    OCR1AL = ((output_val + 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = output_val; // send out low byte
}

void pinConfig (void){
    pinMode(FOOTSWITCH, INPUT_PULLUP);
    pinMode(TOGGLE, INPUT_PULLUP);
    pinMode(PUSHBUTTON_1, INPUT_PULLUP);
    pinMode(PUSHBUTTON_2, INPUT_PULLUP);

    /*Configure former POT pins as digital inputs for effect selection*/
    pinMode(SELECT_OCTAVER_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_NORMAL_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_REVERB_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_ECHO_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_DISTORTION_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_SINEWAVE_BUTTON, INPUT_PULLUP);

    /*Configure audio input and output pins*/
    pinMode(AUDIO_OUT_A, OUTPUT); //PWM0 as output
    pinMode(AUDIO_OUT_B, OUTPUT); //PWM1 as output

    pinMode(LED, OUTPUT);
}

void adcSetup(void){   
    ADMUX = 0x60;  // ADMUX: Reference (AVcc), Left Adjust Result, ADC0 (A0) selected
    ADCSRA = 0xE5; // ADCSRA: ADC Enable, ADC Start Conversion, ADC Auto Trigger Enable, Prescaler CK/32
    ADCSRB = 0x07; // ADCSRB: ADC Auto Trigger Source: Timer/Counter1 Capture Event
    DIDR0 = 0x01;  // DIDR0: Disable Digital Input Buffer for ADC0 (A0) to reduce noise.
}

void pmwSetup(void){
  TCCR1A = (((PWM_QTY - 1) << 5) | 0x80 | (PWM_MODE << 1)); //
  TCCR1B = ((PWM_MODE << 3) | 0x11); // ck/1
  TIMSK1 = 0x20; // interrupt on capture interrupt
  ICR1H = (PWM_FREQ >> 8);
  ICR1L = (PWM_FREQ & 0xff);
  // DDRB |= ((PWM_QTY << 1) | 0x02); // turn on outputs
  
  /*Alternative turn on outputs*/
  pinMode(AUDIO_OUT_A, OUTPUT); // OCR1A
  pinMode(AUDIO_OUT_B, OUTPUT); // OCR1B

  sei(); // turn on global interrupts - not really necessary with arduino
}

/*To save resources, the pushbuttons are checked every 100 times*/
void volumeControl(void) {
  counter++; 
  if(counter==100){ 
    counter=0;
  if (!digitalRead(PUSHBUTTON_2)) {
  if (pot2_value<32768)pot2_value=pot2_value+20; //increase the vol
    digitalWrite(LED, LOW); //blinks the led
    }
  if (!digitalRead(PUSHBUTTON_1)) {
  if (pot2_value>0)pot2_value=pot2_value-20; //decrease vol
  digitalWrite(LED, LOW); //blinks the led
    }
  }
}