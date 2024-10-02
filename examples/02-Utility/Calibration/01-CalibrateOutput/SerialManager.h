
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables from the main *.ino file
extern Tympan myTympan;
extern float sine_freq_Hz;
extern float sine_amplitude;

//Extern Functions (that live in a file other than this file here)
extern float setFrequency_Hz(float freq_Hz);
extern float setAmplitude(float amplitude);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void) : SerialManagerBase() {};

    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    float frequencyIncrement_factor = pow(2.0,1.0/3.0);  //third octave steps
    float amplitudeIcrement_dB = 1.0;  //changes the amplitude of the synthetic sine wave
  private:
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("General: No Prefix");
  Serial.println("  h:     Print this help");
  Serial.println("  f/F:   Sine: Increase or decrease steady-tone frequency (current = " + String(sine_freq_Hz,1) + " Hz)");
  Serial.println("  a/A:   Sine: Increase or decrease sine amplitude (current = " + String(20*log10(sine_amplitude)-3.0,1) + " dB re: output FS = " + String(sine_amplitude,3) + " amplitude)");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'f':
      setFrequency_Hz(max(125.0/4,min(16000.0,sine_freq_Hz*frequencyIncrement_factor)));  //octave-based incrementing. Limit freuqencies to 31.25 Hz -> 16 kHz
      Serial.println("SerialManager: increased tone frequency to " + String(sine_freq_Hz) + " Hz");
      break;
    case 'F':
      setFrequency_Hz(max(125.0/4,min(16000.0,sine_freq_Hz/frequencyIncrement_factor)));  //octave-based incrementing. Limit freuqencies to 31.25 Hz -> 16 kHz
      Serial.println("SerialManager: decreased tone frequency to " + String(sine_freq_Hz) + " Hz");
      break;
    case 'a':
      setAmplitude(max(0.0,sine_amplitude*sqrt(pow(10.0,0.1*amplitudeIcrement_dB)))); //converting dB back to linear units
      Serial.println("SerialManager: increased tone amplitude to " + String(20.f*log10f(sine_amplitude)-3.0,1) + " dB re: output FS");
      break;
    case 'A':
      setAmplitude(max(0.0,sine_amplitude/sqrt(pow(10.0,0.1*amplitudeIcrement_dB)))); //converting dB back to linear units
      Serial.println("SerialManager: decreased tone amplitude to " + String(20.f*log10f(sine_amplitude)-3.0,1) + " dB re: output FS");
      break;
    default:
      Serial.println("SerialManager: command " + String(c) + " not recognized");
      break;
  }
  return ret_val;
}


#endif
