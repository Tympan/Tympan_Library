/*
  WDRC_SingleBand

  Created: Chip Audette (OpenAudio), Feb 2017
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements BTNRH's Wide Dynamic Range Compressor, though
    only in a single frequency band.  I've also added an expansion stage
    to manage noise at very low SPL.

  User Controls:
    Potentiometer on Tympan controls the algorithm gain

   MIT License.  use at your own risk.
*/

// Include the required libraries
#include <Tympan_Library.h>


//create audio library objects for handling the audio
Tympan						      myTympan(TympanRev::D);  //TympanRev::D or TympanRev::C
AudioInputI2S_F32       i2s_in;
AudioFilterBiquad_F32   iir1; 
AudioEffectCompWDRC_F32  compWDRC1;   
AudioOutputI2S_F32       i2s_out; 
AudioConnection_F32     patchCord1(i2s_in, 0, iir1, 0);
AudioConnection_F32     patchCord2(iir1, compWDRC1);
AudioConnection_F32     patchCord3(compWDRC1, 0, i2s_out, 0);
AudioConnection_F32     patchCord4(compWDRC1, 0, i2s_out, 1);


//define a function to setup the Teensy Audio Board how I like it
void setupTympanHardware(void) {
  // Setup the Tympan audio hardware
  myTympan.enable(); // activate AIC

  // Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones // default
  //  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  //  myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line in pads on the TYMPAN board - defaults to mic bias OFF

  //Adjust the MIC bias, if using TYMPAN_INPUT_JACK_AS_MIC
  //  myTympan.setMicBias(TYMPAN_MIC_BIAS_OFF); // Turn mic bias off
  myTympan.setMicBias(TYMPAN_MIC_BIAS_2_5); // set mic bias to 2.5 // default

  // VOLUMES
  myTympan.volume_dB(0.0);  // -63.6 to +24 dB in 0.5dB steps.  uses float
  myTympan.setInputGain_dB(10.0); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}

//The setup function is called once when the system starts up
void setup(void) {
    //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000); //let's use the print functions in "myTympan" so it goes to BT, too!
  myTympan.println("Setup starting...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20); 

  //setup high-pass IIR...[b,a]=butter(2,750/(44100/2),'high')
  float32_t hp_b[]={ 0.927221242739230,  -1.854442485478460,   0.927221242739230};
  float32_t hp_a[]={ 1.000000000000000,  -1.849138705449389,   0.859746265507531};
  iir1.setFilterCoeff_Matlab(hp_b, hp_a); //one stage of N=2 IIR

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //End of setup
  myTympan.println("Setup complete.");
};


//After setup(), the loop function loops forever.
//Note that the audio modules are called in the background.
//They do not need to be serviced by the loop() function.
void loop(void) {

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis(),100); //update every 100msec

  //update the memory and CPU usage...if enough time has passed
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

};


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis,unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      float min_mic_dB = +10.0f;
      float fixed_hp_dB = 00.0f;
      float vol_dB = min_mic_dB +10.0f + 20.0f * ((val - 0.5) * 2.0); //set volume as 10dB +/- 25 dB
      myTympan.print("Changing input gain = "); myTympan.print(vol_dB); myTympan.println(" dB");
      if (vol_dB  < min_mic_dB) {
        myTympan.setInputGain_dB(min_mic_dB);
        myTympan.volume_dB(vol_dB-min_mic_dB+fixed_hp_dB);
      } else {
        myTympan.setInputGain_dB(vol_dB);
        myTympan.volume_dB(fixed_hp_dB);
      }
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
