#include "AudioSwitch_F32.h"

void AudioSwitch4_F32::update(void) {
  audio_block_f32_t *in, *out=NULL;
  
  out = receiveWritable_f32(0);
  if (!out) return;
  
  AudioStream_F32::transmit(out,outputChannel); //just output to the one channel
  AudioStream_F32::release(out);
}
