
#ifndef _AudioCalcLeqCumulative_F32_h
#define _AudioCalcLeqCumulative_F32_h

#include "Arduino.h"
#include "AudioStream_F32.h"
#include <arm_math.h>


/*
 AudioCalcLeqCumulative_F32.h

 Eric Yuan, Aug 2025
 Based on AudioCalcLeq_F32, Chip Audette, May 2020
 
 Purpose: 
    Calculates Leq, averaged cumulatively from a given start time.  In parallel,
		it also keeps track of the peak level since the start of time.  You can
		reset the start time (ie, reset the average to zero) at any time.
 
  Usage:
	  Cumulative Leq: 
		  * To get the equivalent sound level since the effect was active, use 
			  `getCumLevelRms()` or `getCumLevelRms_dB`. 
			* Reset the averaging using `resetCumLeq()`.  
			* This function relies on counting samples, which will wrap. To check
        this, call `isCumLeqValid()`

		Cumulative Peak:
      * To get the peak level since the effect was active, use `getPeakLevel()`
			* To reset the peak level, call `resetPeakLvl()`
  
  MIT License,  Use at your own risk.
*/

class AudioCalcLeqCumulative_F32 : public AudioStream_F32 
{
	public:
		AudioCalcLeqCumulative_F32(void) : AudioStream_F32(1,inputQueueArray) {
			//setTimeWindow_sec(CALC_LEQ_DEFAULT_SEC);
			//setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		}		
		AudioCalcLeqCumulative_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) {
			//setTimeWindow_sec(CALC_LEQ_DEFAULT_SEC);
			//setSampleRate_Hz(settings.sample_rate_Hz);
		}
		virtual ~AudioCalcLeqCumulative_F32() {};
		
		//update the calculations
		void update(void) override;
		virtual void updateCumulativePeak(audio_block_f32_t *block_sq);    //input is audio data that's already been squared
		virtual void updateCumulativeAverage(audio_block_f32_t *block_sq); //input is audio data that's already been squared
				
		//get the results
		virtual bool getCumLevelRms (float32_t &cumLeq);
		virtual bool getCumLevelRms_dB (float32_t &cumLeq);
		virtual float32_t getPeakLevel(void) const;
		virtual float32_t getPeakLevel_dB(void) const;

		//reset the running average and running peak calcluations back to zero
		virtual void resetCumLeq(void) {
			running_sum_of_avg = 0.0f;
			num_averages = 0;
			cumLeqValid = true;	
		}
		virtual void resetPeakLvl(void) { peak_level_sq = 0.0f; }

		//if the averaging wraps around the 64-bit counter, the averaging is invalid.
		//provide a method to let the user know if the value is invalid.
		bool isCumLeqValid(void) const { return (cumLeqValid); }

	protected:
		audio_block_f32_t *inputQueueArray[1];
		
		// Averaging states
		float32_t running_sum_of_avg = 0.0f;  	// This stores a running sum of power levels (mean_val^2)
		unsigned long long num_averages = 0;		// # of values in `running_sum_of_avg (64-bit value)
		bool cumLeqValid = true;
		float32_t peak_level_sq = 0.0f;				  // peak level^2
		
};

#endif
