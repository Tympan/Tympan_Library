/*
*   Equalizer_8BandBiquad
*
*   Created: Chip Audette, OpenAudio, November 2021
*   Purpose: Make an 8-Band Equalizer (ie, change the loudness of 8 different frequency
*            bands) by using a bank of biquad filters).
*            
*   This is mono only.  It takes the audio from the left mic or left input,
*   processes it, and sends it out to both ears.
*   
*   You can control the per-channel gain from the SerialMonitor via USB Serial.
*   This example does not have support for the TympanRemote phone App.
*
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //24000 or 32000 or 44100 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// How many EQ bands do we want? see the function "setupEqualizer" for more settings
#define N_EQ_BANDS  8
int n_eq_bands = N_EQ_BANDS;

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E, audio_settings); //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Tympan's Audio Codec
AudioFilterbankBiquad_F32 filterbank(audio_settings); //this is a parallel set of filters to break up one audio stream into many
AudioEffectGain_F32       gainBlocks[N_EQ_BANDS];     //one gain block for each EQ band
AudioMixer16_F32          mixer(audio_settings);      //rejoin all the EQ bands.  Limited to 16 channels
AudioEffectGain_F32       gainOverall;                //use this as an overall volume control
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, filterbank, 0); //connect the Left input to the filterbank

AudioConnection_F32       patchCord11(filterbank,0,gainBlocks[0],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord12(filterbank,1,gainBlocks[1],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord13(filterbank,2,gainBlocks[2],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord14(filterbank,3,gainBlocks[3],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord15(filterbank,4,gainBlocks[4],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord16(filterbank,5,gainBlocks[5],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord17(filterbank,6,gainBlocks[6],0); //connect one EQ band to its gain block
AudioConnection_F32       patchCord18(filterbank,7,gainBlocks[7],0); //connect one EQ band to its gain block

AudioConnection_F32       patchCord31(gainBlocks[0],0,mixer,0);   //connect gain block back to mixer
AudioConnection_F32       patchCord32(gainBlocks[1],0,mixer,1);   //connect gain block back to mixer
AudioConnection_F32       patchCord33(gainBlocks[2],0,mixer,2);   //connect gain block back to mixer
AudioConnection_F32       patchCord34(gainBlocks[3],0,mixer,3);   //connect gain block back to mixer
AudioConnection_F32       patchCord35(gainBlocks[4],0,mixer,4);   //connect gain block back to mixer
AudioConnection_F32       patchCord36(gainBlocks[5],0,mixer,5);   //connect gain block back to mixer
AudioConnection_F32       patchCord37(gainBlocks[7],0,mixer,7);   //connect gain block back to mixer

AudioConnection_F32       patchCord51(mixer,0,gainOverall,0);     //connect gain block back to mixer
AudioConnection_F32       patchCord5(gainOverall, 0, i2s_out, 0); //connect to the Left output
AudioConnection_F32       patchCord6(gainOverall, 0, i2s_out, 1); //connect to the Right output

// Let's allow for control via serial commands over the USB serial
#include "SerialManager.h"
SerialManager serialManager;

//set the initial settings of the filterbank and gain
void setupEqualizer(void) {
  
  //choose cross-over frequencies between each EQ channel.  Being the value *between* filters, there are 
  //are always only N_Bands-1 of them!  So, for 8 filters, there are only 7 crossover frequencies!
  float crossFreq_Hz[N_EQ_BANDS-1]={125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0}; 

  //choose the filter order for the Biquad (IIR) filters used in the filterbank.  Max is N=6.
  //Smaller gives a more gentle transition and Higher gives a steeper transition.
  //For N=6, you get a 3rd order roll-off for the low transition and 3rd order roll-off for the high transition
  int filter_order = 6;  //Note, this has never been tested this with any value other than 6

  //design the filters!
  int ret_val = filterbank.designFilters(N_EQ_BANDS, filter_order, sample_rate_Hz, audio_block_samples, crossFreq_Hz);
  if (ret_val < 0) Serial.println("setupEqualizer: *** ERROR ***: the filters failed to be created.  Why??");

  //initialize the gain for each gain block
  for (int i=0; i<N_EQ_BANDS; i++) gainBlocks[i].setGain_dB(0.0); //set to zer gain
  gainBlocks[3].setGain_dB(-20.0);  // as an example, let's cut the 4th band (ie, band index 3) by 20 dB.
  
}

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000); //let's use the print functions in "myTympan" so it goes to BT, too!
  myTympan.println("Equalizer_8Band: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(30,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //configure the left and right compressors with the desired settings
  setupEqualizer();

  // check the setting on the potentiometer
  servicePotentiometer(millis(),0);

  // print the EQ settings
  printEqSettings();
  
  myTympan.println("Setup complete.");

  //print the serial help
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  if (Serial.available() > 0) serialManager.respondToByte((char)Serial.read());

  //periodically check the potentiometer
  servicePotentiometer(millis(),100); //update every 100 msec

  //periodically  print the CPU and Memory Usage
  //myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

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
      vol_knob_gain_dB = gainOverall.setGain_dB(vol_knob_gain_dB);  //set the overall loudness
      myTympan.print("servicePotentiometer: overall gain dB = "); myTympan.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


// /////////////////// Routines to help the user set/view the settings via the Serial Link

float incrementChannelGain(float increment_dB,int Ichan) {
  if ((Ichan < 0) || (Ichan >= N_EQ_BANDS)) return -999.9; //check for invalid inputs

  float old_val_dB = gainBlocks[Ichan].getGain_dB();
  float new_val_dB = gainBlocks[Ichan].setGain_dB(old_val_dB + increment_dB);
  return new_val_dB;
}


//print out the gain for each eq channel
void printEqSettings(void) {
  Serial.println("Equalizer Settings, Number of Bands = " + String(N_EQ_BANDS));

  for (int i=0; i<N_EQ_BANDS; i++) { //loop over each EQ band
    
    //print the channel number
    Serial.print(" Chan " + String(i+1) + ": ");

    //print the frequency bounds
    if (i==0) {
      //this is the lowpass filter
       Serial.print(" Below " + String(filterbank.state.get_crossover_freq_Hz(0)) + " Hz: ");
    } else if (i < (N_EQ_BANDS-1)) {
      //this is one of the bandpass filters
      Serial.print(" " + String(filterbank.state.get_crossover_freq_Hz(i-1)) + " to ");
      Serial.print(String(filterbank.state.get_crossover_freq_Hz(i)) + " Hz: ");
    } else {
      //this is the highpass filter
      Serial.print(" Above " + String(filterbank.state.get_crossover_freq_Hz(i-1)) + " Hz: ");
    }

    //print the gain value
    Serial.print(String(gainBlocks[i].getGain_dB()) + " dB");
    Serial.println(); //this line is finished!
    
  } //end the loop over EQ bands
}
