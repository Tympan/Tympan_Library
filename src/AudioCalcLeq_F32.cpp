
/*
 * AudioCalcLeq_F32.cpp
 *
 * Chip Audette, OpenAudio, May 2020
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioCalcLeq_F32.h"

void AudioCalcLeq_F32::update(void)
{
	audio_block_f32_t *block;

	block = AudioStream_F32::receiveWritable_f32();
	if (!block) return;

	//square the incoming audio data
	for (int i=0; i < block->length; i++) {
		block->data[i] *= block->data[i];
	}

	// update peak level
	float32_t tmp_peak_sq = 0.0f;				// buffer to store peak level^2
	unsigned long tmp_peak_sample_idx = 0;		// index of peak level within audio block.  Not used.
	arm_max_f32( &(block->data[0]), block->length, &tmp_peak_sq, &tmp_peak_sample_idx );

	// If new peak found, record it
	if ( tmp_peak_sq > (peak_level_sq) ) { 
		peak_level_sq = tmp_peak_sq;
	}

	//apply the time window averaging
	if (timeWindow_samp == ((unsigned long) block->length)) { //the averaging is the same as the block length!
		calcAverage_exactBlock(block);
	} else {
		calcAverage(block);
	}
	
	//update max
	for (int i=0; i < block->length; i++) { if (block->data[i] > max_value) max_value = block->data[i]; }
		
	//transmit the data
	AudioStream_F32::transmit(block); // send the IIR output
	AudioStream_F32::release(block);
}


int AudioCalcLeq_F32::calcAverage_exactBlock(audio_block_f32_t *block) {
	//average the whole block
	arm_mean_f32(block->data, block->length, &cur_value);  //from the CMSIS DSP library, comes as part of the Teensyduino installation

	//fill the output block with the new value
	//for (int i=0; i < block->data; i++) block->data[i] = cur_value;
	arm_fill_f32(cur_value, block->data, block->length); //from the CMSIS DSP library,
	
	int number_of_updated_averages = 1;

	// update the running avereage of averages
	running_sum_of_avg += cur_value;

	// Increment count of how many averages are in running_sum_of_avg
	if ( num_averages < std::numeric_limits<unsigned long>::max() ) {
		num_averages ++;
	
	// Else about to wrap. Reset the cumulative level as if this was the first block
	} else {
		Serial.println("AudioCalcLeq_F32::update: ***num_averagest*** has wrapped.");
		running_sum_of_avg = 0.0f;
		num_averages = 1;
		cumLeqValid = false;  // mark that this measurement was reset.  Use `resetCumLeq()` to reset.
	}	

	return number_of_updated_averages;
}


int AudioCalcLeq_F32::calcAverage(audio_block_f32_t *block) {
	int number_of_updated_averages = 0;
	
	//step through each data point, accumulating and updating as needed
	for (int i=0; i < block->length; i++) {
		running_sum += block->data[i]; //update the running sum
		points_averaged++;  //update our tracking of the number of data points averaged 
		
		if (points_averaged >= timeWindow_samp) { //do we have enough points to update our average?
			//yes, update the running average
			cur_value = running_sum / ((float32_t) points_averaged);
			number_of_updated_averages++;

			// update the running avereage of averages
			running_sum_of_avg += cur_value;

			// Increment count of how many averages are in running_sum_of_avg
			if ( num_averages < std::numeric_limits<unsigned long>::max() ) {
				num_averages ++;
			
			// Else about to wrap. Reset the cumulative level as if this was the first block
			} else {
				Serial.println("AudioCalcLeq_F32::update: ***num_averagest*** has wrapped.");
				running_sum_of_avg = 0.0f;
				num_averages = 1;
				cumLeqValid = false;  // mark that this measurement was reset.  Use `resetCumLeq()` to reset.
			}	
			clearStates();
		}
		
		//put the current value into the output
		block->data[i] = cur_value;
	}
	
	return number_of_updated_averages;
}	  
	  
	 /*
//maybe this is a bit faster?  more complicated to debug, however
void AudioCalcLeq_F32::calcAverage_longAverage(audio_block_f32_t *block) {

	// continue the current averaging that was started in a previous audio block
	unsigned long n_remain = timeWindow_samp - points_averaged;
	int n_to_process = min(n_remain, block->length);
	for (int i; i < n_to_process; i++) {
		//update the running average
		running_sum += block->data[i];

		//back-fill the output with the previous value
		block->data[i] = cur_value;
	}
	points_averaged += n_to_process; //update the count of points that are averaged
	int next_start_ind = n_to_process;

	//do we update the current averaging?
	if (points_averaged == timeWindow_samp) {
		//a new value is ready!
		cur_value = running_sum / ((float32_t) points_averaged);
		block->data[n_to_process-1] = cur_value;
		clearStates();
	}

	//do we start new averaging to finish out this data block?
	if (n_to_process < block->length) {
		n_to_process = block->length - next_start_ind;
		for (int i; i < n_to_process; i++) {
			//update the running average
			running_sum += block->data[i];

			//back-fill the output with the previous value
			block->data[i] = cur_value;
		}
		points_averaged += n_to_process; //update the count of points that are averaged
	}
}
*/ 


