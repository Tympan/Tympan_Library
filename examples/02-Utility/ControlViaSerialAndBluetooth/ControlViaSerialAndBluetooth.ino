/*
*   ControlViaSerialAndBluetooth
*
*   Created: Chip Audette, OpenAudio, Apr 2017
*   Purpose: Shows how system parameters can be controlled via text commands
*      from the Arduino Serial Monitor (wired USB) or via Bluetooth from a PC
*      or from a mobile phone or tablet.  The Tympan has a Bluetooth Classic
*      module, so you have to manually connect to it from your PC or phone.
*
*   Algorithm: The algorithm being executed in this example is the same as the
*      "TrebleBoost" algorithm shown in the "Basic" collection of examples.
*
*   Uses Tympan Audio Adapter.
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
//include <Audio.h>           //include the Teensy Audio Library
#include <Tympan_Library.h>  //include the Tympan Library

#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan audioHardware(TympanRev::D); //TympanRev::D or TympanRev::C
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.
AudioEffectGain_F32       gain1;                      //Applies digital gain to audio data.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input
AudioConnection_F32       patchCord3(hp_filt1, 0, gain1, 0);    //gain
AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 0);     //connect the Left gain to the Left output
AudioConnection_F32       patchCord6(gain1, 0, i2s_out, 1);     //connect the Right gain to the Right output


//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager_USB(&Serial); //this instance will handle the USB hard-wired serial link

#define BT_SERIAL Serial1
SerialManager serialManager_BT(&BT_SERIAL); //this instance will handle the Bluetooth Serial link


//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A20  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
float hp_cutoff_Hz = 1000.f;
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  BT_SERIAL.begin(115200);
  delay(500);
  Serial.println("ControlViaSerial (TrebleBoost): Starting setup()...");
  BT_SERIAL.println("ControlViaSerial (TrebleBoost): Starting setup()...");

  //allocate the audio memory
  AudioMemory(10); AudioMemory_F32(10,audio_settings); //allocate both kinds of memory

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC

  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the highpassfilter
  setHPCutoffFreq_Hz(hp_cutoff_Hz);  //defined towards the bottom of this file

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  //End of setup
  Serial.println("Setup complete.");BT_SERIAL.println("Setup complete.");
  serialManager_USB.printHelp(); serialManager_BT.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands, if any have been received
  while (Serial.available()) serialManager_USB.respondToByte((char)Serial.read());
  while (BT_SERIAL.available()) serialManager_BT.respondToByte((char)BT_SERIAL.read());

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis(),100);  //update every 100 msec

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) printCPUandMemory(millis(),3000); //update every 3000 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      Serial.print("servicePotentiometer: Digital Gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
      BT_SERIAL.print("servicePotentiometer: Digital Gain dB = "); BT_SERIAL.println(vol_knob_gain_dB); //print text to Serial port for debugging
      setVolKnobGain_dB(vol_knob_gain_dB);  //set the gain
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage(&Serial);  //USB Serial
    printCPUandMemoryMessage(&BT_SERIAL); //Bluetooth Serial
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printCPUandMemoryMessage(Stream *s)  {
  s->print("printCPUandMemory: ");
  s->print("CPU Cur/Peak: ");
  s->print(audio_settings.processorUsage());
  //s->print(AudioProcessorUsage()); //if not using AudioSettings_F32
  s->print("%/");
  s->print(audio_settings.processorUsageMax());
  //s->print(AudioProcessorUsageMax());  //if not using AudioSettings_F32
  s->print("%,   ");
  s->print("Dyn MEM Int16 Cur/Peak: ");
  s->print(AudioMemoryUsage());
  s->print("/");
  s->print(AudioMemoryUsageMax());
  s->print(",   ");
  s->print("Dyn MEM Float32 Cur/Peak: ");
  s->print(AudioMemoryUsage_F32());
  s->print("/");
  s->print(AudioMemoryUsageMax_F32());
  s->println();
}

//here's a function to print the current gain settings.   We'll also invoke it from our serialManager
 void printGainSettings(void) {
  printGainSettings(&Serial);  //USB
  printGainSettings(&BT_SERIAL);  //Bluetooth
 }
 void printGainSettings(Stream *s) {
  s->print("printGainSettings (dB): ");
  s->print("Vol Knob = "); s->print(vol_knob_gain_dB,1);
  s->print(", Input PGA = "); s->print(input_gain_dB,1);
  s->println();
}

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}
void setVolKnobGain_dB(float gain_dB) {
    gain1.setGain_dB(gain_dB);
    vol_knob_gain_dB = gain_dB;
    printGainSettings();
}

//here's a function to change the highpass filter cutoff.  We'll also invoke it from our serialManager
extern void incrementHPCutoffFreq_Hz(float increment_frac) {
  setHPCutoffFreq_Hz(increment_frac*hp_cutoff_Hz);
}
void setHPCutoffFreq_Hz(float cutoff_Hz) {
  float min_allowed_Hz = 62.5f, max_allowed_Hz = 8000.f;
  hp_cutoff_Hz = max(min_allowed_Hz,min(max_allowed_Hz, cutoff_Hz));

  Serial.print("Setting highpass filter cutoff to ");Serial.print(hp_cutoff_Hz);Serial.println(" Hz");
  BT_SERIAL.print("Setting highpass filter cutoff to ");BT_SERIAL.print(hp_cutoff_Hz);BT_SERIAL.println(" Hz");
  hp_filt1.setHighpass(0, hp_cutoff_Hz); //biquad IIR filter.
}
