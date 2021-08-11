


//define our hearing aid prescription...allow toggling between two presents
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_Alternates.h"  //this sets alternate dsl and gha, which can be switched in via commands

//define the filterbank size
#define N_FIR 96

void setupFromDSLandGHA(const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const int n_chan, const int n_fir, const AudioSettings_F32 &settings)
{
     
  //set the per-channel filter coefficients
  filterbank.designFilters(n_chan, n_fir, settings.sample_rate_Hz, settings.audio_block_samples, (float *)this_dsl.cross_freq);

  //setup all of the per-channel compressors
  compbank.configureFromDSLandGHA(settings.sample_rate_Hz,  this_dsl, this_gha);

  //setup the broad band compressor (limiter)
  //configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, compBroadband);
  compBroadband.configureFromGHA(settings.sample_rate_Hz, this_gha);

  //overwrite the one-point calibration based on the dsl data structure
  myState.overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;
     
}

void setupAudioProcessing(void) {
  //make all of the audio connections
  makeAudioConnections();  //see AudioConnections.h

  //setup processing based on the DSL and GHA prescriptions
  if (myState.current_dsl_config == State::DSL_NORMAL) {
    setupFromDSLandGHA(dsl, gha, N_CHAN, N_FIR, audio_settings);
  } else if (myState.current_dsl_config == State::DSL_FULLON) {
    setupFromDSLandGHA(dsl_fullon, gha_fullon, N_CHAN, N_FIR, audio_settings);
  }
}

float incrementChannelGain(int chan, float increment_dB) {
  if (chan < N_CHAN) (compbank.compressors[chan]).incrementGain_dB(increment_dB);
  printGainSettings();  //in main sketch file
  return compbank.getLinearGain_dB(chan);
}


int setDSLConfiguration(int config) {
  config = max(0,min(myState.MAX_DSL_SETTING, config));
  myState.current_dsl_config = config;
  switch (myState.current_dsl_config) {
    case (State::DSL_NORMAL):
      Serial.println("setDSLConfiguration: changing to NORMAL dsl configuration");
      setupFromDSLandGHA(dsl, gha, N_CHAN, N_FIR, audio_settings);  break;
    case (State::DSL_FULLON):
      Serial.println("setDSLConfiguration: changing to FULL-ON dsl configuration");
      setupFromDSLandGHA(dsl_fullon, gha_fullon, N_CHAN, N_FIR, audio_settings); break;
  } 
  return  myState.current_dsl_config;
}
