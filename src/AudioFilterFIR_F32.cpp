/*
 * AudioFilterFIR_F32.cpp
 *
 * Chip Audette, OpenAudio, Apr 2017
 *
 * MIT License,  Use at your own risk.
 *
*/

#include "AudioFilterFIR_F32.h"

bool AudioFilterFIR_F32::begin(const float32_t *cp, const int _n_coeffs, const int block_size) {  //or, you can provide it with the block size
	coeff_p = cp;
	n_coeffs = _n_coeffs;
	
	// Initialize FIR instance (ARM DSP Math Library)
	if (coeff_p && (coeff_p != FIR_F32_PASSTHRU) && n_coeffs <= FIR_MAX_COEFFS) {
		//initialize the ARM FIR module
		arm_fir_init_f32(&fir_inst, n_coeffs, (float32_t *)coeff_p,  &StateF32[0], block_size);
		configured_block_size = block_size;
		
		is_armed = true;
		is_enabled = true;
	} else {
		is_enabled = false;
	}
	
	//Serial.println("AudioFilterFIR_F32: begin complete " + String(is_armed) + " " + String(is_enabled) + " " + String(get_is_enabled()));
	
	return get_is_enabled();
}


void AudioFilterFIR_F32::update(void)
{
	audio_block_f32_t *block, *block_new;

	if (!is_enabled) return;

	//Serial.println("AudioFilterFIR_F32: update: starting...");

	block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;  //no data to get

	// If there's no coefficient table, give up.  
	if (coeff_p == NULL) {
		AudioStream_F32::release(block);
		return;
	}

	// do passthru
	if (coeff_p == FIR_F32_PASSTHRU) {
		// Just pass through
		AudioStream_F32::transmit(block);
		AudioStream_F32::release(block);
		//Serial.println("AudioFilterFIR_F32: update(): PASSTHRU.");
		return;
	}

	// get a block for the FIR output
	block_new = AudioStream_F32::allocate_f32();
	if (block_new == NULL) { AudioStream_F32::release(block); return; } //failed to allocate
	
	//apply the filter
	processAudioBlock(block,block_new);

	//transmit the data and release the memory blocks
	AudioStream_F32::transmit(block_new); // send the FIR output
	AudioStream_F32::release(block_new);  // release the memory
	AudioStream_F32::release(block);	  // release the memory
	
}

int AudioFilterFIR_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new) {
	if ((is_enabled == NULL) || (block==NULL) || (block_new==NULL)) return -1;
	
	//check to make sure our FIR instance has the right size
	if (block->length != configured_block_size) {
		//doesn't match.  re-initialize
		Serial.println("AudioFilterFIR_F32: block size doesn't match.  Re-initializing FIR.");
		begin(coeff_p, n_coeffs, block->length);  //initialize with same coefficients, just a new block length
	}
	
	//apply the FIR
	arm_fir_f32(&fir_inst, block->data, block_new->data, block->length);
	
	//copy info about the block
	block_new->length = block->length;
	block_new->id = block->id;	
	
	return 0;
}

void AudioFilterFIR_F32::printCoeff(int start_ind, int end_ind) {
	start_ind = min(n_coeffs-1,max(0,start_ind));
	end_ind = min(n_coeffs-1,max(0,end_ind));
	Serial.print("AudioFilterFIR_F32: printCoeff [" + String(start_ind) + ", " + String(end_ind) + "): ");
	for (int i=start_ind; i<end_ind; i++) {
		Serial.print(coeff_p[i],4); 
		Serial.print(", ");
	}
	Serial.println();				
}