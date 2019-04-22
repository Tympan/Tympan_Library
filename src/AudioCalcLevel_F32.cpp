
/*
 * AudioCalcLevel_F32.cpp
 *
 * Chip Audette, OpenAudio, June 2017
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioCalcLevel_F32.h"

void AudioCalcLevel_F32::update(void)
{
  audio_block_f32_t *block;

  block = AudioStream_F32::receiveWritable_f32();
  if (!block) return;
  
  //square the incoming audio data
  for (int i=0; i < block->length; i++) block->data[i] *= block->data[i];

  //apply filter (from AudioFilterTimeWeighting)
  applyFilterInPlace(block->data, block->length);
  
  //save last value
  cur_value = block->data[(block->length)-1];
  
	
  //transmit the data
  AudioStream_F32::transmit(block); // send the IIR output
  AudioStream_F32::release(block);
}
