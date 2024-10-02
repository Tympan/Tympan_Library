
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern State myState;


//Extern Functions (that live in a file other than this file here)
extern void setConfiguration(int);
extern float setInputGain_dB(float val_dB);
extern float setFrequency_Hz(float freq_Hz);
extern float setAmplitude(float amplitude);
extern int setOutputChan(int chan);
extern bool enablePrintMemoryAndCPU(bool _enable);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void) : SerialManagerBase() {};

    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    float inputGainIncrement_dB = 5.0;  //changes the input gain of the AIC
    float frequencyIncrement_factor = pow(2.0,1.0/3.0);  //third octave steps
    float amplitudeIcrement_dB = 1.0;  //changes the amplitude of the synthetic sine wave
  private:
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h:   Print this help");
  Serial.print(  "   w:   Switch Input to PCB Mics");                if (myState.input_source==State::INPUT_PCBMICS)   {Serial.println(" (active)");} else { Serial.println(); }
  Serial.print(  "   W:   Switch Input to MicIn on the Pink Jack");  if (myState.input_source==State::INPUT_JACK_MIC)  {Serial.println(" (active)");} else { Serial.println(); }
  Serial.print(  "   e:   Switch Input to LineIn on the Pink Jack"); if (myState.input_source==State::INPUT_JACK_LINE) {Serial.println(" (active)");} else { Serial.println(); }
  Serial.println("   i/I: Input: Increase or decrease input gain (current = " + String(myState.input_gain_dB,1) + " dB)");
  Serial.println("   f/F: Sine: Increase or decrease sine frequency (current = " + String(myState.sine_freq_Hz,1) + " Hz");
  Serial.println("   a/A: Sine: Increase or decrease sine amplitude (current = " + String(20*log10(myState.sine_amplitude)-3.0,1) + " dBFS (output), " + String(myState.sine_amplitude,3) + " amplitude)");
  Serial.println("   1/2/3: Sine: Output to left (1), right (2), or both (3)");
  Serial.print(  "   p/P: Printing: start/Stop printing of input signal levels"); if (myState.flag_printInputLevelToUSB)   {Serial.println(" (active)");} else { Serial.println(" (off)"); }
  Serial.print(  "   o/O: Printing: start/Stop printing of output signal levels"); if (myState.flag_printOutputLevelToUSB)   {Serial.println(" (active)");} else { Serial.println(" (off)"); }
  Serial.println("   r:   SD: Start recording audio to SD card");
  Serial.println("   s:   SD: Stop recording audio to SD card");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'c':
      Serial.println("SerialManager: enabling printing of memory and CPU usage.");
      myState.enable_printCpuToUSB = true;
      break;
    case 'C':
      Serial.println("SerialManager: disabling printing of memory and CPU usage.");
      myState.enable_printCpuToUSB = false;
      break;
    case 'w':
      Serial.println("SerialManager: Switch input to PCB Mics");
      setConfiguration(State::INPUT_PCBMICS);
      break;
    case 'W':
      Serial.println("SerialManager: Switch input to Mics on Jack.");
      setConfiguration(State::INPUT_JACK_MIC);
      break;
    case 'e':
      myTympan.println("SerialManager: Switch input to Line-In on Jack");
      setConfiguration(State::INPUT_JACK_LINE);
      break;
    case 'i':
      setInputGain_dB(myState.input_gain_dB + inputGainIncrement_dB);
      Serial.println("SerialManager: Increased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'I':   //which is "shift i"
      setInputGain_dB(myState.input_gain_dB - inputGainIncrement_dB);
      Serial.println("SerialManager: Decreased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'f':
      setFrequency_Hz(myState.sine_freq_Hz*frequencyIncrement_factor);
      Serial.println("SerialManager: increased tone frequency to " + String(myState.sine_freq_Hz));
      break;
    case 'F':
      setFrequency_Hz(myState.sine_freq_Hz/frequencyIncrement_factor);
      Serial.println("SerialManager: decreased tone frequency to " + String(myState.sine_freq_Hz));
      break;
    case 'a':
      setAmplitude(myState.sine_amplitude*sqrt(pow(10.0,0.1*amplitudeIcrement_dB))); //converting dB back to linear units
      Serial.println("SerialManager: increased tone amplitude to " + String(20.f*log10f(myState.sine_amplitude)-3.0,1) + " dB re: FS (output)");
      break;
    case 'A':
      setAmplitude(myState.sine_amplitude/sqrt(pow(10.0,0.1*amplitudeIcrement_dB))); //converting dB back to linear units
      Serial.println("SerialManager: decreased tone amplitude to " + String(20.f*log10f(myState.sine_amplitude)-3.0,1) + " dB re: FS (output)");
      break;
    case '1':
      Serial.println("SerialManager: outputing sine to left only");
      setOutputChan(State::OUT_LEFT);
      break;
    case '2':
      Serial.println("SerialManager: outputing sine to right only");
      setOutputChan(State::OUT_RIGHT);
      break;
    case '3':
      Serial.println("SerialManager: outputing sine to both left and right");
      setOutputChan(State::OUT_BOTH);
      break;
    case 'p':
      myState.flag_printInputLevelToUSB = true;
      Serial.println("SerialManager: enabled printing of the input signal levels");
      break;
    case 'P':
      myState.flag_printInputLevelToUSB = false;
      Serial.println("SerialManager: disabled printing of the input signal levels");
      break;
    case 'o':
      myState.flag_printOutputLevelToUSB = true;
      Serial.println("SerialManager: enabled printing of the output signal level");
      break;
    case 'O':
      myState.flag_printOutputLevelToUSB = false;
      Serial.println("SerialManager: disabled printing of the output signal level");
      break;
    case 'r':
      Serial.println("SerialManager: starting recording of input sginals to the SD card...");
      audioSDWriter.startRecording();
      break;
    case 's':
      Serial.println("SerialManager: stopping recording of input signals to the SD card...");
      audioSDWriter.stopRecording();
      break;
    default:
      Serial.println("SerialManager: command " + String(c) + " not recognized");
      break;
  }
  return ret_val;
}


#endif
