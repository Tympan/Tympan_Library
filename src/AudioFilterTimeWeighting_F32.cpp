
/*
 * AudioFilterTimeWeighting_F32.cpp
 *
 * Chip Audette, OpenAudio, June 2017
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioFilterTimeWeighting_F32.h"

void AudioFilterTimeWeighting_F32::update(void)
{
  audio_block_f32_t *block;

  block = AudioStream_F32::receiveWritable_f32();
  if (!block) return;

  //apply filter
  float32_t foo = (1.0f - alpha);
  for (int i=0; i < block->length; i++) {
	  block->data[i] = foo * block->data[i] + alpha * prev_val;
	  prev_val = block->data[i]; //save for next time
  }
  
  //transmit the data
  AudioStream_F32::transmit(block); // send the IIR output
  AudioStream_F32::release(block);
}
