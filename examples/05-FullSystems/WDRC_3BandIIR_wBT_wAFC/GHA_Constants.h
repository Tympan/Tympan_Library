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
  3,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {700.0, 2400.0,       1.e4, 1.e4, 1.e4, 1.e4, 1.e4},   // cross frequencies (Hz)...FOR IIR FILTERING, THESE VALUES ARE IGNORED!!!
  {0.57, 0.57, 0.57,     1.0, 1.0, 1.0, 1.0, 1.0},   // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {40.0, 33.0, 34.0,     34., 34., 34., 34., 34.},   // expansion-end kneepoint
  {20.f, 25.f, 30.f,     30.f, 30.f, 30.f, 30.f, 30.f},   // compression-start gain
  {1.5f, 1.5f, 1.5f,     1.5f, 1.5f, 1.5f, 1.5f, 1.5f},   // compression ratio
  {50.0, 50.0, 50.0,     50.0, 50.0, 50.0, 50.0, 50.0},   // compression-start kneepoint (input dB SPL)
  {90.f, 90.f, 90.f,     90.f, 90.f, 91.f, 92.f, 93.f}    // output limiting threshold (comp ratio 10)
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

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc = {   
  1, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl, length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho, smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps, when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};