/**
 * @brief Get the average RMS level (Leq) since the audio effect started, or was cleared.
 * \note To clear the cumLeqValid flag, call resetCumLeq()
 * @param levelRms variable to store RMS level in
 * @return true Valid result
 * @return false Invalid result. levelRms will be set to 0.0f
 */
bool AudioCalcLeq_F32::getCumLevelRms (float32_t &levelRms) {
	bool success = true;
	levelRms = 0.0f;

	// Check that data has been collected
	if ( (num_averages > 0) && (running_sum_of_avg > 0) ) {
		// Take sqrt of mean of running_sum_of_avg, where running_sum_of_avg represents level^2
		arm_sqrt_f32(running_sum_of_avg / ( (float32_t) num_averages), &levelRms);
	} else {
		success = false;
		Serial.println("AudioCalcLeq_F32::getCumLevelRms: ***Error calculating Leq***");
	}
	//Serial.println( String(" running sum of avg: ") + String(running_sum_of_avg) );
	//Serial.println( String("; num_averages: ") + String(num_averages) );

	return success;
}


/**
 * @brief Get the average RMS level (Leq) in dB FS, since the audio effect started, or was cleared.
 * \note To clear the cumLeqValid flag, call resetCumLeq()
 * @param levelRms_dB variable to store RMS level in
 * @return true Valid result
 * @return false Invalid result. levelRms_dB will be set to 0.0f
 */
bool AudioCalcLeq_F32::getCumLevelRms_dB (float32_t &levelRms_dB) {
	bool success = true;
	levelRms_dB = 0.0f;

	// Check that data has been collected
	if ( (num_averages > 0) && (running_sum_of_avg > 0) ) {
		// Take mean and convert to decibels, relying on running_sum_of_avg representing level^2
		levelRms_dB = 10*log10f( running_sum_of_avg / ( (float) num_averages) );
	} else {
		success = false;
		Serial.println("AudioCalcLeq_F32::getCumLevelRms_dB: ***Error calculating Leq***");
	}
	//Serial.println( String(" running sum of avg: ") + String(running_sum_of_avg) );
	//Serial.println( String("; num_averages: ") + String(num_averages) );

	return (success);
}


/**
 * @brief Get peak level since audio effect began, or peak level was cleared
 * \note To reset peak level, call `resetPeakLvl()`. 
 * @return float32_t peak level. Returns 0.0f if peak_level_sq == 0.0f.
 */
float32_t AudioCalcLeq_F32::getPeakLevel(void) const {
	float32_t peak_level = 0.0f;
	
	if (peak_level_sq > 0.0f) {
		peak_level = sqrtf(peak_level_sq);
	}  // else leave peak_level = 0.0f
	return (peak_level);
};



/**
 * @brief Get peak level in units of dB FS, since audio effect began, or peak level was cleared. 
 * \note To reset peak level, call `resetPeakLvl()`. 
 * @return float32_t peak level in dB FS.  Returns 0.0f if peak_level_sq == 0.0f. 
 */
float32_t AudioCalcLeq_F32::getPeakLevel_dB(void) const {
	float32_t peak_level = 0.0f;
	
	if (peak_level_sq > 0.0f) {
		peak_level = 10*log10f(peak_level_sq);
	}  // else leave peak_level = 0.0f
	return (peak_level);
};