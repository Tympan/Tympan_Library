
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
	for (int i=0; i < block->length; i++) block->data[i] *= block->data[i];

	//apply the time window averaging
	if (timeWindow_samp == ((unsigned long) block->length)) { //the averaging is the same as the block length!
		calcAverage_exactBlock(block);
	} else {
		calcAverage(block);
	}
		
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
	return number_of_updated_averages;
}


int AudioCalcLeq_F32::calcAverage(audio_block_f32_t *block) {
	int number_of_updated_averages = 0;
	
	//step through each data point, accumulating and updating as needed
	for (int i=0; i < block->length; i++) {
		running_sum += block->data[i]; //update the running sum
		points_averaged++;  //update our tracking of the number of data points averaged 
		
		if (points_averaged >= timeWindow_samp) { //do we have enough points to update our average?
			//yes, update the average
			cur_value = running_sum / ((float32_t) points_averaged);
			number_of_updated_averages++;
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
