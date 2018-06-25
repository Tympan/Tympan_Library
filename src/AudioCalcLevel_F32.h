
#ifndef _AudioCalcLevel_F32_h
#define _AudioCalcLevel_F32_h

#include "Arduino.h"
#include "AudioFilterTimeWeighting_F32.h"

//Time weighting for sound level meter.  Defaults to SLOW
class AudioCalcLevel_F32 : public AudioFilterTimeWeighting_F32 
{
	public:
		AudioCalcLevel_F32(void) : AudioFilterTimeWeighting_F32() {}		
		
		AudioCalcLevel_F32(const AudioSettings_F32 &settings) : AudioFilterTimeWeighting_F32(settings) {}
		virtual void update(void);
	
	protected:
	
};

#endif
