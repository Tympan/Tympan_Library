
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

  if (is_bypassed) {
    for (int i=0; i < block->length; i++) block->data[i] = 0.0f;  //zero out the data
    AudioStream_F32::transmit(block); // send the IIR output
    AudioStream_F32::release(block);
    return;
  }

  //apply filter
  applyFilterInPlace(block->data,block->length);
  
  //transmit the data
  AudioStream_F32::transmit(block); // send the IIR output
  AudioStream_F32::release(block);
}

void AudioFilterTimeWeighting_F32::applyFilterInPlace(float32_t *data, int length) {
  float32_t foo = (1.0f - alpha);
  for (int i=0; i < length; i++) {
	  data[i] = foo * data[i] + alpha * prev_val;
	  prev_val = data[i]; //save for next time
  }
}