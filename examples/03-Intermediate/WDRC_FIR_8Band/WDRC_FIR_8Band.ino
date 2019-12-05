/*
  WDRC_FIR_8band

  Created: Chip Audette (OpenAudio), Feb 2017
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 8-band compressor.  The BTNRH version was implemented the
    filters in the frequency-domain, whereas I implemented them in the time-domain.


  Uses Teensy Audio Adapter ro the Tympan Audio Board
      For Teensy Audio Board, assumes microphones (or whatever) are attached to the
      For Tympan Audio Board, can use on board mics or external mic

  User Controls:
    Potentiometer on Tympan controls the headphone volume

   MIT License.  use at your own risk.
*/

//Use test tone as input (set to 1)?  Or, use live audio (set to zero)
#define USE_TEST_TONE_INPUT 0


// Include all the of the needed libraries
#include <Tympan_Library.h>

// Define the digital audio parameters
const float sample_rate_Hz = 24000.0f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 32;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                      myTympan(TympanRev::D, audio_settings);  //TympanRev::D or TympanRev::C
AudioSynthWaveformSine_F32  testSignal(audio_settings);          //use to generate test tone as input
AudioInputI2S_F32           i2s_in(audio_settings);          //Digital audio *from* the Teensy Audio Board ADC.  Sends Int16.  Stereo.
AudioOutputI2S_F32          i2s_out(audio_settings);        //Digital audio *to* the Teensy Audio Board DAC.  Expects Int16.  Stereo

//create audio objects for the algorithm
AudioEffectGain_F32     preGain;
#define N_BBCOMP 2      //number of broadband compressors
AudioEffectCompWDRC_F32  compBroadband[N_BBCOMP]; //here are the broad band compressors
#define N_CHAN 8        //number of channels to use for multi-band compression
AudioFilterFIR_F32      firFilt[N_CHAN];  //here are the filters to break up the audio into multipel bands
AudioEffectCompWDRC_F32  compPerBand[N_CHAN]; //here are the per-band compressors
AudioMixer8_F32         mixer1; //mixer to reconstruct the broadband audio

//choose the input audio source
#if (USE_TEST_TONE_INPUT == 1)
  AudioConnection_F32       patchCord1(testSignal, 0, preGain, 0);    //connect a test tone to the Left Int->Float converter
#else
  AudioConnection_F32       patchCord1(i2s_in, 0, preGain, 0);   //#8 wants left, #3 wants right. //connect the Left input to the Left Int->Float converter
#endif

//apply the first broadband compressor
AudioConnection_F32     patchCord5(preGain, 0, compBroadband[0], 0);

//connect to each of the filters to make the sub-bands
AudioConnection_F32     patchCord11(compBroadband[0], 0, firFilt[0], 0);
AudioConnection_F32     patchCord12(compBroadband[0], 0, firFilt[1], 0);
AudioConnection_F32     patchCord13(compBroadband[0], 0, firFilt[2], 0);
AudioConnection_F32     patchCord14(compBroadband[0], 0, firFilt[3], 0);
AudioConnection_F32     patchCord15(compBroadband[0], 0, firFilt[4], 0);
AudioConnection_F32     patchCord16(compBroadband[0], 0, firFilt[5], 0);
AudioConnection_F32     patchCord17(compBroadband[0], 0, firFilt[6], 0);
AudioConnection_F32     patchCord18(compBroadband[0], 0, firFilt[7], 0);

//connect each filter to its corresponding per-band compressor
AudioConnection_F32     patchCord21(firFilt[0], 0, compPerBand[0], 0);
AudioConnection_F32     patchCord22(firFilt[1], 0, compPerBand[1], 0);
AudioConnection_F32     patchCord23(firFilt[2], 0, compPerBand[2], 0);
AudioConnection_F32     patchCord24(firFilt[3], 0, compPerBand[3], 0);
AudioConnection_F32     patchCord25(firFilt[4], 0, compPerBand[4], 0);
AudioConnection_F32     patchCord26(firFilt[5], 0, compPerBand[5], 0);
AudioConnection_F32     patchCord27(firFilt[6], 0, compPerBand[6], 0);
AudioConnection_F32     patchCord28(firFilt[7], 0, compPerBand[7], 0);

