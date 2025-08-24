
#ifndef _AudioCalcLeq_F32_h
#define _AudioCalcLeq_F32_h

#include "Arduino.h"
#include "AudioStream_F32.h"
#include <arm_math.h>


/*
 AudioCalcLeq_F32.h

 Chip Audette, OpenAudio, May 2020
 
 Windowed time averaging Leq for sound level meter.  Defaults to SLOW
	* Squares the incoming signal
	* Add to a cumulative `running_sum`
    * Compute average value of the squared signal [see note]
    * The output of the low-pass filter is the estimated level of the signal
 
 Note: This routine calculates a new value every time the calculation window has
   been complted.  So, if the time window is 0.125 sec, it calculates an updated
   value every 0.125 seconds.  In other words, it does NOT do a rolling average
   of the last 0.125 seconds of data for every audio sample.  That would be too
   many calculations!

  The default calculation window is 0.125 seconds.

  To get the equivalent sound level since the effect was active, use
  `getCumLevelRms()` or `getCumLevelRms_dB`. This is cleared using resetCumLeq().
  This function relies on counting samples, which will wrap.  To check this, call `isCumLeqValid()`

  To get the peak level since the effect was active, use `getPeakLevel()`. 
  To reset the peak level, call `resetPeakLvl()`
  
  MIT License,  Use at your own risk.
*/
#define CALC_LEQ_DEFAULT_SEC  (0.125)
class AudioCalcLeq_F32 : public AudioStream_F32 
{
	public:
		AudioCalcLeq_F32(void) : AudioStream_F32(1,inputQueueArray) {
			setTimeWindow_sec(CALC_LEQ_DEFAULT_SEC);
			setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		}		
		AudioCalcLeq_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) {
			setTimeWindow_sec(CALC_LEQ_DEFAULT_SEC);
			setSampleRate_Hz(settings.sample_rate_Hz);
		}
		
		virtual void update(void);
		virtual float getCurrentLevel(void) { return cur_value; } 
		virtual float getCurrentLevel_dB(void) { return 10.0f*log10f(cur_value); } 
		virtual float getMaxLevel(void) { return max_value; }
		virtual float getMaxLevel_dB(void) { return 10.0f*log10f(max_value); }
		virtual void  resetMaxLevel(void) { max_value = cur_value; }
		bool getCumLevelRms (float32_t &cumLeq);
		bool getCumLevelRms_dB (float32_t &cumLeq);
		float32_t getPeakLevel(void) const;
		float32_t getPeakLevel_dB(void) const;
		
		virtual float32_t setTimeWindow_sec(float32_t window_sec) {
			givenTimeWindow_sec = window_sec;
			unsigned long samp = (unsigned long)(window_sec * sampleRate_Hz + 0.5);
			setTimeWindow_samp(samp);
			return getTimeWindow_sec();
		}
		virtual void setSampleRate_Hz(float32_t _fs_Hz) { 
			sampleRate_Hz = _fs_Hz;
			setTimeWindow_sec(givenTimeWindow_sec);  //recomputes the window time in samples.  And resets the states.
		}
		virtual float32_t getTimeWindow_sec(void) { return ((float32_t)timeWindow_samp) / sampleRate_Hz; } //the actual precise time window due to quantization
		virtual float32_t getGivenTimeWindow_sec(void) { return givenTimeWindow_sec; } //this returns the value that was first given by the user
		virtual unsigned long getTimeWindow_samp(void) { return timeWindow_samp; }
		virtual float32_t getSampleRate_Hz(void) { return sampleRate_Hz; }
		virtual void clearStates(void) { 
			running_sum = 0.0f; 
			points_averaged = 0; //don't clear cur_value, however
		}  
		
		virtual void resetCumLeq(void) {
			running_sum_of_avg = 0.0f;
			num_averages = 0;
			cumLeqValid = true;	
		}
	
		virtual void resetPeakLvl(void) {
			peak_level_sq = 0.0f;
		}

		bool isCumLeqValid(void) const {
			return (cumLeqValid);
		}

	protected:
		audio_block_f32_t *inputQueueArray[1];
		
		//here are the settings
		float32_t givenTimeWindow_sec = CALC_LEQ_DEFAULT_SEC;
		unsigned long timeWindow_samp = (unsigned long)(CALC_LEQ_DEFAULT_SEC * AUDIO_SAMPLE_RATE_EXACT);
		float32_t sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT;

		//here are the averaging states
		float32_t cur_value = 0.0f;
		float32_t running_sum = 0.0f;
		unsigned long points_averaged = 0;
		float32_t max_value = 0.0f;
		
		// For Cumulative Leq
		float32_t running_sum_of_avg = 0.0f;  	// This stores a running sum of power levels (mean_val^2)
		unsigned long long num_averages = 0;			// # of values in `running_sum_of_avg`
		bool cumLeqValid = true;
		
		// For peak level, the max across all data points since cumulative Leq began
		float32_t peak_level_sq = 0.0f;				// peak level^2

		//here are the protected methods
		virtual unsigned long setTimeWindow_samp(unsigned long samples) {
			timeWindow_samp = samples;
			clearStates();  //clears the averaging states!
			resetCumLeq(); 	//clears cumulative Leq
			// does not call resetPeakLvl()
			return timeWindow_samp;
		}
		int calcAverage(audio_block_f32_t *block);
		int calcAverage_exactBlock(audio_block_f32_t *block);
	
};

#endif
