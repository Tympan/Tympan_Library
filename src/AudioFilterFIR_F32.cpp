/*
 * AudioFilterFIR_F32.cpp
 *
 * Chip Audette, OpenAudio, Apr 2017
 *
 * MIT License,  Use at your own risk.
 *
*/

#include "AudioFilterFIR_F32.h"


void AudioFilterFIR_F32::update(void)
{
	audio_block_f32_t *block, *block_new;

	if (!is_enabled) return;

	block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

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
	if (block_new) {
		//apply the filter
		processAudioBlock(block,block_new);

		//transmit the data
		AudioStream_F32::transmit(block_new); // send the FIR output
		AudioStream_F32::release(block_new);
	}
	AudioStream_F32::release(block);
}

int AudioFilterFIR_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new) {
	if (!is_enabled || !block || !block_new) return -1;
	
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