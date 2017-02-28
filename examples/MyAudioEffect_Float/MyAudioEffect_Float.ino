/*
   MyAudioEffect_Float

   Created: Chip Audette, Dec 2016 - Feb 2017
   Purpose: Be a blank canvas for adding your own floating-point audio processing.

   Uses Tympan Audio Adapter.

   MIT License.  use at your own risk.
*/

//These are the includes from the Teensy Audio Library
#include <Audio.h>      //Teensy Audio Library
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <OpenAudio_ArduinoLibrary.h> 
#include "AudioEffectMine_F32.h"

//create audio library objects for handling the audio
AudioControlTLV320AIC3206       tlv320aic3206_1;
AudioInputI2S_F32           i2s_in;        //Digital audio *from* the Teensy Audio Board ADC.
AudioOutputI2S_F32          i2s_out;       //Digital audio *to* the Teensy Audio Board DAC.
AudioEffectMine_F32     effect1, effect2;  //This is your own algorithms

//Make all of the audio connections
AudioConnection_F32         patchCord1(i2s_in, 0, effect1, 0);    //connect the Left input to the effect
AudioConnection_F32         patchCord2(i2s_in, 1, effect2a, 0);    //connect the Right input to the effect
AudioConnection_F32         patchCord11(effect1, 0, i2s_out, 0);  //connect the effect to the Left output
AudioConnection_F32         patchCord12(effect2, 0, i2s_out, 1);  //connect the effect to the Right output


//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //open the USB serial link to enable debugging messages
  delay(500);             //give the computer's USB serial system a moment to catch up.
  Serial.println("Tympan_Library: MyAudioEffect_Float...");

  // Audio connections require memory
  AudioMemory(10);      //allocate Int16 audio data blocks
  AudioMemory_F32(10);  //allocate Float32 audio data blocks

  // Setup the TLV320
  tlv320aic3206_1.enable(); // activate AIC

  // Choose the desired input
  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones // default
  //  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  //  tlv320aic3206_1.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line in pads on the TYMPAN board - defaults to mic bias OFF

  //Adjust the MIC bias, if using TYMPAN_INPUT_JACK_AS_MIC
  //  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_OFF); // Turn mic bias off
  tlv320aic3206_1.setMicBias(TYMPAN_MIC_BIAS_2_5); // set mic bias to 2.5 // default


  // VOLUMES
  tlv320aic3206_1.volume_dB(10);  // -63.6 to +24 dB in 0.5dB steps.  uses float
  tlv320aic3206_1.setInputGain_dB(10); // set MICPGA volume, 0-47.5dB in 0.5dB setps

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis());

  //update the memory and CPU usage...if enough time has passed
  printMemoryAndCPU(millis());
} //end loop()


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(analogRead(POT_PIN)) / 1024.0; //0.0 to 1.0
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter

    //add code here to change the potentiometer value to something useful (like gain_dB?)
    // ..... add code here if you'd like ......

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      Serial.print("Sending new value to my algorithms: "); Serial.println(val);
      effect1.setUserParameter(val);   effect2.setUserParameter(val);
    }
    prev_val = val;  //use the value the next time around
  } // end if
} //end servicePotentiometer();


void printMemoryAndCPU(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("CPU: Usage, Max: ");
    Serial.print(AudioProcessorUsage());
    Serial.print(", ");
    Serial.print(AudioProcessorUsageMax());
    Serial.print("    ");
    Serial.print("Int16 Memory: ");
    Serial.print(AudioMemoryUsage());
    Serial.print(", ");
    Serial.print(AudioMemoryUsageMax());
    Serial.print("    ");
    Serial.print("Float Memory: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print(", ");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}