//compute the output of the per-band compressors to the mixers (to make into one signal again)
AudioConnection_F32     patchCord31(compPerBand[0], 0, mixer1, 0);
AudioConnection_F32     patchCord32(compPerBand[1], 0, mixer1, 1);
AudioConnection_F32     patchCord33(compPerBand[2], 0, mixer1, 2);
AudioConnection_F32     patchCord34(compPerBand[3], 0, mixer1, 3);
AudioConnection_F32     patchCord35(compPerBand[4], 0, mixer1, 4);
AudioConnection_F32     patchCord36(compPerBand[5], 0, mixer1, 5);
AudioConnection_F32     patchCord37(compPerBand[6], 0, mixer1, 6);
AudioConnection_F32     patchCord38(compPerBand[7], 0, mixer1, 7);


//connect the output of the mixers to the final broadband compressor
AudioConnection_F32     patchCord43(mixer1, 0, compBroadband[1], 0);

//send the audio out
AudioConnection_F32     patchCord44(compBroadband[1], 0, i2s_out, 0);    //Left.  makes Float connections between objects
AudioConnection_F32     patchCord45(compBroadband[1], 0, i2s_out, 1);    //Right.  makes Float connections between objects


void setupTympanHardware(void) {
  myTympan.println("Setting up Tympan Audio Board...");
  myTympan.enable(); // activate AIC

  //choose input
  switch (2) {
    case 1:
      //choose on-board mics
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
      break;
    case 2:
      //choose external input, as a line in
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //
      break;
    case 3:
      //choose external mic plus the desired bias level
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack
      myTympan.setMicBias(TYMPAN_MIC_BIAS_2_5); //choices: TYMPAN_MIC_BIAS_2_5, TYMPAN_MIC_BIAS_1_7, TYMPAN_MIC_BIAS_1_25, TYMPAN_MIC_BIAS_VSUPPLY
      break;
  }

  //set volumes
  myTympan.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(15.f); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}

//define functions to setup the audio processing parameters
#define N_FIR 96
float firCoeff[N_CHAN][N_FIR];
void setupAudioProcessing(void) {
  //set the pre-gain (if used)
  preGain.setGain_dB(0.0f);

  //set the per-channel filter coefficients
  #include "GHA_Constants.h"  //this sets dsl and gha, which are used in the next line
  AudioConfigFIRFilterBank_F32 makeFIRcoeffs(N_CHAN, N_FIR, audio_settings.sample_rate_Hz, (float *)dsl.cross_freq, (float *)firCoeff);
  for (int i=0; i< N_CHAN; i++) firFilt[i].begin(firCoeff[i], N_FIR, audio_settings.audio_block_samples);

  //setup all of the the compressors
  configureBroadbandWDRCs(N_BBCOMP, audio_settings.sample_rate_Hz, &gha, compBroadband);
  configurePerBandWDRCs(N_CHAN, audio_settings.sample_rate_Hz, &dsl, &gha, compPerBand);
}

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000); //let's use the print functions in "myTympan" so it goes to BT, too!

  myTympan.println("WDRC_Compressor_Float: setup()...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory_F32(40,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //check the potentiometer
  servicePotentiometer(millis(),0);

  //setup sine wave as test signal..if the sine input
  testSignal.amplitude(0.01);
  testSignal.frequency(500.0f);
  myTympan.println("setup() complete");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  //asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis(),100);

  //update the memory and CPU usage...if enough time has passed
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //print the state of the gain processing
  printCompressorState(millis(),3000);

} //end loop()


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = 0.1 * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      #if USE_TEST_TONE_INPUT==1
            float freq = 700.f + 200.f * ((val - 0.5) * 2.0); //change tone 700Hz +/- 200 Hz
            myTympan.print("Changing tone frequency to = "); myTympan.println(freq);
            testSignal.frequency(freq);
      #else
            float min_mic_dB = +15.0f;
            float vol_dB = min_mic_dB + 25.0f * ((val - 0.5) * 2.0); //set volume as 15dB +/- 25 dB
            //vol_dB = 1.5*vol_dB + 10.0f; //(+/15*1.75 = +/-
            myTympan.print("Changing input gain = "); myTympan.print(vol_dB); myTympan.println(" dB");
            if (vol_dB  < min_mic_dB) {
              myTympan.setInputGain_dB(min_mic_dB);
              myTympan.volume_dB(vol_dB-min_mic_dB);
            } else {
              myTympan.setInputGain_dB(vol_dB);
              myTympan.volume_dB(0.0f);
            }
      #endif
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printCompressorState(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

      myTympan.print("Gain Processing: Pre-Gain (dB) = ");
      myTympan.print(preGain.getGain_dB());
      myTympan.print(", BB1 Level (dB) = "); myTympan.print(compBroadband[0].getCurrentLevel_dB());
      myTympan.print(". Per-Chan Level (dB) = ");
      for (int Ichan = 0; Ichan < N_CHAN; Ichan++ ) {
        myTympan.print(compPerBand[Ichan].getCurrentLevel_dB());
        if (Ichan < (N_CHAN-1)) myTympan.print(", ");
      }
      myTympan.print(". BB2 Level (dB) = "); myTympan.print(compBroadband[1].getCurrentLevel_dB());
      myTympan.println();

      lastUpdate_millis = curTime_millis; //we will use this value the next time around.
    }
};


