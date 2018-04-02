#include <Tympan_Library.h>
//include <Audio.h>
//include <Wire.h>
#include <SPI.h>
//include <SD.h>
//include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S_F32        i2sAudioIn1;    //xy=136,112
AudioFilterBiquad_F32       iir1;           //xy=233,172
AudioEffectCompWDRC_F32  compWDRC1;      //xy=427,172
AudioOutputI2S_F32       i2sAudioOut1;   //xy=630,174
AudioConnection_F32         patchCord1(i2sAudioIn1, 0, iir1, 0);
AudioConnection_F32         patchCord2(iir1, compWDRC1);
AudioConnection_F32         patchCord3(compWDRC1, 0, i2sAudioOut1, 0);
AudioConnection_F32         patchCord4(compWDRC1, 0, i2sAudioOut1, 1);
AudioControlTLV320AIC3206 tlv320aic3206_1; //xy=136,38
// GUItool: end automatically generated code

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
  tlv320aic3206_1.volume_dB(0);  // -63.6 to +24 dB in 0.5dB steps.  uses float
  tlv320aic3206_1.setInputGain_dB(10); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}

//The setup function is called once when the system starts up
void setup(void) {
  //Start the USB serial link (to enable debugging)
  Serial.begin(115200); delay(500);
  Serial.println("Setup starting...");

  //Allocate dynamically shuffled memory for the audio subsystem
  AudioMemory(10);  AudioMemory_F32(10);

  //setup high-pass IIR...[b,a]=butter(2,750/(44100/2),'high')
  float32_t hp_b[]={ 0.927221242739230,  -1.854442485478460,   0.927221242739230};
  float32_t hp_a[]={ 1.000000000000000,  -1.849138705449389,   0.859746265507531};
  iir1.setFilterCoeff_Matlab(hp_b, hp_a); //one stage of N=2 IIR

  // Enable the audio shield, select input, and enable output
  setupMyAudioBoard();

  //End of setup
  Serial.println("Setup complete.");
};


//After setup(), the loop function loops forever.
//Note that the audio modules are called in the background.
//They do not need to be serviced by the loop() function.
void loop(void) {

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis());

};


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
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      float min_mic_dB = +10.0f;
      float fixed_hp_dB = 00.0f;
      float vol_dB = min_mic_dB +10.0f + 20.0f * ((val - 0.5) * 2.0); //set volume as 10dB +/- 25 dB
      Serial.print("Changing input gain = "); Serial.print(vol_dB); Serial.println(" dB");
      if (vol_dB  < min_mic_dB) {
        tlv320aic3206_1.setInputGain_dB(min_mic_dB);
        tlv320aic3206_1.volume_dB(vol_dB-min_mic_dB+fixed_hp_dB);
      } else {
        tlv320aic3206_1.setInputGain_dB(vol_dB);
        tlv320aic3206_1.volume_dB(fixed_hp_dB);
      }
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
