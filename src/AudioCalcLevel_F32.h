
#ifndef _AudioCalcLevel_F32_h
#define _AudioCalcLevel_F32_h

#include "Arduino.h"
#include "AudioFilterTimeWeighting_F32.h"

//Exponential time weighting for sound level meter.  Defaults to SLOW
class AudioCalcLevel_F32 : public AudioFilterTimeWeighting_F32 
{
	public:
		AudioCalcLevel_F32(void) : AudioFilterTimeWeighting_F32() {}		
		
		AudioCalcLevel_F32(const AudioSettings_F32 &settings) : AudioFilterTimeWeighting_F32(settings) {}
		virtual void update(void);
		virtual float getCurrentLevel(void) { return cur_value; } 
		virtual float getCurrentLevel_dB(void) { return 10.0f*log10f(cur_value); } 
		
		
	protected:
		float32_t cur_value = 0.0f;

};

#endif