static void configureBroadbandWDRCs(int ncompressors, float fs_Hz, BTNRH_WDRC::CHA_WDRC *gha, AudioEffectCompWDRC_F32 *WDRCs) {
  //assume all broadband compressors are the same
  for (int i=0; i< ncompressors; i++) {
    //logic and values are extracted from from CHAPRO repo agc_prepare.c...the part setting CHA_DVAR

    //extract the parameters
    float atk = (float)gha->attack;  //milliseconds!
    float rel = (float)gha->release; //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override...not taken from gha
    float maxdB = (float) gha->maxdB;
    float exp_cr = (float)gha->exp_cr;
    float exp_end_knee = (float)gha->exp_end_knee;
    float tk = (float) gha->tk;
    float comp_ratio = (float) gha->cr;
    float tkgain = (float) gha->tkgain;
    float bolt = (float) gha->bolt;

    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk,rel,maxdB,exp_cr,exp_end_knee,tkgain,comp_ratio,tk,bolt);
  }
}

static void configurePerBandWDRCs(int nchan, float fs_Hz, BTNRH_WDRC::CHA_DSL *dsl, BTNRH_WDRC::CHA_WDRC *gha, AudioEffectCompWDRC_F32 *WDRCs) {
  if (nchan > dsl->nchannel) {
    myTympan.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    myTympan.print(F("    : nchan = ")); myTympan.println(nchan);
    myTympan.print(F("    : dsl.nchannel = ")); myTympan.println(dsl->nchannel);
  }

  //now, loop over each channel
  for (int i=0; i < nchan; i++) {

    //logic and values are extracted from from CHAPRO repo agc_prepare.c
    float atk = (float)dsl->attack;   //milliseconds!
    float rel = (float)dsl->release;  //milliseconds!
    float fs = (float) fs_Hz; // WEA override
    float maxdB = (float) dsl->maxdB;
    float exp_cr = (float) dsl->exp_cr[i];
    float exp_end_knee = (float) dsl->exp_end_knee[i];    
    float tk = (float) dsl->tk[i];
    float comp_ratio = (float) dsl->cr[i];
    float tkgain = (float) dsl->tkgain[i];
    float bolt = (float) dsl->bolt[i];

    // adjust BOLT
    float cltk = (float)gha->tk;
    if (bolt > cltk) bolt = cltk;
    if (tkgain < 0) bolt = bolt + tkgain;

    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk,rel,maxdB,exp_cr,exp_end_knee,tkgain,comp_ratio,tk,bolt);
    
  }
}

//static void configureMultiBandWDRCasGHA(float fs_Hz, BTNRH_WDRC::CHA_DSL *dsl, BTNRH_WDRC::CHA_WDRC *gha,
//    int nBB, AudioEffectCompWDRC_F32 *broadbandWDRCs,
//    int nchan, AudioEffectCompWDRC_F32 *perBandWDRCs) {
//
//  configureBroadbandWDRCs(nBB, fs_Hz, gha, broadbandWDRCs);
//  configurePerBandWDRCs(nchan, fs_Hz, dsl, gha, perBandWDRCs);
//}
