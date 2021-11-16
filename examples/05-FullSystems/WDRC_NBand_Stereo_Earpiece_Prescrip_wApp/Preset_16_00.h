/* 
 *  This file defines the hearing prescription for each user.  Put each
 *  user's prescription in the "dsl" (for multi-band parameters) and
 *  "gha" (for broadband limiter at the end) below.  
 */

// //////////////////////////// LEFT SIDE

AudioFilterBiquad_F32_settings preFilt_left = {
  4,      //FilterType: 1=LOWPASS, 2=BANDPASS, 3=HIGHPASS, 4=NOTCH
  0,      //isBypassed: 1 = bypass the filter; 0 = filter is active
  4000.0, //for LOWPASS or HIGPASS: Cutoff Frequency (Hz);  for BANDPASS or NOTCH: Center Frequency (Hz)
  20.0    //Filter Q (width / center)...0.707 for low/high pass, maybe 5-20 for a bandpass or notch
};

// Here is the per-band prescription to be our default behavior of the multi-band processing
BTNRH_WDRC::CHA_DSL dsl_left = {5,  // attack (ms)
  50,      // release (ms)
  115,     // maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,       // 0=left, 1=right...ignored
  16,  // num channels
  {250.,  315.,  400., 500., 630., 800., 1000., 1250., 1600., 2000., 2500., 3150., 4000., 5000., 6300.},         // cross frequencies (Hz)
  {0.7,    0.7,   0.7,  0.7,  0.7,  0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7},  // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0,  30.0,  30.0, 30.0, 30.0, 30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0},  // expansion-end kneepoint
  {-13.0, -16.0, -4.0,  7.0, 11.0, 16.0,  18.0,  20.0,  22.0,  24.0,  26.0,  26.0,  26.0,  26.0,  26.0,  26.0},  // compression-start gain
  {0.7,    0.9,   1.0,  1.1,  1.2,  1.3,   1.4,   1.4,   1.4,   1.5,   1.5,   1.5,   1.5,   1.5,   1.5,   1.5},  // compression ratio
  {32.0,   27.0, 27.0, 27.0, 30.0, 30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0},  // compression-start kneepoint (input dB SPL)
  {79.0,   88.0, 91.0, 93.0, 98.0, 99.0,  100.,  101.,  103.,  102.,  102.,  102.,  102.,  102.,  102.,  102.}   // output limiting threshold (comp ratio 10)
};

// Here are the settings for the broadband limiter at the end.
BTNRH_WDRC::CHA_WDRC bb_left = {1.0, // attack time (ms)
  50.0,     // release time (ms)
  24000.0,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  115.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  105.0,    // compression-start kneepoint...set to some high value to make it not relevant
  10.0,     // compression ratio...set to 1.0 to make linear (to defeat)
  105.0     // output limiting threshold...hardwired to compression ratio of 10.0
};


// //////////////////////////// RIGHT SIDE

AudioFilterBiquad_F32_settings preFilt_right = {
  4,      //FilterType: 1=LOWPASS, 2=BANDPASS, 3=HIGHPASS, 4=NOTCH
  0,      //isBypassed: 1 = bypass the filter; 0 = filter is active
  4000.0, //for LOWPASS or HIGPASS: Cutoff Frequency (Hz);  for BANDPASS or NOTCH: Center Frequency (Hz)
  20.0    //Filter Q (width / center)...0.707 for low/high pass, maybe 5-20 for a bandpass or notch
};

// Here is the per-band prescription to be our default behavior of the multi-band processing
BTNRH_WDRC::CHA_DSL dsl_right = {5,  // attack (ms)
  50,      // release (ms)
  115,     // maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1,       // 0=left, 1=right...ignored
  16,  // num channels
  {250.,  315.,  400., 500., 630., 800., 1000., 1250., 1600., 2000., 2500., 3150., 4000., 5000., 6300.},         // cross frequencies (Hz)
  {0.7,    0.7,   0.7,  0.7,  0.7,  0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7,   0.7},  // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0,  30.0,  30.0, 30.0, 30.0, 30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0},  // expansion-end kneepoint
  {-13.0, -16.0, -4.0,  7.0, 11.0, 16.0,  18.0,  20.0,  22.0,  24.0,  26.0,  26.0,  26.0,  26.0,  26.0,  26.0},  // compression-start gain
  {0.7,    0.9,   1.0,  1.1,  1.2,  1.3,   1.4,   1.4,   1.4,   1.5,   1.5,   1.5,   1.5,   1.5,   1.5,   1.5},  // compression ratio
  {32.0,   27.0, 27.0, 27.0, 30.0, 30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0,  30.0},  // compression-start kneepoint (input dB SPL)
  {79.0,   88.0, 91.0, 93.0, 98.0, 99.0,  100.,  101.,  103.,  102.,  102.,  102.,  102.,  102.,  102.,  102.}   // output limiting threshold (comp ratio 10)
};

// Here are the settings for the broadband limiter at the end.
BTNRH_WDRC::CHA_WDRC bb_right = {1.0, // attack time (ms)
  50.0,     // release time (ms)
  24000.0,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  115.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  105.0,    // compression-start kneepoint...set to some high value to make it not relevant
  10.0,     // compression ratio...set to 1.0 to make linear (to defeat)
  105.0     // output limiting threshold...hardwired to compression ratio of 10.0
};
