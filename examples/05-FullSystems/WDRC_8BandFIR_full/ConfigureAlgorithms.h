
//define the filterbank size
#define N_FIR 96
float firCoeff[N_CHAN][N_FIR];

//define our hearing aid prescription...allow toggling between two presents
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_Alternates.h"  //this sets alternate dsl and gha, which can be switched in via commands


void configureBroadbandWDRCs(float fs_Hz, const BTNRH_WDRC::CHA_WDRC &this_gha, AudioEffectCompWDRC_F32 &WDRC)
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
    WDRC.setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
 // }
}

void configurePerBandWDRCs(int nchan, float fs_Hz,
    const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
    AudioEffectCompWDRC_F32 *WDRCs)
{
  if (nchan > this_dsl.nchannel) {
    Serial.println(F("configurePerBandWDRCs: *** ERROR ***: nchan > dsl.nchannel"));
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

void setupFromDSLandGHA(const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
     const int n_chan, const int n_fir, const AudioSettings_F32 &settings)
{

  //compute the per-channel filter coefficients
  AudioConfigFIRFilterBank_F32 makeFIRcoeffs(n_chan, n_fir, settings.sample_rate_Hz, (float *)this_dsl.cross_freq, (float *)firCoeff);

  //set the coefficients (if we lower n_chan, we should be sure to clean out the ones that aren't set)
  for (int i=0; i< n_chan; i++) firFilt[i].begin(firCoeff[i], n_fir, settings.audio_block_samples);

  //setup all of the per-channel compressors
  configurePerBandWDRCs(n_chan, settings.sample_rate_Hz, this_dsl, this_gha, expCompLim);

  //setup the broad band compressor (limiter)
  configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, compBroadband);

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
  if (chan < N_CHAN) expCompLim[chan].incrementGain_dB(increment_dB);
  printGainSettings();  //in main sketch file
  return expCompLim[chan].getGain_dB();
}

int incrementDSLConfiguration(void) {
  myState.incrementDSLindex();
  switch (myState.current_dsl_config) {
    case (State::DSL_NORMAL):
      Serial.println("incrementDSLConfiguration: changing to NORMAL dsl configuration");
      setupFromDSLandGHA(dsl, gha, N_CHAN, N_FIR, audio_settings);  break;
    case (State::DSL_FULLON):
      Serial.println("incrementDSLConfiguration: changing to FULL-ON dsl configuration");
      setupFromDSLandGHA(dsl_fullon, gha_fullon, N_CHAN, N_FIR, audio_settings); break;
  }
  return myState.current_dsl_config;
}
