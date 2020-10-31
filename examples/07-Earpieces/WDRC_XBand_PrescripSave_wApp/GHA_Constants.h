/* 
 *  This file defines the hearing prescription for each user.  Put each
 *  user's prescription in the "dsl" (for multi-band parameters) and
 *  "gha" (for broadband limiter at the end) below.  
 */

#include <AudioEffectCompWDRC_F32.h>  //links to "BTNRH_WDRC_Types.h", which sets the CHA_DSL and CHA_WDRC data types

// Here is the per-band prescription that is the default behavior of the multi-band processing.
BTNRH_WDRC::CHA_DSL dsl = {
  5.0,  // attack (ms)
  300.0,  // release (ms)
  130.0,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // Choose which earpiece to listen to: 0=left, 1=right
  6,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {500.0, 840.0, 1420.0, 2500.0, 5000.0,            1.e4,  1.e4, 1.e4}, // cross frequencies (Hz)
  {  0.7,   0.7,    0.7,    0.7,    0.7,   0.7,      1.0,   1.0},       // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  { 50.0,  45.0,   45.0,   45.0,   45.0,  45.0,     10.0,  10.0},       // expansion-end kneepoint (Input dB SPL) (Note: Disabled by setting the ratio to 1.0)
  {  7.0,  10.0,   20.0,   20.0,   25.0,  25.0,     10.0,  10.0},       // compression-start gain
  {  1.1,   1.2,    1.5,    1.5,    1.5,   1.5,     1.00,  1.00},       // compression ratio (disabled by setting ratio to 1.)
  { 55.0,  55.0,   55.0,   55.0,   55.0,  55.0,     55.0,  55.0},       // compression-start kneepoint (input dB SPL)
  {140.0, 140.0,  140.0,  140.0,  140.0, 140.0,    140.0, 140.0}        // output limiting threshold (input dB SPL) (comp ratio 10) (Disabled by setting to high value)
};

// Here are the settings for the broadband limiter at the end.
// Again, it sounds OK to WEA, but YMMV.
//from GHA_Demo.c  from "amplify()"   Used for broad-band limiter.
BTNRH_WDRC::CHA_WDRC gha = {
  5.0,      // attack time (ms)
  300.0,    // release time (ms)
  22050.0,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  130.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  200.0,    // compression-start kneepoint...set to some high value to make it not relevant (Disable by setting knee to high value)
  1.0,      // compression ratio...set to 1.0 to make linear (Bug: Is still enabled when ratio set to 1.0; to disable, set knee high 
  110.0     // output limiting threshold...hardwired to compression ratio of 10.0
};

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc = {   
  0, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl (100?), length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu (1.0e-3?), scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho (0.9?), smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps (0.008?), when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> FULL-ON GAIN

/*
 *  Here is an alternate configuration for the algorithms.  The intention
 *  is that these settings would be for the "full-on gain" condition for ANSI
 *  testing.  These values shown are place-holders until values can be
 *  tested and improved experimentally
*/

// these values should be probably adjusted experimentally until the gains run right up to 
// the point of the output sounding bad.  The only thing to adjust is the compression-start gain.

//per-band processing parameters...all compression/expansion is defeated.  Just gain is left.
BTNRH_WDRC::CHA_DSL dsl_fullon = {
  5.0,  // attack (ms)
  300.0,  // release (ms)
  130.0,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // Choose which earpiece to listen to: 0=left, 1=right
  6,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {500.0, 840.0, 1420.0, 2500.0, 5000.0,             1.e4, 1.e4, 1.e4},// cross frequencies (Hz)
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,      1.0, 1.0},       // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  { 10.0,  10.0,   10.0,   10.0,   10.0,   10.0,     10.0, 10.0},      // expansion-end kneepoint (Input dB SPL) (Note: Disabled by setting the ratio to 1.0)
  { 27.0,  30.0,   40.0,   40.0,   45.0,   48.0,     10.f, 10.f},      // compression-start gain
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,     1.0f, 1.0f},      // compression ratio(Note: Disable by setting the ratio to 1.0)
  { 80.0,  80.0,   80.0,   80.0,   80.0,   80.0,     80.0, 80.0},      // compression-start kneepoint (input dB SPL) 
  {140.0, 140.0,  140.0,  140.0,  140.0, 140.0,    140.0, 140.0}        // output limiting threshold (input dB SPL) (comp ratio 10) (Disabled by setting to high value)
};


// Here is the broadband limiter for the full-on gain condition.  Only the "bolt" (last value) needs to be iterated.
BTNRH_WDRC::CHA_WDRC gha_fullon = {
  5.0,      // attack time (ms)
  300.0,    // release time (ms)
  22050.0,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  130.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  200.0,    // compression-start kneepoint...set to some high value to make it not relevant (Disable by setting knee to high value)
  1.0,      // compression ratio...set to 1.0 to make linear (Bug: Is still enabled when ratio set to 1.0; to disable, set knee high 
  110.0     // output limiting threshold...hardwired to compression ratio of 10.0
};

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc_fullon = {   
  0, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl (100?), length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu (1.0e-3?), scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho (0.9?), smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps (0.008?), when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> RTS

/*
 *  Here is an alternate configuration for the algorithms.  The intention
 *  is that these settings would be for the "RTS" condition for ANSI
 *  testing.  These values shown are place-holders until values can be
 *  tested and improved experimentally
*/

// these values are based upon the f

//per-band processing parameters...all compression/expansion is defeated.  Just gain is left.
BTNRH_WDRC::CHA_DSL dsl_rts = {
  5.0,  // attack (ms)
  300.0,  // release (ms)
  130.0,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // Choose which earpiece to listen to: 0=left, 1=right
  6,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {500.0, 840.0, 1420.0, 2500.0, 5000.0,            1.e4, 1.e4, 1.e4},// cross frequencies (Hz)
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,     1.0, 1.0},       // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  { 10.0,  10.0,   10.0,   10.0,   10.0,   10.0,    10.0, 10.0},      // expansion-end kneepoint (Input dB SPL) (Note: Disabled by setting the ratio to 1.0)
  { 22.0,  25.0,   35.0,   35.0,   40.0,   43.0,    10.f, 10.f},      // compression-start gain
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,    1.0f, 1.0f},      // compression ratio(Note: Disabled by setting the ratio to 1.0)
  { 80.0,  80.0,   80.0,   80.0,   80.0,   80.0,    80.0, 80.0},      // compression-start kneepoint (input dB SPL) 
  {140.0, 140.0,  140.0,  140.0,  140.0, 140.0,    140.0, 140.0}        // output limiting threshold (input dB SPL) (comp ratio 10) (Disabled by setting to high value)
};


// Here is the broadband limiter for the full-on gain condition.  Only the "bolt" (last value) needs to be iterated.
BTNRH_WDRC::CHA_WDRC gha_rts = {
  5.0,       // attack time (ms)
  300.0,    // release time (ms)
  22050.0,  // sampling rate (Hz)...IGNORED.  (Set globally in the main program.)
  130.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  200.0,    // compression-start kneepoint...set to some high value to make it not relevant (Disable by setting knee to high value)
  1.0,      // compression ratio...set to 1.0 to make linear (Bug: Is still enabled when ratio set to 1.0; to disable, set knee high 
  110.0     // output limiting threshold...hardwired to compression ratio of 10.0
};

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc_rts = {   
  0, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl (100?), length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu (1.0e-3?), scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho (0.9?), smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps (0.008?), when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};
