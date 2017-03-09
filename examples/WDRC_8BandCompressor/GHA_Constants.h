
#include <AudioEffectCompWDRC_F32.h>  //for CHA_DSL and CHA_WDRC data types

//from GHA_Demo.c
// DSL prescription - (first subject, left ear) from LD_RX.mat
static BTNRH_WDRC::CHA_DSL dsl = {5,  //attack (ms)
  50,  //release (ms)
  119, //max dB
  0, // 0=left, 1=right
  8, //num channels
  {317.1666,502.9734,797.6319,1264.9,2005.9,3181.1,5044.7},   // cross frequencies (Hz)
  {-13.5942,-16.5909,-3.7978,6.6176,11.3050,23.7183,35.8586,37.3885},  // compression-start gain
  {0.7,0.9,1,1.1,1.2,1.4,1.6,1.7},   // compression ratio
  {32.2,26.5,26.7,26.7,29.8,33.6,34.3,32.7},    // compression-start kneepoint
  {78.7667,88.2,90.7,92.8333,98.2,103.3,101.9,99.8}  // broadband output limiting threshold
};

//from GHA_Demo.c  from "amplify()"
BTNRH_WDRC::CHA_WDRC gha = {1.f, // attack time (ms)
  50.f,     // release time (ms)
  24000.f,  // sampling rate (Hz)
  119.f,    // maximum signal (dB SPL)
  0.f,      // compression-start gain
  105.f,    // compression-start kneepoint
  10,     // compression ratio
  105     // broadband output limiting threshold
};

