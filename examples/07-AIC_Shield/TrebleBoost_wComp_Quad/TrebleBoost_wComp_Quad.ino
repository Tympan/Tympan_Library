/*
*   TrebleBoost_wComp
*
*   Created: Chip Audette, OpenAudio, Apr 2017 (updated August 2021 for Quad)
*   Purpose: Process audio by applying a high-pass filter followed by gain followed
*            by a dynamic range compressor.
*            
*   Uses a Tympan RevD or RevE along with an AICShield (or earpiece shield).  Defaults
*   to using the microphones built into the Tympan's circuit board.  It applies the
*   Treble Boost and Compression to these two channels.  It sends the processed audio
*   to the stereo headphone output of both the Tympan board and the AIC Shield.
*
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*   
*   No USB Serial control nor any Bluetooth control.  There is information sent to the 
*   USB Serial Monitor, however, so you can see what is happening.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44100 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan       myTympan(TympanRev::E, audio_settings);   //do TympanRev::D or TympanRev::E
AICShield    aicShield(TympanRev::E, AICShieldRev::A); //do TympanRev::D or TympanRev::E


AudioInputI2SQuad_F32     i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32     hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectCompWDRC_F32   comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.
AudioEffectCompWDRC_F32   comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right.
AudioOutputI2SQuad_F32    i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
#if 1
  //Use the inputs on the Tympan board
  AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input to the left highpass filter
  AudioConnection_F32       patchCord2(i2s_in, 1, hp_filt2, 0);   //connect the Right input to the right highpass filter
#else
  //Use the inputs on the AIC Shield
  AudioConnection_F32       patchCord1(i2s_in, 2, hp_filt1, 0);   //connect the Left input to the left highpass filter
  AudioConnection_F32       patchCord2(i2s_in, 3, hp_filt2, 0);   //connect the Right input to the right highpass filter
#endif

//connect the compressors
AudioConnection_F32       patchCord3(hp_filt1, 0, comp1, 0);    //connect to the left gain/compressor/limiter
AudioConnection_F32       patchCord4(hp_filt2, 0, comp2, 0);    //connect to the right gain/compressor/limiter

//connect to the Tympan headphone outputs
AudioConnection_F32       patchCord5(comp1, 0, i2s_out, 0);     //connect to the Left output
AudioConnection_F32       patchCord6(comp2, 0, i2s_out, 1);     //connect to the Right output

//connect to the AIC Shield headphone outputs
AudioConnection_F32       patchCord7(comp1, 0, i2s_out, 2);     //copy to left output on AIC Shield
AudioConnection_F32       patchCord8(comp2, 0, i2s_out, 3);     //copy to right output on AIC Shield

//define a function to configure the left and right compressors
void setupMyCompressors(void) {
  //set the speed of the compressor's response
  float attack_msec = 5.0;
  float release_msec = 300.0;
  comp1.setAttackRelease_msec(attack_msec, release_msec); //left channel
  comp2.setAttackRelease_msec(attack_msec, release_msec); //right channel

  //Single point system calibration.  what is the SPL of a full scale input (including effect of input_gain_dB)?
  float SPL_of_full_scale_input = 115.0;
  comp1.setMaxdB(SPL_of_full_scale_input);  //left channel
  comp2.setMaxdB(SPL_of_full_scale_input);  //right channel

  //now define the compression parameters
  float knee_compressor_dBSPL = 55.0;  //follows setMaxdB() above
  float comp_ratio = 1.5;
  comp1.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //left channel
  comp2.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //right channel
  comp1.setCompRatio(comp_ratio); //left channel
  comp2.setCompRatio(comp_ratio);

  //finally, the WDRC module includes a limiter at the end.  set its parameters
  float knee_limiter_dBSPL = SPL_of_full_scale_input - 20.0;  //follows setMaxdB() above
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp1.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //left channel
  comp2.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //right channel

}


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000); //let's use the print functions in "myTympan" so it goes to BT, too!
  myTympan.println("TrebleBoost_wComp_Quad: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
  aicShield.enable();

  //Choose the desired input on main Tympan board
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Choose the desired input on the AIC shield (though there are no software AudioConnections to it)
  //aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF


  //Set the desired volume levels
  myTympan.volume_dB(0);                    // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  aicShield.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB);  // set input volume, 0-47.5dB in 0.5dB setps
  aicShield.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps (not being used
  

  //Set the cutoff frequency for the highpassfilter
  float cutoff_Hz = 1000.f;  //frequencies below this will be attenuated
  myTympan.print("Highpass filter cutoff at ");myTympan.print(cutoff_Hz);myTympan.println(" Hz");
  hp_filt1.setHighpass(0, cutoff_Hz); //biquad IIR filter.  left channel
  hp_filt2.setHighpass(0, cutoff_Hz); //biquad IIR filter.  right channel

  //configure the left and right compressors with the desired settings
  setupMyCompressors();

  // check the setting on the potentiometer
  servicePotentiometer(millis(),0);

  myTympan.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //periodically check the potentiometer
  servicePotentiometer(millis(),100); //update every 100 msec

  //periodically  print the CPU and Memory Usage
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //periodically print the gain status
  printGainStatus(millis(),2000); //update every 4000 msec

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
      comp1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel linear gain at start of compression
      comp2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel linear gain at start of compression
      myTympan.print("servicePotentiometer: linear gain dB = "); myTympan.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



//This routine plots the current gain settings, including the dynamically changing gains
//of the compressors
void printGainStatus(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    myTympan.print("printGainStatus: ");

    myTympan.print("Input PGA = ");
    myTympan.print(input_gain_dB,1);
    myTympan.print(" dB.");

    myTympan.print(" Compressor Gain (L/R) = ");
    myTympan.print(comp1.getCurrentGain_dB(),1);
    myTympan.print(", ");
    myTympan.print(comp2.getCurrentGain_dB(),1);
    myTympan.print(" dB.");

    myTympan.println();

    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
