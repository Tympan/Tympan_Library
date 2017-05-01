/*
  MultiBandCompressor_Float

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

//Use the new Tympan board?
#define USE_TYMPAN 1    //1 = uses tympan hardware, 0 = uses the old teensy audio board

//Use test tone as input (set to 1)?  Or, use live audio (set to zero)
#define USE_TEST_TONE_INPUT 0


// Include all the of the needed libraries
//include "AudioStream_Mod.h" //include my custom AudioStream.h...this prevents the default one from being used
//include <Audio.h>      //Teensy Audio Library
#include <Wire.h>
#include <SPI.h>
//include <SD.h>
//include <SerialFlash.h>
#include <Tympan_Library.h>


const float sample_rate_Hz = 24000.0f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 32;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
#if USE_TYMPAN == 0
  AudioControlSGTL5000        audioHardware;    //controller for the Teensy Audio Board
#else
  AudioControlTLV320AIC3206   audioHardware;    //controller for the Teensy Audio Board
#endif
AudioSynthWaveformSine_F32  testSignal(audio_settings);          //use to generate test tone as input
AudioInputI2S_F32           i2s_in(audio_settings);          //Digital audio *from* the Teensy Audio Board ADC.  Sends Int16.  Stereo.
AudioOutputI2S_F32          i2s_out(audio_settings);        //Digital audio *to* the Teensy Audio Board DAC.  Expects Int16.  Stereo
//AudioAnalyzeRMS         rms_input, rms_output; //only used for Bluetooth monitoring of signal levels
//AudioConvert_F32toI16   float2Int1, float2Int2;     //Converts Float to Int16.

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
  //AudioConnection_F32       patchCord2(testSignal, 0, float2Int1, 0);
#else
  AudioConnection_F32       patchCord1(i2s_in, 0, preGain, 0);   //#8 wants left, #3 wants right. //connect the Left input to the Left Int->Float converter
  //AudioConnection_F32       patchCord2(i2s_in, 0, float2Int1, 0);    //connect the Left input to level monitor (Bluetooth reporting only)
#endif
//AudioConnection       patchCord2(float2Int1, 0, rms_input, 0);    //connect the Left input to level monitor (Bluetooth reporting only)

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

//AudioConnection_F32   patchCord45(compBroadband[1], 0, float2Int2, 0);
//AudioConnection       patchCord46(float2Int2, 0, rms_output, 0); //measure the RMS of the output (for reporting)

//Bluetooth parameters
#define USE_BT_SERIAL 0  //set to zero to disable bluetooth
#define BT_SERIAL Serial1

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define functions to setup the hardware
void setupAudioHardware(void) {
  //#if USE_TYMPAN == 1
    setupTympanHardware();
  //#else
  //  setupTeensyAudioBoard();
  //#endif

  //setup the potentiometer.  same for Teensy Audio Board as for Tympan
  pinMode(POT_PIN, INPUT); //set the potentiometer's input pin as an INPUT
}

//void setupTeensyAudioBoard(void) {
//  Serial.println("Setting up Teensy Audio Board...");
//  const int myInput = AUDIO_INPUT_LINEIN;   //which input to use?  AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
//  audioHardware.enable();                   //start the audio board
//  audioHardware.inputSelect(myInput);       //choose line-in or mic-in
//  audioHardware.volume(0.8);                //volume can be 0.0 to 1.0.  0.5 seems to be the usual default.
//  audioHardware.lineInLevel(10, 10);        //level can be 0 to 15.  5 is the Teensy Audio Library's default
//  audioHardware.adcHighPassFilterDisable(); //reduces noise.  https://forum.pjrc.com/threads/27215-24-bit-audio-boards?p=78831&viewfull=1#post78831
//}

void setupTympanHardware(void) {
  Serial.println("Setting up Tympan Audio Board...");
  audioHardware.enable(); // activate AIC
  
  //choose input
  switch (1) {
    case 1: 
      //choose on-board mics
      audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
      break;
    case 2:
      //choose external input, as a line in
      audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //
      break;
    case 3:
      //choose external mic plus the desired bias level
      audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack
      int myBiasLevel = TYMPAN_MIC_BIAS_2_5;  //choices: TYMPAN_MIC_BIAS_2_5, TYMPAN_MIC_BIAS_1_7, TYMPAN_MIC_BIAS_1_25, TYMPAN_MIC_BIAS_VSUPPLY
      audioHardware.setMicBias(myBiasLevel); // set mic bias to 2.5 // default
      break;
  }
  
  //set volumes
  audioHardware.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  audioHardware.setInputGain_dB(15.f); // set MICPGA volume, 0-47.5dB in 0.5dB setps
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
  Serial.begin(115200);   //Open USB Serial link...for debugging
  if (USE_BT_SERIAL) BT_SERIAL.begin(115200);  //Open BT link
  delay(500); 
  
  Serial.println("WDRC_Compressor_Float: setup()...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);
  if (USE_BT_SERIAL) BT_SERIAL.println("WDRC_Compressor_Float: starting...");

  // Audio connections require memory
  AudioMemory(10);      //allocate Int16 audio data blocks (need a few for under-the-hood stuff)
  AudioMemory_F32_wSettings(40,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupAudioHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //setup sine wave as test signal..if the sine input
  testSignal.amplitude(0.01);
  testSignal.frequency(500.0f);
  Serial.println("setup() complete");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service the potentiometer...if enough time has passed
  servicePotentiometer(millis());

  //update the memory and CPU usage...if enough time has passed
  printMemoryAndCPU(millis());

  //print the state of the gain processing
  printCompressorState(millis(),&Serial);

  //print compressor status to bluetooth
  #if USE_BT_SERIAL 
    printStatusToBluetooth(millis(), &BT_SERIAL);
  #endif

} //end loop()


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
      if (USE_TYMPAN == 1) val = 1.0 - val; //reverse direction of potentiometer (error with Tympan PCB)

      #if USE_TEST_TONE_INPUT==1
            float freq = 700.f + 200.f * ((val - 0.5) * 2.0); //change tone 700Hz +/- 200 Hz
            Serial.print("Changing tone frequency to = "); Serial.println(freq);
            testSignal.frequency(freq);
      #else
        #if USE_TYMPAN == 1
              //float vol_dB = 0.f + 15.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 15 dB
              //Serial.print("Changing output volume to = "); Serial.print(vol_dB); Serial.println(" dB");
              //audioHardware.volume_dB(vol_dB);

              float min_mic_dB = +15.0f;
              float vol_dB = min_mic_dB + 25.0f * ((val - 0.5) * 2.0); //set volume as 15dB +/- 25 dB
              //vol_dB = 1.5*vol_dB + 10.0f; //(+/15*1.75 = +/-
              Serial.print("Changing input gain = "); Serial.print(vol_dB); Serial.println(" dB");
              if (vol_dB  < min_mic_dB) {
                audioHardware.setInputGain_dB(min_mic_dB);
                audioHardware.volume_dB(vol_dB-min_mic_dB);
              } else {
                audioHardware.setInputGain_dB(vol_dB);
                audioHardware.volume_dB(0.0f);
              }
        #else
              float vol = 0.70f + 0.15f * ((val - 0.5) * 2.0); //set volume as 0.70 +/- 0.15
              Serial.print("Setting output volume control to = "); Serial.println(vol);
              audioHardware.volume(vol);
        #endif
      #endif
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printMemoryAndCPU(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("CPU Cur/Peak: ");
    Serial.print(audio_settings.processorUsage());
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
    Serial.print("%,   ");
    Serial.print("Dyn MEM Int16 Cur/Peak: ");
    Serial.print(AudioMemoryUsage());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax());
    Serial.print(",   ");
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void printCompressorState(unsigned long curTime_millis, Stream *s) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

      s->print("Gain Processing: Pre-Gain (dB) = ");
      s->print(preGain.getGain_dB());
      s->print(", BB1 Level (dB) = "); s->print(compBroadband[0].getCurrentLevel_dB());
      s->print(". Per-Chan Level (dB) = ");
      for (int Ichan = 0; Ichan < N_CHAN; Ichan++ ) {
        s->print(compPerBand[Ichan].getCurrentLevel_dB());
        if (Ichan < (N_CHAN-1)) s->print(", ");
      }
      s->print(". BB2 Level (dB) = "); s->print(compBroadband[1].getCurrentLevel_dB());
      s->println();

      lastUpdate_millis = curTime_millis; //we will use this value the next time around.
    }
};


//void printStatusToBluetooth(unsigned long curTime_millis, Stream *s) {
//  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
//  static unsigned long lastUpdate_millis = 0;
//
//  //has enough time passed to update everything?
//  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
//  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
//
//      s->print("In, Out (dBFS): ");
//      s->print(AudioEffectCompWDR_F32::fast_dB(rms_input.read())); //use a faster dB function
//      s->print(", ");
//      s->print(AudioEffectCompWDR_F32::fast_dB(rms_output.read())); //use a faster dB function
//      s->println();
//      
//      lastUpdate_millis = curTime_millis; //we will use this value the next time around.
//    }
//};
//


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
    float tk = (float) gha->tk;
    float comp_ratio = (float) gha->cr;
    float tkgain = (float) gha->tkgain;
    float bolt = (float) gha->bolt;
    
    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk,rel,maxdB,tkgain,comp_ratio,tk,bolt);    
  }
}
    
static void configurePerBandWDRCs(int nchan, float fs_Hz, BTNRH_WDRC::CHA_DSL *dsl, BTNRH_WDRC::CHA_WDRC *gha, AudioEffectCompWDRC_F32 *WDRCs) {
  if (nchan > dsl->nchannel) {
    Serial.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    Serial.print(F("    : nchan = ")); Serial.println(nchan);
    Serial.print(F("    : dsl.nchannel = ")); Serial.println(dsl->nchannel);
  }
  
  //now, loop over each channel
  for (int i=0; i < nchan; i++) {
    
    //logic and values are extracted from from CHAPRO repo agc_prepare.c
    float atk = (float)dsl->attack;   //milliseconds!
    float rel = (float)dsl->release;  //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override
    float maxdB = (float) gha->maxdB;
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
    WDRCs[i].setParams(atk,rel,maxdB,tkgain,comp_ratio,tk,bolt);
  }  
}

//static void configureMultiBandWDRCasGHA(float fs_Hz, BTNRH_WDRC::CHA_DSL *dsl, BTNRH_WDRC::CHA_WDRC *gha, 
//    int nBB, AudioEffectCompWDRC_F32 *broadbandWDRCs,
//    int nchan, AudioEffectCompWDRC_F32 *perBandWDRCs) {
//    
//  configureBroadbandWDRCs(nBB, fs_Hz, gha, broadbandWDRCs);
//  configurePerBandWDRCs(nchan, fs_Hz, dsl, gha, perBandWDRCs);
//}

