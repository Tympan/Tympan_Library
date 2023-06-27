
#ifndef _AudioCalcLevel_F32_h
#define _AudioCalcLevel_F32_h

#include <Arduino.h>
#include "AudioFilterTimeWeighting_F32.h"

/*
 AudioCalcLeq_F32.h
 
 Chip Audette, OpenAudio, Updated Comments Jan 2023
 
 Exponential time weighting for sound level meter.  Defaults to SLOW
	* Squares the incoming signal
    * Applies a low-pass filter to the squared signal (via the time constant that you specify)
    * The output of the low-pass filter is the estimated level of the signal
	
  MIT License,  Use at your own risk.	
*/
class AudioCalcLevel_F32 : public AudioFilterTimeWeighting_F32 
{
	public:
		AudioCalcLevel_F32(void) : AudioFilterTimeWeighting_F32() {}		
		
		AudioCalcLevel_F32(const AudioSettings_F32 &settings) : AudioFilterTimeWeighting_F32(settings) {}
		virtual void update(void);
		virtual float getCurrentLevel(void) { return cur_value; } 
		virtual float getCurrentLevel_dB(void) { return 10.0f*log10f(cur_value); } 
		virtual float getMaxLevel(void) { return max_value; }
		virtual float getMaxLevel_dB(void) { return 10.0f*log10f(max_value); }
		virtual void  resetMaxLevel(void) { max_value = cur_value; }
		
	protected:
		float32_t cur_value = 0.0f;
		float32_t max_value = 0.0f;

};

#endif
