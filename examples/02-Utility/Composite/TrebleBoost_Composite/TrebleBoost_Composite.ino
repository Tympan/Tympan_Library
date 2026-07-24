/*
*   TrebleBoost_Composite
*
*   Created: Chip Audette, OpenAudio, July 2026
*   Purpose: Demonstrate how to make a "composite" audio processing class that joins several
*            smaller processing steps into one class.  This makes it easier to create copies
*            of a complex audio processing chain, such as for performing the same operations
*            on both the left and right ears.  In this case, the composite class will apply 
*            a high-pass filter followed by gain.  Obviously, one could setup much more
*            complicated processing, if desired.
*
*   This example is built upon the simpler Tympan example: Basic/TrebleBoost.ino
*
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  
#include "AudioFilterGain_F32.h"  //here is the new composite audio class that we've created

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F, audio_settings);   //do TympanRev::D or E or F
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio *from* the Tympan AIC. 
AudioFilterGain_F32       filterGain1(audio_settings), filterGain2(audio_settings);  // the new composite class.  one left, one right.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, filterGain1, 0);  //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, filterGain2, 0);  //connect the Right input
AudioConnection_F32       patchCord3(filterGain1, 0, i2s_out, 0);  //connect the Left output
AudioConnection_F32       patchCord4(filterGain2, 0, i2s_out, 1);  //connect the Right Output


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000); //let's use the print functions in "myTympan" so it goes to BT, too!
  myTympan.println("TrebleBoost: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the highpassfilter
  float cutoff_Hz = 1000.f;  //frequencies below this will be attenuated
  myTympan.print("Highpass filter cutoff at ");myTympan.print(cutoff_Hz);myTympan.println(" Hz");
  filterGain1.setCutoff_Hz(cutoff_Hz); //biquad IIR filter.  left channel
  filterGain2.setCutoff_Hz(cutoff_Hz); //biquad IIR filter.  right channel

  // check the volume knob
  servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  myTympan.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //periodicallly check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //periodically print the CPU and Memory Usage
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

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
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      filterGain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      filterGain2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel gain processor
      myTympan.print("servicePotentiometer: Digital Gain dB = "); myTympan.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
