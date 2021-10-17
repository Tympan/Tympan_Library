/* 
 *  This file defines the hearing prescription for each user.  Put each
 *  user's prescription in the "dsl" (for multi-band parameters) and
 *  "gha" (for broadband limiter at the end) below.  
 */

// //////////////////////////// LEFT SIDE

// Here is the per-band prescription to be our default behavior of the multi-band processing
BTNRH_WDRC::CHA_DSL dsl_left = {5,  // attack (ms)
  50,      // release (ms)
  115,     // maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,       // 0=left, 1=right...ignored
  8,  // num channels
  {375., 750., 1125., 1500., 1875., 2250., 2625., 3000., 3375., 3750., 4125., 4500., 4875., 5250., 5625.},   // cross frequencies (Hz)
  {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},                  // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0},  // expansion-end kneepoint.  not relevant when there is no compression
  {40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0},  // compression-start gain
  {1.0,   1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0},  // compression ratio.  Set to 1.0 to defeat.
  {50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0},  // compression-start kneepoint (input dB SPL).  not relevant when there is no compression
  {200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0}  // output limiting threshold (comp ratio 10).  Set to large value to defeat
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

// Here is the per-band prescription to be our default behavior of the multi-band processing
BTNRH_WDRC::CHA_DSL dsl_right = {5,  // attack (ms)
  50,      // release (ms)
  115,     // maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1,       // 0=left, 1=right...ignored
  8,  // num channels
  {375., 750., 1125., 1500., 1875., 2250., 2625., 3000., 3375., 3750., 4125., 4500., 4875., 5250., 5625.},   // cross frequencies (Hz)
  {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},                  // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0},  // expansion-end kneepoint.  not relevant when there is no compression
  {40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0},  // compression-start gain
  {1.0,   1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0},  // compression ratio.  Set to 1.0 to defeat.
  {50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0},  // compression-start kneepoint (input dB SPL).  not relevant when there is no compression
  {200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0,200.0}  // output limiting threshold (comp ratio 10).  Set to large value to defeat
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
