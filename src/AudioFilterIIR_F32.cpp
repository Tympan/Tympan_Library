/*
 * AudioFilterIIR_F32.cpp
 *
 * Chip Audette, OpenAudio, May 2020
 *
 * MIT License,  Use at your own risk.
 *
*/

#include "AudioFilterIIR_F32.h"

#include "utility/BTNRH_iir_filter.h"

void AudioFilterIIR_F32::update(void)
{
	audio_block_f32_t *block, *block_new;

	block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

	// If there's no coefficient table, give up.  
	if (n_coeff == 0) {
		AudioStream_F32::release(block);
		//if (Serial) Serial.println("AudioFilterIIR_F32: update: no coeff.  Returning.");
		return;
	}

	// get a block for the IIR output
	block_new = AudioStream_F32::allocate_f32();
	if (block_new) {

		//apply the IIR
		filterz_nocheck(b_coeff, n_coeff,
				a_coeff, n_coeff,
				block->data, block_new->data, block->length,
				filter_states);   //this is in "utility/BTNRH_iir_filter.h"
				
		//copy info about the block
		block_new->length = block->length;
		block_new->id = block->id;

		//transmit the data
		AudioStream_F32::transmit(block_new); // send the FIR output
		AudioStream_F32::release(block_new);
	} else {
		if (Serial) Serial.println("AudioFilterIIR_F32: update: could not allocate block_new.");
	}
	AudioStream_F32::release(block);
}
