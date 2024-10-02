
#ifndef _TestToneControl_h
#define _TestToneControl_h

//These should existing in the main *.ino file
extern void printInputSignalLevels(unsigned long, unsigned long);
extern void printOutputSignalLevels(unsigned long, unsigned long);

float setSteppedTone(float freq_Hz) {
  Serial.println("setSteppedTone: setting frequency to " + String(freq_Hz) + " Hz");
  setFrequency_Hz(freq_Hz);                    //setFrequency is in "AudioProcessing.h"
  setAmplitude(myState.default_sine_amplitude);  //setAmplitude is in "AudioProcessing.h"
  myState.stepped_test_next_change_millis = millis() + (unsigned long)(1000.0*myState.stepped_test_step_dur_sec);
  return myState.sine_freq_Hz;
}

int switchTestToneMode(int new_mode) {
  switch (new_mode) {
    case State::TONE_MODE_MUTE:
      myState.current_test_tone_mode = new_mode;
      Serial.print("switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
      setFrequency_Hz(myState.default_sine_freq_Hz);  //setFrequency is in "AudioProcessing.h"
      setAmplitude(0.0);  //mute   (setAmplitude is in "AudioProcessing.h")
      break;
    case State::TONE_MODE_STEADY:
      myState.current_test_tone_mode = new_mode;
      Serial.print("switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
      setFrequency_Hz(myState.default_sine_freq_Hz);  //setFrequency is in "AudioProcessing.h"
      setAmplitude(myState.default_sine_amplitude);   //setAmplitude is in "AudioProcessing.h"
      break;
    case State::TONE_MODE_STEPPED_FREQUENCY:
      myState.current_test_tone_mode = new_mode;
      Serial.print("switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
      setSteppedTone(myState.stepped_test_start_freq_Hz);
      break;   
    default:
      Serial.println("switchTestToneMode: *** WARNING ***: mode = " + String(new_mode) + " not recognized.  Ignoring.");
      break;
  }
  return myState.current_test_tone_mode;
}


void serviceSteppedToneTest(unsigned long current_millis) {
  if ( myState.current_test_tone_mode != State::TONE_MODE_STEPPED_FREQUENCY) return;
  if (current_millis >= myState.stepped_test_next_change_millis) {
    //compute the next test tone frequency
    float new_freq_Hz = myState.sine_freq_Hz + myState.stepped_test_step_Hz; 
    
    //we're about to make a change, so print the current levels
    if (myState.flag_printOutputLevelToUSB) printOutputSignalLevels(millis(),0);  //the "0" forces it to print now, regardless of the current time (millis())
    if (myState.flag_printInputLevelToUSB) printInputSignalLevels(millis(),0);  //the "0" forces it to print now, regardless of the current time (millis())

    //is this frequency allowed or are we done?
    if (new_freq_Hz > myState.stepped_test_end_freq_Hz) {
      Serial.println("serviceStppedToneTest: stepped-tone test completed!");
      switchTestToneMode(State::TONE_MODE_MUTE);
    } else {
      //set the sine wave to the new frequency
      setSteppedTone(new_freq_Hz);
    }
  }
}

void printTestToneMode(void) {
  switch (myState.current_test_tone_mode) {
    case State::TONE_MODE_MUTE:
      Serial.print("Muted"); break;
    case State::TONE_MODE_STEADY:
      Serial.print("Steady Tone"); break;
    case State::TONE_MODE_STEPPED_FREQUENCY:
      Serial.print("Stepped Tones"); break;
  }
}

#endif