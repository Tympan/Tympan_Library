/*
  FrequencyCompression_FD  (Stereo!)

  Demonstrate non-linear frequency compression and shifting via frequency domain processing.

  By "non-linear", you can compress and/or shift some of frequency range, but you don't
  have to shift *all* of the frequency range as in our other frequency-domain examples.
  Typically, this is used to squeeze a wide range of higher frequencies down into the
  lower frequencies to avoid high-frequency hearing loss.

  Created: Chip Audette (OpenAudio) Oct 2020-Jun 2021 based on guidance from Joshua Alexander
  through personal communication.  See also his publications, such as Alexander, J.M. et al,
  "Effects of Frequency Compression and Frequency Transposition on Fricative and Affricative
  Perception in Listeners with Normal Hearing and Mild to Moderate Hearing Loss".  Ear & 
  Hearing.  2014.

  Approach: This processing is performed in the frequency domain where we take the  FFT, scale
  or shift the bins upward or downward, take the IFFT, and listen to the results.

  Currently, this is implemented as using the vocoder technique, meaning only the FFT amplitudes
  are moved around while FFT phases are left at their original frequencies.

  To change the length of the FFT, you need to change the audio processing block size 
  "audio_block_samples" and the FFT "overlap_factor".  The FFT ("N") is simply 
  audio_block_samples*overlap_factor.  For example:

    64 sample blocks * 2 overlap_factor is an N_FFT of 128 points (with 50% overlap)
    64 sample blocks * 4 overlap_factor is an N_FFT of 256 points (with 75% overlap)
    128 sample blocks * 2 overlap_factor is an N_FFT of 256 points (with 50% overlap)
    128 sample blocks * 4 overlap_factor is an N_FFT of 512 points (with 75% overlap)

  Be aware of being limited by how much computational power that is available. For Tympan RevD 
  (ie, Teensy 3.6) running in stereo, here is the self-reported CPU usage:

     fs = 24kHz, stereo (analog inputs, -500 Hz shift):
        32 sample blocks, 4 overlap_fac (N=128, 75% overlap -> 188 Hz resolution): CPU = 46%  (9.6 msec latency?)
        64 sample blocks, 2 overlap_fac (N=128, 50% overlap -> 188 Hz resolution): CPU = 23%  (12.3 msec latency?)
        64 sample blocks, 4 overlap_fac (N=256, 75% overlap -> 94 Hz resolution):  CPU = 37%  (17.6 msec latency?)
        128 sample blocks, 2 overlap_fac (N=256, 50% overlap -> 94 Hz resolution): CPU = 18%  (22.9 msec latency?)
        128 sample blocks, 4 overlap_fac (N=512, 75% overlap -> 47 Hz resolution): CPU = 53%  (33.6 msec latency?)

     fs = 44100 kHz, stereo (analog inputs, -500 Hz shift):
        32 sample blocks, 4 overlap_fac (N=128, 75% overlap -> 345 Hz resolution):  CPU = 84% (5.2 msec latency?)
        64 sample blocks, 2 overlap_fac (N=128, 50% overlap -> 345 Hz resolution):  CPU = 42% (6.7 msec latency?)
        64 sample blocks, 4 overlap_fac (N=256, 75% overlap -> 172 Hz resolution):  CPU = 66% (9.6 msec latency?)
        128 sample blocks, 2 overlap_fac (N=256, 50% overlap -> 172 Hz resolution): CPU = 32% (12.5 msec latency?)
        128 sample blocks, 4 overlap_fac (N=512, 75% overlap -> 86 Hz resolution):  CPU = 98%+ (too slow!  try mono.)  (would be 18.3 msec latency?)

  If you are using the Tympan RevE (ie, Teensy 4.1), however, it is much more efficient in
  executing FFT and IFFT operations, so you can probably do 4-5x more calcluations (ie CPU 
  utilization will be 1/4 to 1/5 of the values shown above for the RevD).

  Latency (sec) is probably something like [ (17+21 samples) + 2*block_length + N_FFT) / sample_rate_Hz ]

  MIT License.  Use at your own risk.

 */

#include <Tympan_Library.h>
#include "AudioEffectFreqComp_FD_F32.h"
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
//const float sample_rate_Hz = 24000.f; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const float sample_rate_Hz = 44100.f;   //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;       //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
const int FFT_overlap_factor = 4;         //2 is 50% overlap, 4 is 75% overlap
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                       myTympan(TympanRev::F, audio_settings); //do TympanRev::D or E or F
//AICShield                  earpieceShield(TympanRev::F, AICShieldRev::A);  //do TympanRev::D or E or F
AudioInputI2S_F32            i2s_in(audio_settings);          //Digital audio *from* the Tympan AIC.
AudioEffectFreqComp_FD_F32   freqShift_L(audio_settings);     //create the frequency-domain processing block
AudioEffectFreqComp_FD_F32   freqShift_R(audio_settings);     //create the frequency-domain processing block
AudioEffectGain_F32          gain_L(audio_settings);          //Applies digital gain to audio data.
AudioEffectGain_F32          gain_R(audio_settings);          //Applies digital gain to audio data.
AudioOutputI2S_F32           i2s_out(audio_settings);         //Digital audio out *to* the Tympan AIC.

//Make all of the audio connections
AudioConnection_F32       patchCord10(i2s_in, 0, freqShift_L, 0);  //use the Left input
AudioConnection_F32       patchCord11(i2s_in, 1, freqShift_R, 0);  //use the Right input
AudioConnection_F32       patchCord20(freqShift_L, 0, gain_L, 0);  //connect to gain
AudioConnection_F32       patchCord21(freqShift_R, 0, gain_R, 0);  //connect to gain
AudioConnection_F32       patchCord30(gain_L, 0, i2s_out, 0);      //connect to the left output
AudioConnection_F32       patchCord41(gain_R, 0, i2s_out, 1);      //connect to the right output

