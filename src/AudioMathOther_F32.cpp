/*
 * AudioMathOther_F32
 * 
 * Created: Chip Audette, Open Audio, Sept 2025
 * Purpose: Contains a bunch of classes for math operations on audio blocks
 * Assumes floating-point data.
 *          
 * These classes generally process a single stream fo audio data (ie, they are mono)       
 *          
 * MIT License.  use at your own risk.
*/

#include "AudioMathOther_F32.h"


void AudioMathAbs_F32::update(void) {
  audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32(0);
	audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if ((block==nullptr) || (out_block==nullptr)) {
		AudioStream_F32::release(block);
		AudioStream_F32::release(out_block);
		return;
	}
  
	//process the data
	//for (int i=0; i<block->length; i++) out_block->data[i] = fabs(block-data[i]);
	arm_abs_f32(block->data, out_block->data, block->length);
	out_block->length = block->length;
	out_block->fs_Hz = block->fs_Hz;

  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
	AudioStream_F32::release(block);
}

void AudioMathSquare_F32::update(void) {
  audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32(0);
	audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if ((block==nullptr) || (out_block==nullptr)) {
		AudioStream_F32::release(block);
		AudioStream_F32::release(out_block);
		return;
	}
  
	//process the data
	for (int i=0; i<block->length; i++) out_block->data[i] = (block->data[i])*(block->data[i]);
	out_block->length = block->length;
	out_block->fs_Hz = block->fs_Hz;

  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
	AudioStream_F32::release(block);
}



void AudioMathSqrt_F32::update(void) {
  audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32(0);
	audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if ((block==nullptr) || (out_block==nullptr)) {
		AudioStream_F32::release(block);
		AudioStream_F32::release(out_block);
		return;
	}
  
	//process the data
	//for (int i=0; i<block->length; i++) block->data[i] = sqrtf(block-data[i]);
	for (int i=0; i<block->length; i++) arm_sqrt_f32( block->data[i], &(out_block->data[i]));
	out_block->length = block->length;
	out_block->fs_Hz = block->fs_Hz;

  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
	AudioStream_F32::release(block);
}
