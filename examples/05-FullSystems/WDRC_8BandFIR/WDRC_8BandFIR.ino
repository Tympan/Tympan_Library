/*
  WDRC_8BandFIR

  Created: Chip Audette (OpenAudio), Feb 2017
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 8-band WDRC compressor.  The BTNRH version was implemented the
    filters in the frequency-domain, whereas I implemented them as FIR filters
	in the time-domain. I've also added an expansion stage to manage noise at very
	low SPL.  Communicates via USB Serial and via Bluetooth Serial

  User Controls:
    Potentiometer on Tympan controls the algorithm gain

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>
#include "SerialManager.h"

// Define the overall setup
String overall_name = String("Tympan: WDRC Expander-Compressor-Limiter with Overall Limiter");
const int N_CHAN_MAX = 8;  //number of frequency bands (channels)
int N_CHAN = N_CHAN_MAX;  //will be changed to user-selected number of channels later
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0; //will be overridden by volume knob

int USE_VOLUME_KNOB = 1;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below

const float sample_rate_Hz = 24000.0f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::D,audio_settings);     //do TympanRev::C or TympanRev::D
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //move this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
AudioFilterFIR_F32          firFilt[N_CHAN_MAX];      //here are the filters to break up the audio into multiple bands
AudioEffectCompWDRC_F32     expCompLim[N_CHAN_MAX];   //here are the per-band compressors
AudioMixer8_F32             mixer1;                   //mixer to reconstruct the broadband audio
AudioEffectCompWDRC_F32     compBroadband;            //broadband compressor
AudioOutputI2S_F32          i2s_out(audio_settings);  //Digital audio output to the DAC.  Should be last.

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32   audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_FIR(audio_settings);
AudioControlTestAmpSweep_F32     ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_FIR(audio_settings,audioTestGenerator,audioTestMeasurement_FIR);

//make the audio connections
#define N_MAX_CONNECTIONS 100  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count=0;

  //connect input
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioTestGenerator, 0); 

  //make the connection for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_FIR, 0);

  //make per-channel connections
  for (int i = 0; i < N_CHAN; i++) {
    //audio connections
    patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, firFilt[i], 0); //connect to FIR filter
    patchCord[count++] = new AudioConnection_F32(firFilt[i], 0, expCompLim[i], 0); //connect filter to compressor
    patchCord[count++] = new AudioConnection_F32(expCompLim[i], 0, mixer1, i); //connect to mixer

    //make the connection for the audio test measurements
    patchCord[count++] = new AudioConnection_F32(firFilt[i], 0, audioTestMeasurement_FIR, 1+i);
  }

  //connect the output of the mixers to the final broadband compressor
  patchCord[count++] = new AudioConnection_F32(mixer1, 0, compBroadband, 0);  //connect to final limiter

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
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) { enable_printAveSignalLevels = !enable_printAveSignalLevels; printAveSignalLevels_as_dBSPL = as_dBSPL;};
SerialManager serialManager(N_CHAN,expCompLim,ampSweepTester,freqSweepTester,freqSweepTester_FIR);


//routine to setup the hardware
void setupTympanHardware(void) {
  myTympan.println("Setting up Tympan Audio Board...");
  myTympan.enable(); // activate AIC

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  myTympan.enableMicDetect(true);

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop() 
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board micropphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //set volumes
  myTympan.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}


// /////////// setup the audio processing
//define functions to setup the audio processing parameters
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_Alternates.h"  //this sets alternate dsl and gha, which can be switched in via commands
#define DSL_NORMAL 0
#define DSL_FULLON 1
int current_dsl_config = DSL_NORMAL; //used to select one of the configurations above for startup
float overall_cal_dBSPL_at0dBFS; //will be set later

//define the filterbank size
#define N_FIR 96
float firCoeff[N_CHAN_MAX][N_FIR];


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
    myTympan.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    myTympan.print(F("    : nchan = ")); myTympan.println(nchan);
    myTympan.print(F("    : dsl.nchannel = ")); myTympan.println(dsl.nchannel);
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

void setupFromDSLandGHA(const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const int n_chan_max, const int n_fir, const AudioSettings_F32 &settings)
{
  int n_chan = n_chan_max;  //maybe change this to be the value in the DSL itself.  other logic would need to change, too.

  //compute the per-channel filter coefficients
  AudioConfigFIRFilterBank_F32 makeFIRcoeffs(n_chan, n_fir, settings.sample_rate_Hz, (float *)this_dsl.cross_freq, (float *)firCoeff);

  //set the coefficients (if we lower n_chan, we should be sure to clean out the ones that aren't set)
  for (int i=0; i< n_chan; i++) firFilt[i].begin(firCoeff[i], n_fir, settings.audio_block_samples);

  //setup all of the per-channel compressors
  configurePerBandWDRCs(n_chan, settings.sample_rate_Hz, this_dsl, this_gha, expCompLim);

  //setup the broad band compressor (limiter)
  configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, vol_knob_gain_dB, compBroadband);

  //overwrite the one-point calibration based on the dsl data structure
  overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;

}


void setupAudioProcessing(void) {
  //make all of the audio connections
  makeAudioConnections();

  //setup processing based on the DSL and GHA prescriptions
  if (current_dsl_config == DSL_NORMAL) {
    setupFromDSLandGHA(dsl, gha, N_CHAN_MAX, N_FIR, audio_settings);
  } else if (current_dsl_config == DSL_FULLON) {
    setupFromDSLandGHA(dsl_fullon, gha_fullon, N_CHAN_MAX, N_FIR, audio_settings);
  }
}

void incrementDSLConfiguration(void) {
  current_dsl_config++;
  if (current_dsl_config==2) current_dsl_config=0;
  switch (current_dsl_config) {
    case (DSL_NORMAL):
      myTympan.println("incrementDSLConfiguration: changing to NORMAL dsl configuration");
      setupFromDSLandGHA(dsl, gha, N_CHAN_MAX, N_FIR, audio_settings);  break;
    case (DSL_FULLON):
      myTympan.println("incrementDSLConfiguration: changing to FULL-ON dsl configuration");
      setupFromDSLandGHA(dsl_fullon, gha_fullon, N_CHAN_MAX, N_FIR, audio_settings); break;
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  myTympan.print(overall_name);myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory_F32(40,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //update the potentiometer settings
	if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //End of setup
  printGainSettings();
  myTympan.println("Setup complete.");
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  //asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //Bluetooth

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //check the mic_detect signal
  serviceMicDetect(millis(),500);

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000);  //print every 3000 msec

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
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
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
  myTympan.print("Gain (dB): ");
  myTympan.print("Vol Knob = "); myTympan.print(vol_knob_gain_dB,1);
  myTympan.print(", Input PGA = "); myTympan.print(input_gain_dB,1);
  myTympan.print(", Per-Channel = ");
  for (int i=0; i<N_CHAN; i++) {
    myTympan.print(expCompLim[i].getGain_dB()-vol_knob_gain_dB,1);
    myTympan.print(", ");
  }
  myTympan.println();
}

extern void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
    float prev_vol_knob_gain_dB = vol_knob_gain_dB;
    vol_knob_gain_dB = gain_dB;
    float linear_gain_dB;
    for (int i=0; i<N_CHAN; i++) {
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

    cur_val = myTympan.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
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
    printAveSignalLevelsMessage(as_dBSPL);
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevelsMessage(bool as_dBSPL) {
  float offset_dB = 0.0f;
  String units_txt = String("dBFS");
  if (as_dBSPL) {
    offset_dB = overall_cal_dBSPL_at0dBFS;
    units_txt = String("dBSPL, approx");
  }
  myTympan.print("Ave Input Level (");myTympan.print(units_txt); myTympan.print("), Per-Band = ");
  for (int i=0; i<N_CHAN; i++) { myTympan.print(aveSignalLevels_dBFS[i]+offset_dB,1);  myTympan.print(", ");  }
  myTympan.println();
}
