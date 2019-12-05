/* 
 *  This file defines the hearing prescription for each user.  Put each
 *  user's prescription in the "dsl" (for multi-band parameters) and
 *  "gha" (for broadband limiter at the end) below.  
 */

#include <AudioEffectCompWDRC_F32.h>  //for CHA_DSL and CHA_WDRC data types

// Here is the per-band prescription that is the default behavior of the multi-band
// processing.  This sounded decent to WEA's ears but YMMV.
BTNRH_WDRC::CHA_DSL dsl = {5,  // attack (ms)
  300,  // release (ms)
  115,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // 0=left, 1=right...ignored
  8,    //num channels...ignored.  8 is always assumed
  {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7},   // cross frequencies (Hz)
  {0.57, 0.57, 0.57, 0.57, 0.57, 0.57, 0.57, 0.57},   // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {45.0, 45.0, 33.0, 32.0, 36.0, 34.0, 36.0, 40.0},   // expansion-end kneepoint
  {20.f, 20.f, 25.f, 30.f, 30.f, 30.f, 30.f, 30.f},   // compression-start gain
  {1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f},   // compression ratio
  {50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0},   // compression-start kneepoint (input dB SPL)
  {90.f, 90.f, 90.f, 90.f, 90.f, 91.f, 92.f, 93.f}    // output limiting threshold (comp ratio 10)
};

// Here are the settings for the broadband limiter at the end.
// Again, it sounds OK to WEA, but YMMV.
//from GHA_Demo.c  from "amplify()"   Used for broad-band limiter.
BTNRH_WDRC::CHA_WDRC gha = {5.f, // attack time (ms)
  300.f,    // release time (ms)
  24000.f,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  115.f,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.f,      // compression-start gain....set to zero for pure limitter
  115.f,    // compression-start kneepoint...set to some high value to make it not relevant
  1.f,      // compression ratio...set to 1.0 to make linear (to defeat)
  98.0      // output limiting threshold...hardwired to compression ratio of 10.0
};




