/*
*   DetectExtMic
*
*   Created: Chip Audette, April 2018
*   Purpose: Process audio by applying gain.
*
*   Blue potentiometer adjusts the digital gain applied to the audio signal.
*   Built on example "BasicGain.ino".
*   Adds code to detect if external mic is inserted into jack.
*   Automatically switches the input to the external mic (and back) as it is plugged or unplugged.
*
*   Uses Teensy Audio default sample rate of 44100 Hz with Block Size of 128
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F);   //do TympanRev::D or E or F
AudioInputI2S_F32         i2s_in;        //Digital audio *from* the Tympan AIC.
AudioEffectGain_F32       gain1, gain2;  //Applies digital gain to audio data.
AudioOutputI2S_F32        i2s_out;       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gain1, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gain2, 0);    //connect the Right input
AudioConnection_F32       patchCord11(gain1, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(gain2, 0, i2s_out, 1);  //connect the Right gain to the Right output

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000); //both Serial (USB) and Serial1 (BT) are started here
  myTympan.println("DetectExtMic: starting setup()..."); //prints to both Serial destinations

  //allocate the audio memory
  AudioMemory_F32(20); //allocate audio memory

 //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //configure for mic detection
  myTympan.enableMicDetect(true);

  //Choose the desired input
  //myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  // check the volume knob and whether anything is plugged into the mic jack
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.
  serviceMicDetect(millis(),0);

  myTympan.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //check the mic_detect signal
  serviceMicDetect(millis(),500);

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
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      gain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      gain2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel gain processor
      myTympan.print("servicePotentiometer: Digital Gain dB = "); myTympan.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111;
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = myTympan.updateInputBasedOnMicDetect();
    if (cur_val != prev_val) {
      if (cur_val) {
        myTympan.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        myTympan.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
      }
    }
    prev_val = cur_val;
    lastUpdate_millis = curTime_millis;
  }
}
