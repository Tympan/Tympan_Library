/*
*   BasicGain_wAFC (NLMS algorithm)
*
*   CREATED: Chip Audette, OpenAudio, Sept 2023
*   PURPOSE: Process audio by applying gain to the audio.  The processing is then
*      wrapped in an adaptive feedback cancellation (AFC), the NLMS algorithm.
*      
*      * The AFC algorithm is from Boys Town National Research Hospital (BTNRH) via 
*        their CHAPRO library of hearing aid algorithms.  See their documentation in
*        this PDF: https://github.com/BoysTownOrg/chapro/blob/master/chapro.pdf, though
*        my implementation here might be out-of-date relative to up-to-date document
*        linked above.
*        
*      * You can adjust the AFC parameters via the SerialMonitor.  Send an 'h' (no quotes)
*        to ge the help menu.  Have fun!
*
*      * The volume wheel on the Tympan adjusts the digital gain applied to the
*        processed audio
*   
*   MIC AND SPEAKER: This example has not been written for the Tympan earpieces.  It
*      assumes that you are using your own microphone and headphones.  This AFC 
*      algorithm assumes, however, that the distance between the mic and speakers is
*      very close (like ~1cm), like in a behind-the-ear (BTE) hearing aid.  If your
*      own mic and earpice are seperated by more than this distance, the model at the 
*      heart of the AFC algorithm is probably not long enough to capture the longer 
*      acoustic travel time.  So, you will need to lengthen the AFC model or move the 
*      mic and speaker closer together.
*      
*   TYMPAN HARDWARE: This example has only been tested on Tympan RevE.  Older Tympan devices
*   might not be fast enough. 
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //Choose any sample rate that you would like, though the AFC length is suitable for 24kHz
const int audio_block_samples = 32;     //Shorter results in less latency.  Longer is more CPU efficient (but no bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                      myTympan(TympanRev::E,audio_settings);   //only tested on Tympan RevE
AudioInputI2S_F32           i2s_in(audio_settings);                  //Digital audio *from* the Tympan AIC. 
AudioFeedbackCancelNLMS_F32 afc(audio_settings);                     //adaptive feedback cancelation (AFC), NLMS method
AudioEffectGain_F32         gain1(audio_settings);                   //Applies digital gain to audio data.
AudioOutputI2S_F32          i2s_out(audio_settings);                 //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioLoopBack_F32           afc_loopback(audio_settings);            //here's how we close the loop on the AFC

//Make all of the audio connections
AudioConnection_F32       patchCord10(i2s_in, 0, afc, 0);          //connect the left input straight to the afc
AudioConnection_F32       patchCord20(afc,    0, gain1, 0);        //connect to the AFC to your audio processing
AudioConnection_F32       patchCord30(gain1,  0, i2s_out, 0);      //output to the Left output
AudioConnection_F32       patchCord31(gain1,  0, i2s_out, 1);      //output to the Right output
AudioConnection_F32       patchCord40(gain1,  0, afc_loopback, 0); //close the loop with the AFC

// Setup the serial mananger (for controlling your algorithms in real-time)
#include "SerialManager.h"
SerialManager serialManager;

// some functions
void setupAFC(void) {
  float mu = 1.E-3;  //AFC step size.
  float rho = 0.9;   //AFC forgetting factor.  Must be < 1.0.  Neely 2018-05-07 said try 0.9
  float eps = 0.008; //AFC tolerance for setting a floor on the smallest signal level (thereby avoiding divide-by-near-zero) Neely 2018-05-07 said try 0.008
  int afl = 100;     //AFC filter (model) length.  Neely set 100 for fs = 24kHz.  Value must be <= 256, though AudioEffectFeedbackCancel_F32 could be changed to allow this to be bigger! 
  afc.setParams(mu, rho, eps, afl); //you can also set invidivual values via setMu(), setRho(), etc
}

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
bool enable_printCPUandMemory = false;
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(500); //let's use the print functions in "myTympan" so it goes to BT, too!
  myTympan.println("BasicGain_wAFC: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //connect the afc_loopback to the afc (do this before audio starts flowing (ie before myTympan.enable() ??)
  afc_loopback.setTarget(&afc);

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself.  HP filtering is very important for AFC!!!
  float cutoff_Hz = 100.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  // setup the AFC
  setupAFC();
  afc.setEnable(true);  //set to false to disable AFC

  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  myTympan.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //handle any in-coming serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB

  //periodicallly check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //periodically print the CPU and Memory Usage
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //Blink the LEDs!
  myTympan.serviceLEDs(millis());   //defaults to a slow toggle (see Tympan.h and Tympan.cpp)

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      float gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB
      setVolKnobGain_dB(gain_dB);
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


// //////////////////////////// Functions for controlling (especially from SerialManager)
float incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as myTympan.Manager.h
  return setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

float setVolKnobGain_dB(float gain_dB) {
  vol_knob_gain_dB = gain1.setGain_dB(gain_dB);
  printGainSettings();
  return vol_knob_gain_dB;
}

void printGainSettings(void) {
  Serial.print("Gains: ");
  Serial.print(" Input (dB) = " + String(input_gain_dB));
  Serial.print(", Digital (dB) = " + String(vol_knob_gain_dB));
  Serial.println();
}
