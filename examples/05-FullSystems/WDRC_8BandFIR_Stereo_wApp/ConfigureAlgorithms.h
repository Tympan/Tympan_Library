


//define our hearing aid prescription...allow toggling between two presents
#include "DSL_GHA_Preset0.h"  //this sets dsl and gha settings, which will be the defaults
#include "DSL_GHA_Preset1.h"  //this sets alternative dsl and gha, which can be switched in via commands

//define the filterbank size
#define N_FILT_ORDER 6

void setupFromDSLandGHA(const int Ichan, const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const int n_chan, const int n_filt_order, const AudioSettings_F32 &settings)
{
  //for (int Ichan = StereoContainer_UI::LEFT; Ichan <= StereoContainer_UI::RIGHT; Ichan++) {
    //set the per-channel filter coefficients (using our filterbank class)
    multiBandWDRC[Ichan].filterbank.designFilters(n_chan, n_filt_order, settings.sample_rate_Hz, settings.audio_block_samples, (float *)this_dsl.cross_freq);

    //setup all of the per-channel compressors (using our compressor bank class)
    multiBandWDRC[Ichan].compbank.configureFromDSLandGHA(settings.sample_rate_Hz,  this_dsl, this_gha);
  
    //setup the broad band compressor (typically used as a limiter)
    //configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, compBroadband);
    multiBandWDRC[Ichan].compBroadband.configureFromGHA(settings.sample_rate_Hz, this_gha);
    
  //}
  
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
  config = max(0,min(myState.MAX_DSL_SETTING, config));
  myState.current_dsl_config = config;
  switch (myState.current_dsl_config) {
    case (State::DSL_NORMAL):
      Serial.println("setDSLConfiguration: changing to NORMAL dsl configuration");
      setupFromDSLandGHA(StereoContainer_UI::LEFT,  dsl_left,         gha_left,         N_CHAN, N_FILT_ORDER, audio_settings);
      setupFromDSLandGHA(StereoContainer_UI::RIGHT, dsl_right,        gha_right,        N_CHAN, N_FILT_ORDER, audio_settings);
    case (State::DSL_FULLON):
      Serial.println("setDSLConfiguration: changing to FULL-ON dsl configuration");
      setupFromDSLandGHA(StereoContainer_UI::LEFT,  dsl_fullon_left,  gha_fullon_left,  N_CHAN, N_FILT_ORDER, audio_settings);
      setupFromDSLandGHA(StereoContainer_UI::RIGHT, dsl_fullon_right, gha_fullon_right, N_CHAN, N_FILT_ORDER, audio_settings);
  } 
  return  myState.current_dsl_config;
}
