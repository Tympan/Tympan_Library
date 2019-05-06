/*
  WDRC_3BandIIR_wBT_wAFC

  Created: Chip Audette (OpenAudio), 2018
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 3-band WDRC compressor with adaptive feedback cancelation (AFC)
      based on the work of BTNRH.
    
  Filters: The BTNRH filterbank was implemented in the frequency-domain, whereas
	  I implemented them in the time-domain via IIR filters.  Furthermore, I delay
	  the individual IIR filters to try to line up their impulse response so that
	  the overall frequency response is smoother because the phases are better aligned
	  in the cross-over region between neighboring filters.
   
  Compressor: The BTNRH WDRC compresssor did not include an expansion stage at low SPL.
	  I added an expansion stage to better manage noise.

  Feedback Management: Implemented the BTNHRH adaptivev feedback cancelation algorithm
    from their CHAPRO repository: https://github.com/BoysTownorg/chapro
	  
  Connectivity: Communicates via USB Serial and via Bluetooth Serial

  User Controls:
    Potentiometer on Tympan controls the algorithm gain.

  MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//local files
#include "AudioEffectFeedbackCancel_F32.h"
#include "AudioEffectAFC_BTNRH_F32.h"
#include "SerialManager.h"

//Bluetooth parameters...if used
#define USE_BT_SERIAL 1   //set to zero to disable bluetooth
#define BT_SERIAL Serial1

// Define the overall setup
String overall_name = String("Tympan: 3-Band IIR WDRC with Adaptive Feedback Cancelation");
const int N_CHAN_MAX = 3;  //number of frequency bands (channels)
int N_CHAN = N_CHAN_MAX;  //will be changed to user-selected number of channels later
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0; //will be overridden by volume knob

int USE_VOLUME_KNOB = 1;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below

const float sample_rate_Hz = 22050.0f ; //16000, 24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                        audioHardware(TympanRev::D);     //do TympanRev::C or TympanRev::D
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //keep this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
AudioFilterBiquad_F32      preFilter(audio_settings);   //remove low frequencies near DC
#if 0
  AudioEffectFeedbackCancel_F32 feedbackCancel(audio_settings);      //adaptive feedback cancelation, optimized by Chip Audette
#else
  AudioEffectAFC_BTNRH_F32 feedbackCancel(audio_settings);   //original adaptive feedback cancelation from BTNRH
#endif
AudioFilterBiquad_F32       bpFilt[N_CHAN_MAX];         //here are the filters to break up the audio into multiple bands
AudioEffectDelay_F32        postFiltDelay[N_CHAN_MAX];  //Here are the delay modules that we'll use to time-align the output of the filters
AudioEffectCompWDRC_F32    expCompLim[N_CHAN_MAX];     //here are the per-band compressors
AudioMixer8_F32             mixer1;                     //mixer to reconstruct the broadband audio
AudioEffectCompWDRC_F32    compBroadband;              //broad band compressor
AudioEffectFeedbackCancel_LoopBack_F32 feedbackLoopBack(audio_settings);
AudioOutputI2S_F32          i2s_out(audio_settings);    //Digital audio output to the DAC.  Should be last.

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32  audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_filterbank(audio_settings);
AudioControlTestAmpSweep_F32    ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_FIR(audio_settings,audioTestGenerator,audioTestMeasurement_filterbank);

//make the audio connections
#define N_MAX_CONNECTIONS 100  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count=0;

  //connect input
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioTestGenerator, 0); //#8 wants left, #3 wants right. //connect the Left input to the Left Int->Float converter

  //make the connection for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_filterbank, 0);

  //start the algorithms with the feedback cancallation block
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, feedbackCancel, 0);

  //make per-channel connections: filterbank -> delay -> WDRC Compressor -> mixer (synthesis)
  for (int i = 0; i < N_CHAN_MAX; i++) {
    patchCord[count++] = new AudioConnection_F32(feedbackCancel, 0, bpFilt[i], 0); //connect to Feedback canceler
    patchCord[count++] = new AudioConnection_F32(bpFilt[i], 0, postFiltDelay[i], 0);  //connect to delay
    patchCord[count++] = new AudioConnection_F32(postFiltDelay[i], 0, expCompLim[i], 0); //connect to compressor
    patchCord[count++] = new AudioConnection_F32(expCompLim[i], 0, mixer1, i); //connect to mixer

    //make the connection for the audio test measurements
    patchCord[count++] = new AudioConnection_F32(bpFilt[i], 0, audioTestMeasurement_filterbank, 1+i);
  }

  //connect the output of the mixers to the final broadband compressor
  patchCord[count++] = new AudioConnection_F32(mixer1, 0, compBroadband, 0);  //connect to final limiter

  //connect the loop back to the adaptive feedback canceller
  feedbackLoopBack.setTargetAFC(&feedbackCancel);
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, feedbackLoopBack, 0); //loopback to the adaptive feedback canceler

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 0);  //left output
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 1);  //right output
  //make the connections for the audio test measurements

  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, audioTestMeasurement, 1);

  return count;
}


//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; }; //"extern" let's be it accessible outside
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) { enable_printAveSignalLevels = !enable_printAveSignalLevels; printAveSignalLevels_as_dBSPL = as_dBSPL;};
SerialManager serialManager_USB(&Serial,N_CHAN_MAX,audioHardware,expCompLim,ampSweepTester,freqSweepTester,freqSweepTester_FIR,feedbackCancel);
#if (USE_BT_SERIAL)
  SerialManager serialManager_BT(&BT_SERIAL,N_CHAN_MAX,audioHardware,expCompLim,ampSweepTester,freqSweepTester,freqSweepTester_FIR,feedbackCancel); //this instance will handle the Bluetooth Serial link
#endif

//routine to setup the hardware
void setupTympanHardware(void) {
  Serial.println("Setting up Tympan Audio Board...");
  #if (USE_BT_SERIAL)
    BT_SERIAL.println("Setting up Tympan Audio Board...");
  #endif
  audioHardware.enable(); // activate AIC

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  audioHardware.enableMicDetect(true);

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  audioHardware.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble
  //Serial.print("Setting HP Filter in hardware at "); Serial.print(audioHardware.getHPCutoff_Hz()); Serial.println(" Hz.");

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop() 
  //audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board micropphones
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //set volumes
  audioHardware.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  audioHardware.setInputGain_dB(input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}


// /////////// setup the audio processing
//define functions to setup the audio processing parameters
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_Alternates.h"  //this sets alternate dsl and gha, which can be switched in via commands
#include "filter_coeff_sos.h"  //IIR filter coefficients for our filterbank
#define DSL_NORMAL 0
#define DSL_FULLON 1
int current_dsl_config = DSL_NORMAL; //used to select one of the configurations above for startup
float overall_cal_dBSPL_at0dBFS; //will be set later

void setupAudioProcessing(void) {
  //make all of the audio connections
  makeAudioConnections();

  //set the DC-blocking higpass filter cutoff
  preFilter.setHighpass(0,40.0);

  //setup processing based on the DSL and GHA prescriptions
  if (current_dsl_config == DSL_NORMAL) {
    setupFromDSLandGHAandAFC(dsl, gha, afc, N_CHAN_MAX, audio_settings);
  } else if (current_dsl_config == DSL_FULLON) {
    setupFromDSLandGHAandAFC(dsl_fullon, gha_fullon, afc_fullon, N_CHAN_MAX, audio_settings);
  }
}

void setupFromDSLandGHAandAFC(const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const BTNRH_WDRC::CHA_AFC &this_afc, const int n_chan_max, const AudioSettings_F32 &settings)
{
  //int n_chan = n_chan_max;  //maybe change this to be the value in the DSL itself.  other logic would need to change, too.
  N_CHAN = max(1,min(n_chan_max, this_dsl.nchannel));

  // //compute the per-channel filter coefficients
  //AudioConfigFIRFilterBank_F32 makeFIRcoeffs(n_chan, n_fir, settings.sample_rate_Hz, (float *)this_dsl.cross_freq, (float *)firCoeff);

  //give the pre-computed coefficients to the IIR filters
  for (int i=0; i< n_chan_max; i++) {
    if (i < N_CHAN) {
      bpFilt[i].setFilterCoeff_Matlab_sos(&(all_matlab_sos[i][0]),SOS_N_BIQUADS_PER_FILTER);   //from filter_coeff_sos.h.  Also calls begin().
    } else {
      bpFilt[i].end();
    }
  }

  //setup the per-channel delays
  for (int i=0; i<n_chan_max; i++) { 
    postFiltDelay[i].setSampleRate_Hz(audio_settings.sample_rate_Hz);
    if (i < N_CHAN) {
      postFiltDelay[i].delay(0,all_matlab_sos_delay_msec[i]);  //from filter_coeff_sos.h.  milliseconds!!!
    } else {
      postFiltDelay[i].delay(0,0);  //from filter_coeff_sos.h.  milliseconds!!!
    }
  }

  //setup the AFC
  feedbackCancel.setParams(this_afc);

  //setup all of the per-channel compressors
  configurePerBandWDRCs(N_CHAN, settings.sample_rate_Hz, this_dsl, this_gha, expCompLim);

  //setup the broad band compressor (limiter)
  configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, vol_knob_gain_dB, compBroadband);

  //overwrite the one-point calibration based on the dsl data structure
  overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;

}

void incrementDSLConfiguration(Stream *s) {
  current_dsl_config++;
  if (current_dsl_config==2) current_dsl_config=0;
  switch (current_dsl_config) {
    case (DSL_NORMAL):
      if (s) s->println("incrementDSLConfiguration: changing to NORMAL dsl configuration");
      setupFromDSLandGHAandAFC(dsl, gha, afc, N_CHAN_MAX, audio_settings);  break;
    case (DSL_FULLON):
      if (s) s->println("incrementDSLConfiguration: changing to FULL-ON dsl configuration");
      setupFromDSLandGHAandAFC(dsl_fullon, gha_fullon, afc_fullon, N_CHAN_MAX, audio_settings); break;
  }
}

void configureBroadbandWDRCs(float fs_Hz, const BTNRH_WDRC::CHA_WDRC &this_gha,
      float vol_knob_gain_dB, AudioEffectCompWDRC_F32 &WDRC)
{
  //assume all broadband compressors are the same
  //for (int i=0; i< ncompressors; i++) {
    //logic and values are extracted from from CHAPRO repo agc_prepare.c...the part setting CHA_DVAR

    //extract the parameters
    float atk = (float)this_gha.attack;  //milliseconds!
    float rel = (float)this_gha.release; //milliseconds!
    //float fs = this_gha.fs;
    float fs = (float)fs_Hz; // WEA override...not taken from gha
    float maxdB = (float) this_gha.maxdB;
    float exp_cr = (float) this_gha.exp_cr;
    float exp_end_knee = (float) this_gha.exp_end_knee;
    float tk = (float) this_gha.tk;
    float comp_ratio = (float) this_gha.cr;
    float tkgain = (float) this_gha.tkgain;
    float bolt = (float) this_gha.bolt;

    //set the compressor's parameters
    //WDRCs[i].setSampleRate_Hz(fs);
    //WDRCs[i].setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
    WDRC.setSampleRate_Hz(fs);
    WDRC.setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain + vol_knob_gain_dB, comp_ratio, tk, bolt);
 // }
}

void configurePerBandWDRCs(int nchan, float fs_Hz,
    const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
    AudioEffectCompWDRC_F32 *WDRCs)
{
  if (nchan > this_dsl.nchannel) {
    Serial.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    Serial.print(F("    : nchan = ")); Serial.println(nchan);
    Serial.print(F("    : dsl.nchannel = ")); Serial.println(dsl.nchannel);
  }

  //now, loop over each channel
  for (int i=0; i < nchan; i++) {

    //logic and values are extracted from from CHAPRO repo agc_prepare.c
    float atk = (float)this_dsl.attack;   //milliseconds!
    float rel = (float)this_dsl.release;  //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override
    float maxdB = (float) this_dsl.maxdB;
    float exp_cr = (float)this_dsl.exp_cr[i];
    float exp_end_knee = (float)this_dsl.exp_end_knee[i];
    float tk = (float) this_dsl.tk[i];
    float comp_ratio = (float) this_dsl.cr[i];
    float tkgain = (float) this_dsl.tkgain[i];
    float bolt = (float) this_dsl.bolt[i];

    // adjust BOLT
    float cltk = (float)this_gha.tk;
    if (bolt > cltk) bolt = cltk;
    if (tkgain < 0) bolt = bolt + tkgain;

    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk,rel,maxdB,exp_cr,exp_end_knee,tkgain,comp_ratio,tk,bolt);
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  Serial.begin(115200);   //Open USB Serial link...for debugging
  #if (USE_BT_SERIAL)
    BT_SERIAL.begin(115200); //Open BT serial link
  #endif
  delay(500);

  Serial.print(overall_name);Serial.println(": setup():...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);
  #if (USE_BT_SERIAL)
    BT_SERIAL.print(overall_name);BT_SERIAL.println(": setup():...");
    BT_SERIAL.print("Sample Rate (Hz): "); BT_SERIAL.println(audio_settings.sample_rate_Hz);
    BT_SERIAL.print("Audio Block Size (samples): "); BT_SERIAL.println(audio_settings.audio_block_samples);
  #endif

  // Audio connections require memory
  AudioMemory_F32_wSettings(200,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //update the potentiometer settings
	if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //End of setup
  printGainSettings(&Serial);Serial.println("Setup complete.");serialManager_USB.printHelp();
  #if (USE_BT_SERIAL)
    printGainSettings(&BT_SERIAL); BT_SERIAL.println("Setup complete.");  serialManager_BT.printHelp();
  #endif

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //respond to Serial commands
  while (Serial.available()) serialManager_USB.respondToByte((char)Serial.read());
  #if (USE_BT_SERIAL)
    while (BT_SERIAL.available()) serialManager_BT.respondToByte((char)BT_SERIAL.read());
  #endif

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //check the mic_detect signal
  serviceMicDetect(millis(),500);

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) printCPUandMemory(millis());

  //print info about the signal processing
  updateAveSignalLevels(millis());
  if (enable_printAveSignalLevels) printAveSignalLevels(millis(),printAveSignalLevels_as_dBSPL);

} //end loop()


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(audioHardware.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      setVolKnobGain_dB(val*45.0f - 10.0f - input_gain_dB);
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printGainSettings(void) {
  printGainSettings(&Serial);
  #if (USE_BT_SERIAL)
    printGainSettings(&BT_SERIAL);
  #endif
}
void printGainSettings(Stream *s) {
  s->print("Gain (dB): ");
  s->print("Vol Knob = "); s->print(vol_knob_gain_dB,1);
  s->print(", Input PGA = "); s->print(input_gain_dB,1);
  s->print(", Per-Channel = ");
  for (int i=0; i<N_CHAN; i++) {
    s->print(expCompLim[i].getGain_dB()-vol_knob_gain_dB,1);
    s->print(", ");
  }
  s->println();
}

extern void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
    float prev_vol_knob_gain_dB = vol_knob_gain_dB;
    vol_knob_gain_dB = gain_dB;
    float linear_gain_dB;
    for (int i=0; i<N_CHAN_MAX; i++) {
      linear_gain_dB = vol_knob_gain_dB + (expCompLim[i].getGain_dB()-prev_vol_knob_gain_dB);
      expCompLim[i].setGain_dB(linear_gain_dB);
    }
    printGainSettings();
}


void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111; //some sort of weird value
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = audioHardware.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
    if (cur_val != prev_val) {
      if (cur_val) {
        Serial.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        Serial.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
      }
    }
    prev_val = cur_val;
    lastUpdate_millis = curTime_millis;
  }
}

void printCPUandMemory(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage(&Serial);  //USB Serial
    #if (USE_BT_SERIAL)
      printCPUandMemoryMessage(&BT_SERIAL); //Bluetooth Serial
    #endif
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printCPUandMemoryMessage(Stream *s) {
    s->print("CPU Cur/Peak: ");
    s->print(audio_settings.processorUsage());
    //s->print(AudioProcessorUsage());
    s->print("%/");
    s->print(audio_settings.processorUsageMax());
    //s->print(AudioProcessorUsageMax());
    s->print("%,   ");
    s->print("Dyn MEM Float32 Cur/Peak: ");
    s->print(AudioMemoryUsage_F32());
    s->print("/");
    s->print(AudioMemoryUsageMax_F32());
    s->println();
}

float aveSignalLevels_dBFS[N_CHAN_MAX];
void updateAveSignalLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how often to perform the averaging
  static unsigned long lastUpdate_millis = 0;
  float update_coeff = 0.2;

  //is it time to update the calculations
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    for (int i=0; i<N_CHAN_MAX; i++) { //loop over each band
      aveSignalLevels_dBFS[i] = (1.0-update_coeff)*aveSignalLevels_dBFS[i] + update_coeff*expCompLim[i].getCurrentLevel_dB(); //running average
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevels(unsigned long curTime_millis, bool as_dBSPL) {
  static unsigned long updatePeriod_millis = 3000; //how often to print the levels to the screen
  static unsigned long lastUpdate_millis = 0;

  //is it time to print to the screen
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printAveSignalLevelsMessage(&Serial,as_dBSPL);
    #if (USE_BT_SERIAL)
      printAveSignalLevelsMessage(&BT_SERIAL,as_dBSPL);
    #endif
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevelsMessage(Stream *s, bool as_dBSPL) {
  float offset_dB = 0.0f;
  String units_txt = String("dBFS");
  if (as_dBSPL) {
    offset_dB = overall_cal_dBSPL_at0dBFS;
    units_txt = String("dBSPL, approx");
  }
  s->print("Ave Input Level (");s->print(units_txt); s->print("), Per-Band = ");
  for (int i=0; i<N_CHAN; i++) { s->print(aveSignalLevels_dBFS[i]+offset_dB,1);  s->print(", ");  }
  s->println();
}
