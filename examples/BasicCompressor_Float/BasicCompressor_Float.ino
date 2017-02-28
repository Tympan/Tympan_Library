/*
   BasicCompressor_Float

   Created: Chip Audette, Dec 2016 - Jan 2017
   Purpose: Process audio by applying a single-band compressor
            Demonstrates audio processing using floating point data type.

   Uses Tympan Audio Adapter.
   Blue potentiometer adjusts the gain.
   
   MIT License.  use at your own risk.
*/

//These are the includes from the Teensy Audio Library
#include <Audio.h>   //Teensy Audio Librarya
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <Tympan_Library.h> 

//create audio library objects for handling the audio
AudioControlTLV320AIC3206 tlv320aic3206_1;
AudioInputI2S_F32         i2s_in;        //Digital audio *from* the Teensy Audio Board ADC.
AudioOutputI2S_F32        i2s_out;       //Digital audio *to* the Teensy Audio Board DAC.
AudioEffectCompressor_F32 comp1, comp2;

//Make all of the audio connections, with the option of USB audio in and out
AudioConnection_F32       patchCord1(i2s_in, 0, comp1, 0);   //Left.
AudioConnection_F32       patchCord2(i2s_in, 1, comp2, 0);   //Right.
AudioConnection_F32       patchCord20(comp1, 0, i2s_out, 0);  //Left.
AudioConnection_F32       patchCord21(comp2, 0, i2s_out, 1);  //Right.


//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

//define a function to setup the Teensy Audio Board how I like it
void setupMyAudioBoard(void) {
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
}

//define a function to configure the left and right compressors
void setupMyCompressors(boolean use_HP_filter, float knee_dBFS, float comp_ratio, float attack_sec, float release_sec) {
  comp1.enableHPFilter(use_HP_filter);   comp2.enableHPFilter(use_HP_filter);
  comp1.setThresh_dBFS(knee_dBFS);       comp2.setThresh_dBFS(knee_dBFS);
  comp1.setCompressionRatio(comp_ratio); comp2.setCompressionRatio(comp_ratio);

  float fs_Hz = AUDIO_SAMPLE_RATE;
  comp1.setAttack_sec(attack_sec, fs_Hz);       comp2.setAttack_sec(attack_sec, fs_Hz);
  comp1.setRelease_sec(release_sec, fs_Hz);     comp2.setRelease_sec(release_sec, fs_Hz);
}

// define the overall setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //open the USB serial link to enable debugging messages
  delay(500);             //give the computer's USB serial system a moment to catch up.
  Serial.println("Tympan_Library: BasicCompressor_Float..."); //identify myself over the USB serial

  // Audio connections require memory, and the record queue uses this memory to buffer incoming audio.
  AudioMemory(10);  //allocate Int16 audio data blocks
  AudioMemory_F32(10); //allocate Float32 audio data blocks

  // Enable the audio shield, select input, and enable output
  setupMyAudioBoard();

  //choose the compressor parameters...note that preGain is set by the potentiometer in the main loop()
  boolean use_HP_filter = true; //enable the software HP filter to get rid of DC?
  float knee_dBFS, comp_ratio, attack_sec, release_sec;
  if (false) {
      Serial.println("Configuring Compressor for fast response for use as a limitter.");
      knee_dBFS = -15.0f; comp_ratio = 5.0f;  attack_sec = 0.005f;  release_sec = 0.200f;
  } else {
      Serial.println("Configuring Compressor for slow response more like an automatic volume control.");
      knee_dBFS = -50.0; comp_ratio = 5.0;  attack_sec = 1.0;  release_sec = 2.0;
  }

  //configure the left and right compressors with the desired settings
  setupMyCompressors(use_HP_filter, knee_dBFS, comp_ratio, attack_sec, release_sec);

  // setup any other other features
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
unsigned long updatePeriod_millis = 100; //how many milliseconds between updating gain reading?
unsigned long lastUpdate_millis = 0;
unsigned long curTime_millis = 0;
int prev_gain_dB = 0;
unsigned long lastMemUpdate_millis = 0;
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //has enough time passed to try updating the GUI?
  curTime_millis = millis(); //what time is it right now
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float32_t val = float(analogRead(POT_PIN)) / 1024.0f; //0.0 to 1.0
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter
    val = 1.0 - val;  //correct for the pot being wired backwards on the Tympan Audio Board.  Sorry!

    //compute desired digital gain
    const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
    float gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB) * val; //computed desired gain value in dB

    //if the gain is different than before, set the new gain value
    if (abs(gain_dB - prev_gain_dB) > 1.0) { //is it different than before
      prev_gain_dB = gain_dB;  //we will use this value the next time around
      
      //gain_dB = 0.0; //force to 0 dB for debugging
      comp1.setPreGain_dB(gain_dB);  //set the gain of the Left-channel gain processor
      comp2.setPreGain_dB(gain_dB);  //set the gain of the Right-channel gain processor
      Serial.print("Setting Digital Pre-Gain dB = "); Serial.println(gain_dB); //print text to Serial port for debugging
    }

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  } // end if


  //print status information to the Serial port
  if ((curTime_millis - lastMemUpdate_millis) > 2000) {  // print a summary of the current & maximum usage
    //printCompressorState(&Serial);
    printCPUandMemoryUsage(&Serial);
    lastMemUpdate_millis = curTime_millis; //we will use this value the next time around.
  }

} //end loop();

void printCompressorState(Stream *s) {
  s->print("Current Compressor: Pre-Gain (dB) = ");
  s->print(comp1.getPreGain_dB());
  s->print(", Level (dBFS) = ");
  s->print(comp1.getCurrentLevel_dBFS());
  s->print(", ");
  s->print(comp2.getCurrentLevel_dBFS());
  s->print(", Dynamic Gain L/R (dB) = ");
  s->print(comp1.getCurrentGain_dB());
  s->print(", ");
  s->print(comp2.getCurrentGain_dB());
  s->println();
};

void printCPUandMemoryUsage(Stream *s) {
  s->print("Usage/Max: ");
  s->print("comp1 CPU = "); s->print(comp1.processorUsage()); s->print("/"); s->print(comp1.processorUsageMax()); s->print(", ");
  s->print("all CPU = " ); s->print(AudioProcessorUsage()); s->print("/");  s->print(AudioProcessorUsageMax()); s->print(", ");
  s->print("Int16 Mem = "); s->print(AudioMemoryUsage()); s->print("/"); s->print(AudioMemoryUsageMax()); s->print(", ");
  s->print("Float Mem = "); s->print(AudioMemoryUsage_F32()); s->print("/"); s->print(AudioMemoryUsageMax_F32()); s->print(", ");
  s->println();
};
