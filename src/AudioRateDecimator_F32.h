/*
 * AudioRateDecimator_F32
 * 
 * Created: Chip Audette (OpenAudio) Sept 2021
 * 
 * Purpose: Uses ARM CMSIS DSP functions to filter and then decimate
 *
 * MIT License.  Use at your own risk.  Have fun!
 * 
 */

#ifndef _AudioRateDecimator_F32_h
#define _AudioRateDecimator_F32_h

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "arm_math.h"

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define DEC_FIR_F32_PASSTHRU ((const float32_t *) 1)   //if you sete coeff_p to this, update() will simply 
#define DEC_FIR_MAX_COEFFS 200

class AudioRateDecimator_F32 : public AudioStream_F32 {
	public:
		AudioRateDecimator_F32(void) : AudioStream_F32(1,inputQueueArray) {}
		AudioRateDecimator_F32(const AudioSettings_F32 &settings): AudioStream_F32(1,inputQueueArray), 
			start_sample_rate_Hz(settings.sample_rate_Hz), 
			end_sample_rate_Hz(settings.sample_rate_Hz) {}
	
		//initialize the decimator filter by giving it the filter coefficients
		bool begin(void) { return begin(coeff_passthru, 1, 1, AUDIO_BLOCK_SAMPLES); }
		bool begin(const float32_t *cp, const int _n_coeffs, const int _dec_fac) { return begin(cp, _n_coeffs, _dec_fac, AUDIO_BLOCK_SAMPLES); } //assume that the block size is the maximum
		bool begin(const float32_t *cp, const int _n_coeffs, const int _dec_fac, const int block_size);   //or, you can provide it with the block size
		void end(void) {  coeff_p = NULL; enable(false); }
		
		void update(void);
		int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update(); returns zero if OK

		bool enable(bool enable = true) { 
			if (enable == true) {
				if ((coeff_p != DEC_FIR_F32_PASSTHRU) && (is_armed)) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				}
			}
			is_enabled = false;
			return get_is_enabled();
		}
		bool get_is_enabled(void) { return is_enabled; }

		//void setBlockDC(void) {}	//helper function that sets this up for a first-order HP filter at 20Hz
		
		float set_startSampleRate_Hz(float fs_Hz) { start_sample_rate_Hz = fs_Hz;  end_sample_rate_Hz = start_sample_rate_Hz / dec_fac; return start_sample_rate_Hz; }
		float get_startSampleRate_Hz(void) { return start_sample_rate_Hz; }
		float get_endSampleRate_Hz(void) { return end_sample_rate_Hz; }
		
		
		void printCoeff(int start_ind, int end_ind);
	
	protected:
		audio_block_f32_t *inputQueueArray[1];
		float start_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		float end_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		
		bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
		bool is_enabled = false; //do you want this filter to execute?
	
		// pointer to current coefficients or NULL or FIR_PASSTHRU
		const float32_t coeff_passthru[1] = {1.0f}; //if you do begin() with this, the FIR filter will actually execute and update() will transmit the same values that you put in
		const float32_t *coeff_p;
		int n_coeffs = 1;
		int dec_fac = 1;
		int configured_block_size;

		// ARM DSP Math library filter instance
		arm_fir_decimate_instance_f32 decimate_inst;
		const int fir_max_coeffs = DEC_FIR_MAX_COEFFS;
		float32_t StateF32[AUDIO_BLOCK_SAMPLES + DEC_FIR_MAX_COEFFS];
	
};


bool AudioRateDecimator_F32::begin(const float32_t *cp, const int _n_coeffs, const int _dec_fac, const int block_size) {  //or, you can provide it with the block size
	coeff_p = cp;
	n_coeffs = _n_coeffs;
	dec_fac = _dec_fac;
	
	// Initialize FIR instance (ARM DSP Math Library)
	if (coeff_p && (coeff_p != DEC_FIR_F32_PASSTHRU) && n_coeffs <= fir_max_coeffs) {
		//initialize the ARM FIR module
		arm_fir_decimate_init_f32(&decimate_inst, n_coeffs, dec_fac, (float32_t *)coeff_p,  &StateF32[0], block_size);
		configured_block_size = block_size;
		end_sample_rate_Hz = start_sample_rate_Hz / ((float)dec_fac);
		
		is_armed = true;
		is_enabled = true;
	} else {
		is_enabled = false;
	}
	
	//Serial.println("AudioRateDecimator_F32: begin complete " + String(is_armed) + " " + String(is_enabled) + " " + String(get_is_enabled()));
	
	return get_is_enabled();
}


void AudioRateDecimator_F32::update(void)
{
	audio_block_f32_t *block, *block_new;

	if (!is_enabled) return;

	//Serial.println("AudioRateDecimator_F32: update: starting...");

	block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;  //no data to get

	// If there's no coefficient table, give up.  
	if (coeff_p == NULL) {
		AudioStream_F32::release(block);
		return;
	}

	// do passthru
	if (coeff_p == DEC_FIR_F32_PASSTHRU) {
		// Just pass through
		AudioStream_F32::transmit(block);
		AudioStream_F32::release(block);
		//Serial.println("AudioRateDecimator_F32: update(): PASSTHRU.");
		return;
	}

	// get a block for the decimator output
	block_new = AudioStream_F32::allocate_f32();
	if (block_new == NULL) { AudioStream_F32::release(block); return; } //failed to allocate
	
	//apply the filter
	processAudioBlock(block,block_new);

	//transmit the data and release the memory blocks
	AudioStream_F32::transmit(block_new); // send the output
	AudioStream_F32::release(block_new);  // release the memory
	AudioStream_F32::release(block);	  // release the memory
	
}


int AudioRateDecimator_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new) {
	if ((is_enabled == false) || (block==NULL) || (block_new==NULL)) return -1;
	
	//check to make sure our decimator instance has the right size
	if (block->length != configured_block_size) {
		//doesn't match.  re-initialize
		Serial.println("AudioRateDecimator_F32: block size doesn't match.  Re-initializing Decimator.");
		begin(coeff_p, n_coeffs, block->length);  //initialize with same coefficients, just a new block length
	}
	
	//apply the decimator
	arm_fir_decimate_f32(&decimate_inst, block->data, block_new->data, block->length);
	
	//copy info about the block
	block_new->length = block->length / dec_fac;
	block_new->id = block->id;	
	
	return 0;
}


void AudioRateDecimator_F32::printCoeff(int start_ind, int end_ind) {
	start_ind = min(n_coeffs-1,max(0,start_ind));
	end_ind = min(n_coeffs-1,max(0,end_ind));
	Serial.print("AudioRateDecimator_F32: printCoeff [" + String(start_ind) + ", " + String(end_ind) + "): ");
	for (int i=start_ind; i<end_ind; i++) {
		Serial.print(coeff_p[i],4); 
		Serial.print(", ");
	}
	Serial.println();				
}



#endif