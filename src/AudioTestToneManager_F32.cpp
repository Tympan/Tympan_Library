
#include "AudioTestToneManager_F32.h"


void AudioTestToneManager_F32::startNextTone(void) {
	if (allToneParams.size()==0) { stopTest(); return; }
	if (curToneIndex >= (int)(allToneParams.size() - 1)) { stopTest(); return; } //cannot start next tone.  we're done.     
	curToneIndex++;
	curToneParams = allToneParams[curToneIndex];
	tone_dphase_rad = 2.0*M_PI*curToneParams.freq_Hz / sample_rate_Hz;
	if (current_state != AudioTestToneManager_F32::STATE_TONE) {
		//we were silent, so now reset the tone's phase
		tone_phase_rad = 0.0;
	}
	remaining_samples_needed = (int)(0.5+curToneParams.dur_sec*sample_rate_Hz);
	current_state = AudioTestToneManager_F32::STATE_TONE;
	if (flag_printChangesToSerial) {
		Serial.print("AudioTestToneManager_F32: starting tone ");
		Serial.print(curToneIndex+1); Serial.print(" of "); Serial.print(allToneParams.size());
		Serial.print(" at ");  Serial.print(curToneParams.freq_Hz); Serial.print(" Hz");
		Serial.print(" at ");  Serial.print(curToneParams.amp,3); Serial.print(" amplitude");
		Serial.print(" for "); Serial.print(curToneParams.dur_sec); Serial.print(" sec");
		Serial.println();
	}
	return;
}

void AudioTestToneManager_F32::startSilence(void) {
	current_state = AudioTestToneManager_F32::STATE_SILENCE;
	remaining_samples_needed = (int)(0.5+silenceBetweenTones_sec*sample_rate_Hz);
	if (flag_printChangesToSerial) {
		Serial.print("AudioTestToneManager_F32: starting silence ");
		Serial.print(curToneIndex+1); Serial.print(" of "); Serial.print(allToneParams.size());
		Serial.print(" for ");  Serial.print(silenceBetweenTones_sec);  Serial.print(" sec");
		Serial.println();
	}
}

int AudioTestToneManager_F32::incrementState(void) {
	switch (current_state) {
		case STATE_TONE:
			{
				//we should switch to silence, but first see if there is any silence to be played
				int total_samp_for_silence = (int)(0.5+silenceBetweenTones_sec*sample_rate_Hz); //round
				if (total_samp_for_silence == 0) {
					startNextTone(); //no silence!  jump to next tone
				} else {
					startSilence();
				}
			}
			break;
		case STATE_SILENCE:
			startNextTone(); //starts next tone (or auto-stops if there is no next tone)
			break;
		default:
			//nothing to do;
			break;
	}
	return current_state;
}

void AudioTestToneManager_F32::update(void) {
	if (isTestActive() == false) return;
	
	audio_block_f32_t *block_new = allocate_f32();
	if (!block_new) return; //could not allocate block.  So, release memory and return.

	// set properties of the out-going audio block
	int block_len = min(block_new->length, audio_block_samples);
	block_new->length = block_len;
	block_new->id = block_counter;
	block_new->fs_Hz = sample_rate_Hz;

	//fill one audio_block_f32 with audio samples
	float temp_val = 0.0f;
	for (int Isamp = 0; Isamp < block_new->length; Isamp++) {
		//check the state and increment as needed
		if (remaining_samples_needed == 0) incrementState();

		//generate an audio sample
		temp_val = 0.0; //assume silence for now
		if (remaining_samples_needed > 0) {
			if (current_state == AudioTestToneManager_F32::STATE_TONE) {
				//sine
				temp_val = curToneParams.amp * arm_sin_f32(tone_phase_rad);
				tone_phase_rad += tone_dphase_rad;
				if (tone_phase_rad > (2*M_PI)) tone_phase_rad -= (2*M_PI);  //wrap te phase
				if (tone_phase_rad < (-2*M_PI)) tone_phase_rad += (2*M_PI); //wrap the phase
			}
		}

		//save value and prepare for next loop
		block_new->data[Isamp] = temp_val;
		if (remaining_samples_needed > 0) remaining_samples_needed--;
	}
	
	//increment counters
	block_counter++;

	//return data
	AudioStream_F32::transmit(block_new);
	AudioStream_F32::release(block_new);
}

int AudioTestToneManager_F32::createFreqTest(float32_t start_freq_Hz, float32_t end_freq_Hz, float32_t step_amount, float32_t amp, float32_t dur_sec, int step_type) {
	removeAllTonesFromQueue();
	float32_t min_freq_Hz = min(start_freq_Hz, end_freq_Hz);
	float32_t max_freq_Hz = max(start_freq_Hz, end_freq_Hz);
	float32_t this_step_freq_Hz = start_freq_Hz;
	const int max_allowed_steps = 128;
	for (int Icount = 0; Icount < max_allowed_steps; Icount++) {
		//only add the tone if it's frequency is valid
		if ( (this_step_freq_Hz <= max_freq_Hz) && (this_step_freq_Hz >= min_freq_Hz) ) {
			addToneToQueue(this_step_freq_Hz, amp, dur_sec);
		} 

		//increment the frequency
		if (step_type == STEP_LINEAR) {
			this_step_freq_Hz += step_amount; //linear steps
		} else {
			this_step_freq_Hz *= step_amount; //exponential
		}
	}
	return (int)allToneParams.size();
}

int AudioTestToneManager_F32::createAmplitudeTest(float32_t start_amp, float32_t end_amp, float32_t step_amount, float32_t freq_Hz, float32_t dur_sec, int step_type) {
	removeAllTonesFromQueue();
	float32_t min_amp = min(start_amp, end_amp);
	float32_t max_amp = max(start_amp, end_amp);
	float32_t this_amp = start_amp;
	const int max_allowed_steps = 128;
	for (int Icount = 0; Icount < max_allowed_steps; Icount++) {
		//only add the tone if it's frequency is valid
		if ( (this_amp <= max_amp) && (this_amp >= min_amp) ) {
			//if dB, convert to linear
			float amp_linear = this_amp;
			if (step_type != STEP_LINEAR) amp_linear = sqrtf(powf(10.0f, this_amp/10.0));  //convert from dB to linear

			//add this tone to the queue of test tones
			addToneToQueue(freq_Hz, amp_linear, dur_sec);
		} 

		//increment the frequency
		this_amp += step_amount;
	}
	return (int)allToneParams.size();
}