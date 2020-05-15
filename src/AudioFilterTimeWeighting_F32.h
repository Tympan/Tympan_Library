
#ifndef _AudioFilterTimeWeighting_F32_h
#define _AudioFilterTimeWeighting_F32_h

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"

#define TIME_CONST_SLOW  (1.0f)
#define TIME_CONST_FAST	 (0.125f)

//Exponential time weighting for traditional (ie, old style) sound level meter.  
//   Default time constant is SLOW
class AudioFilterTimeWeighting_F32 : public AudioStream_F32 
{
	public:
		AudioFilterTimeWeighting_F32(void) : AudioStream_F32(1,inputQueueArray),
			timeConst_sec(TIME_CONST_SLOW) {
			setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		}		
		
		AudioFilterTimeWeighting_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray),
			timeConst_sec(TIME_CONST_SLOW) {
			setSampleRate_Hz(settings.sample_rate_Hz);
		}
		virtual void setTimeConst_sec(float32_t _tau) {
			timeConst_sec = _tau;
			computeFilterCoefficients();
		}
		virtual void setSampleRate_Hz(float32_t _fs_Hz) { 
			sampleRate_Hz = _fs_Hz;
			computeFilterCoefficients();
		}
		virtual float32_t getTimeConst_sec(void) { return timeConst_sec; }
		virtual float32_t getSampleRate_Hz(void) { return sampleRate_Hz; }
		virtual void clearStates(void) { prev_val = 0.0f; }
		virtual void update(void);
		virtual void applyFilterInPlace(float32_t *, int);
	
	protected:
		audio_block_f32_t *inputQueueArray[1];
		float32_t timeConst_sec = TIME_CONST_SLOW;
		float32_t sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT;
		float32_t alpha = 0.0f;
		float32_t prev_val = 0.0f;
		
		virtual void computeFilterCoefficients(void) {
			//https://dsp.stackexchange.com/questions/10544/exponential-average-with-time-constant-of-slow-fast-and-impulse
			alpha = expf(-1.0f / (getSampleRate_Hz() * getTimeConst_sec()));
		}
	
};

#endif
