
/*
//define our hearing aid prescription...allow toggling between two presets
#include "Preset_00.h"  //this sets dsl and broadband settings, which will be the defaults
#include "Preset_01.h"  //this sets alternative dsl and broadband, which can be switched in via commands

//make SD-enabled versions of tha prescription-holding objects
BTNRH_WDRC::CHA_DSL_SD dsl_left_sd(dsl_left);
BTNRH_WDRC::CHA_DSL_SD dsl_right_sd(dsl_right);
BTNRH_WDRC::CHA_DSL_SD dsl_fullon_left_sd(dsl_fullon_left);
BTNRH_WDRC::CHA_DSL_SD dsl_fullon_right_sd(dsl_fullon_right);
BTNRH_WDRC::CHA_WDRC_SD bb_left_sd(bb_left);
BTNRH_WDRC::CHA_WDRC_SD bb_right_sd(bb_right);
BTNRH_WDRC::CHA_WDRC_SD bb_fullon_left_sd(bb_fullon_left);
BTNRH_WDRC::CHA_WDRC_SD bb_fullon_right_sd(bb_fullon_right);
*/



//define the filterbank size
#define N_FILT_ORDER 96   //96 for FIR, 6 fir IIR

void setupFromDSLandGHA(const int Ichan, const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const int n_chan, const int n_filt_order, const AudioSettings_F32 &settings)
{ 
  //set the per-channel filter coefficients (using our filterbank class)
  multiBandWDRC[Ichan].filterbank.designFilters(n_chan, n_filt_order, settings.sample_rate_Hz, settings.audio_block_samples, (float *)this_dsl.cross_freq);

  //setup all of the per-channel compressors (using our compressor bank class)
  multiBandWDRC[Ichan].compbank.configureFromDSLandGHA(settings.sample_rate_Hz,  this_dsl, this_gha);

  //setup the broad band compressor (typically used as a limiter)
  //configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, compBroadband);
  multiBandWDRC[Ichan].compBroadband.configureFromGHA(settings.sample_rate_Hz, this_gha);
  
  //overwrite the one-point SPL calibration based on the value in the DSL data structure
  // THIS NEEDS TO BE RECONSIDERED NOW THAT WE'RE STEREO!!!
  myState.overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;
     
}

void setupAudioProcessing(void) {
   
  //make all of the audio connections
  makeAudioConnections();  //see AudioConnections.h

  //setup processing based on the DSL and GHA prescriptions for the default prescription choice
  setDSLConfiguration(myState.current_dsl_config);
}

float incrementChannelGain(int chan, float increment_dB) {
  if (chan < N_CHAN) {
    for (int Ichan = StereoContainer_UI::LEFT; Ichan <= StereoContainer_UI::RIGHT; Ichan++) {
      (multiBandWDRC[Ichan].compbank.compressors[chan]).incrementGain_dB(increment_dB);
    }
  }
  printGainSettings();  //in main sketch file
  return multiBandWDRC[StereoContainer_UI::LEFT].compbank.getLinearGain_dB(chan);
}


int setDSLConfiguration(int config) {
  if ((config < 0) || (config >= presetManager.n_presets)) return  myState.current_dsl_config;  //do nothing

  BTNRH_Preset preset = presetManager.presets[config];
  myState.current_dsl_config = config;

  //loop over left and right and provide the settings
  for (int Ichan = StereoContainer_UI::LEFT; Ichan <= StereoContainer_UI::RIGHT; Ichan++) {
    setupFromDSLandGHA(Ichan, preset.wdrc_perBand[Ichan], preset.wdrc_broadband[Ichan], N_CHAN, N_FILT_ORDER, audio_settings);
  }
  return  myState.current_dsl_config;
}