//Create BLE
BLE& ble = myTympan.getBLE();   //myTympan owns the ble object, but we have a reference to it here

//control display and serial interaction
SerialManager serialManager(&ble);
State myState(&audio_settings, &myTympan);

//inputs and levels
float default_mic_input_gain_dB = 20.0f; //gain on the microphone
void switchToPCBMics(void) {
  myTympan.println("Switching to PCB Mics.");
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the microphone jack - defaults to mic bias OFF
  setInputGain_dB(default_mic_input_gain_dB);
}
void switchToLineInOnMicJack(void) {
  myTympan.println("Switching to Line-in on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  setInputGain_dB(0.0);
}
void switchToMicInOnMicJack(void) {
  myTympan.println("Switching to Mic-In on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias OFF
  myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
  setInputGain_dB(default_mic_input_gain_dB);
}

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState);

  //create the GUI (but don't transmit it yet.  wait for the requuest from the phone.)
  serialManager.createTympanRemoteLayout();
}

// ////////////////////////////////////////////

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1500);
  myTympan.println("freqLowering: starting setup()...");
  myTympan.print("    : sample rate (Hz) = ");  myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("    : block size (samples) = ");  myTympan.println(audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(40, audio_settings);

  // Configure the FFT parameters algorithm
  int N_FFT = audio_block_samples * FFT_overlap_factor;
  Serial.print("    : N_FFT = "); Serial.println(N_FFT);
  freqShift_L.setup(audio_settings, N_FFT); //do after AudioMemory_F32();
  freqShift_R.setup(audio_settings, N_FFT); //do after AudioMemory_F32();

  //configure the frequency shifting
  setFreqKnee_Hz(myState.freq_knee_Hz);
  setFreqCR(myState.freq_CR);
  setFreqShift_Hz(myState.freq_shift_Hz); //0 is no freq shifting.
  Serial.print("FFT resolution allowed for a shift of "); Serial.print(myState.freq_shift_Hz); Serial.println(" Hz");

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
  //earpieceShield.enable();

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disble
  //earpieceShield.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable

  //Choose the desired input
  switchToPCBMics();        //use PCB mics as input
  //switchToMicInOnMicJack(); //use Mic jack as mic input (ie, with mic bias)
  //switchToLineInOnMicJack();  //use Mic jack as line input (ie, no mic bias)

  //Set the desired volume levels
  setOutputGain_dB(myState.output_gain_dB);    // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

   //setup BLE
	myTympan.setupBLE(); delay(500); //Assumes the default Bluetooth firmware.

  //finish the setup by printing the help menu to the serial connections
  setupSerialManager();
  serialManager.printHelp();
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i = 0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]); //ends up in serialManager.processCharacter()
  }

  //If there is no BLE connection, make sure that we keep advertising
  ble.updateAdvertising(millis(), 5000); //check every 5000 msec

  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000 msec
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, const unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0 / 10.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //use the potentiometer value to control something interesting
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      //change the volume
      float gain_dB = 0.f + 30.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 30 dB
      myTympan.print("Changing output volume to = "); myTympan.print(gain_dB); myTympan.println(" dB");
      setOutputGain_dB(gain_dB);
      serialManager.setOutputGainButtons();
    }


    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printGainSettings(void) {
  myTympan.print("Gain (dB): ");
  myTympan.print("  Input Gain = "); myTympan.println(myState.input_gain_dB);
  myTympan.print("  Digital Gain = "); myTympan.println(myState.digital_gain_dB, 1);
  myTympan.print("  Output Gain = "); myTympan.println(myState.output_gain_dB);
}

// //set various gain settings
float setInputGain_dB(float gain_dB) { return myState.input_gain_dB = myTympan.setInputGain_dB(gain_dB);}
float setOutputGain_dB(float gain_dB) { return myState.output_gain_dB = myTympan.volume_dB(gain_dB); }
void incrementDigitalGain(float increment_dB) { setDigitalGain_dB(myState.digital_gain_dB + increment_dB); }
void setDigitalGain_dB(float gain_dB) { gain_L.setGain_dB(gain_dB); gain_R.setGain_dB(gain_dB); }

// //set various settings for the frequency compression
float incrementFreqKnee(float incr_factor) {
  float cur_val = freqShift_L.getStartFreq_Hz();
  return setFreqKnee_Hz(cur_val + incr_factor);
}
float setFreqKnee_Hz(float new_val) { 
  freqShift_L.setStartFreq_Hz(new_val);
  freqShift_R.setStartFreq_Hz(new_val);
  return myState.freq_knee_Hz = freqShift_R.getStartFreq_Hz();
}

float incrementFreqCR(float incr_factor) {
  incr_factor = max(0.1, min(5.0, incr_factor));
  float cur_val = freqShift_L.getFreqCompRatio();
  return setFreqCR(cur_val * incr_factor);
}
float setFreqCR(float new_val) { 
  freqShift_L.setFreqCompRatio(new_val);
  freqShift_R.setFreqCompRatio(new_val);
  return myState.freq_CR = freqShift_R.getFreqCompRatio();
}

float incrementFreqShift(float incr_factor) {
  float cur_val = freqShift_L.getShift_Hz();
  return setFreqShift_Hz(cur_val + incr_factor);
}
float setFreqShift_Hz(float new_val) { 
  freqShift_L.setShift_Hz(new_val);
  freqShift_R.setShift_Hz(new_val);
  return myState.freq_shift_Hz = freqShift_R.getShift_Hz();
}
