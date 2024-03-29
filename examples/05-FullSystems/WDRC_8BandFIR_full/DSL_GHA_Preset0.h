/* 
 *  This file defines the hearing prescription for each user.  Put each
 *  user's prescription in the "dsl" (for multi-band parameters) and
 *  "gha" (for broadband limiter at the end) below.  
 */





// Here is the per-band prescription to be our default behavior of the multi-band processing
BTNRH_WDRC::CHA_DSL dsl = {5,  // attack (ms)
  50,      // release (ms)
  115,     // maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,       // 0=left, 1=right...ignored
  N_CHAN,  // num channels
  {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7},   // cross frequencies (Hz)
  {0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7, 0.7},           // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0},   // expansion-end kneepoint
  {-13.0, -16.0, -4.0, 7.0, 11.0, 24.0, 36.0, 37.0},  // compression-start gain
  {0.7f, 0.9f, 1.0f, 1.1f, 1.2f, 1.4f, 1.5f, 1.5f},   // compression ratio
  {32.0, 27.0, 27.0, 27.0, 30.0, 30.0, 30.0, 30.0},   // compression-start kneepoint (input dB SPL)
  {79.f, 88.f, 91.f, 93.f, 98.f, 103.f, 102.f, 100.f} // output limiting threshold (comp ratio 10)
};

// Here are the settings for the broadband limiter at the end.
BTNRH_WDRC::CHA_WDRC gha = {1.0, // attack time (ms)
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
