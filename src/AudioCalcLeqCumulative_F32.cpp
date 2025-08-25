
/*
 * AudioCalcLeqCumulative_F32.cpp
 *
 * Eric Yuan, Chip Audette, Aug 2025
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioCalcLeqCumulative_F32.h"

void AudioCalcLeqCumulative_F32::update(void)
{
	audio_block_f32_t *block;

	block = AudioStream_F32::receiveWritable_f32();
	if (!block) return;

	//square the incoming audio data
	for (int i=0; i < block->length; i++) {	block->data[i] *= block->data[i] ;}
	
	//update the tracking of the peak value
	updateCumulativePeak(block);  //block holds the squared audio samples!

	//apply the time window averaging
	updateCumulativeAverage(block);
	
	//fill the outgoing audio block with the Leq RMS value
	float32_t RMS_val = 0.0;
	getCumLevelRms(RMS_val);
	for (int i=0; i < block->length; i++) {	block->data[i] = RMS_val; }

	//transmit the data
	AudioStream_F32::transmit(block); // send the IIR output
	AudioStream_F32::release(block);
}

void AudioCalcLeqCumulative_F32::updateCumulativePeak(audio_block_f32_t *block_sq) { //block_sq holds the squared audio data
	// update peak level
	float32_t tmp_peak_sq = 0.0f;				// buffer to store peak level^2
	unsigned long tmp_peak_sample_idx = 0;		// index of peak level within audio block.  Not used.
	arm_max_f32( &(block_sq->data[0]), block_sq->length, &tmp_peak_sq, &tmp_peak_sample_idx );

	// If new peak found, record it
	if ( tmp_peak_sq > (peak_level_sq) ) { peak_level_sq = tmp_peak_sq; }
}

void AudioCalcLeqCumulative_F32::updateCumulativeAverage(audio_block_f32_t *block_sq) { //block_sq holds the squared audio data
	
	//get the average value of this data block
	float32_t ave_val_sq = 0.0;
	arm_mean_f32(block_sq->data, block_sq->length, &ave_val_sq);
	
	//add the block average to the overall running average
	running_sum_of_avg += ave_val_sq;
	
	//can we validly increment our averaging counter?
	if ( num_averages < std::numeric_limits<unsigned long long>::max() ) {
		// Increment count of how many averages are in running_sum_of_avg
		num_averages ++;
	
	} else { 
		// Else about to wrap. Reset the cumulative level as if this was the first block
		Serial.println("AudioCalcLeq_F32::update: ***num_averagest*** has wrapped.");
		running_sum_of_avg = 0.0f;
		num_averages = 1;
		cumLeqValid = false;  // mark that this measurement was reset.  Use `resetCumLeq()` to reset.
	}	
	
}	  
	

/**
 * @brief Get the average RMS level (Leq) since the audio effect started, or was cleared.
 * \note To clear the cumLeqValid flag, call resetCumLeq()
 * @param levelRms variable to store RMS level in
 * @return true Valid result
 * @return false Invalid result. levelRms will be set to 0.0f
 */
bool AudioCalcLeqCumulative_F32::getCumLevelRms (float32_t &levelRms) {
	bool success = true;
	levelRms = 0.0f;

	// Check that data has been collected
	if ( (num_averages > 0) && (running_sum_of_avg > 0) ) {
		// Take sqrt of mean of running_sum_of_avg, where running_sum_of_avg represents level^2
		arm_sqrt_f32(running_sum_of_avg / ( (float32_t) num_averages), &levelRms);
	} else {
		success = false;
		Serial.println("AudioCalcLeqCumulative_F32::getCumLevelRms: ***Error calculating Leq***");
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
bool AudioCalcLeqCumulative_F32::getCumLevelRms_dB (float32_t &levelRms_dB) {
	bool success = true;
	levelRms_dB = 0.0f;

	// Check that data has been collected
	if ( (num_averages > 0) && (running_sum_of_avg > 0) ) {
		// Take mean and convert to decibels, relying on running_sum_of_avg representing level^2
		levelRms_dB = 10.0f*log10f( running_sum_of_avg / ( (float) num_averages) );
	} else {
		success = false;
		Serial.println("AudioCalcLeqCumulative_F32::getCumLevelRms_dB: ***Error calculating Leq***");
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
float32_t AudioCalcLeqCumulative_F32::getPeakLevel(void) const {
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
float32_t AudioCalcLeqCumulative_F32::getPeakLevel_dB(void) const {
	float32_t peak_level = 0.0f;
	
	if (peak_level_sq > 0.0f) {
		peak_level = 10.0f*log10f(peak_level_sq);
	}  // else leave peak_level = 0.0f
	return (peak_level);
};