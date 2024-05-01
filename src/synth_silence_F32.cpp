/* 
 * AdioSynthSilence_F32
 * 
 * Created: Chip Audette (OpenAudio) Sept 2019
 * Modeled on: AudioSynthWaveformSine from Teensy Audio Library
 * 
 * Purpose: Create a block of silence
 *
 * License: MIT License. Use at your own risk.
 *
 */

#include "synth_silence_F32.h"


void AudioSynthSilence_F32::update(void)
{
	audio_block_f32_t *block;
	
	if (enabled) {
		
		block = allocate_f32();
		if (block) {
			for (int i=0; i < block->length; i++) {
				block->data[i]=0.0;
			}
			
			block_counter++;
			block->id = block_counter;
			
			AudioStream_F32::transmit(block);
			AudioStream_F32::release(block);
			return;
		}
		
	}
}






