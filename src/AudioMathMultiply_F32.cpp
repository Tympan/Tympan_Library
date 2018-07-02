#include "AudioMathMultiply_F32.h"

void AudioMathMultiply_F32::update(void) {
  audio_block_f32_t *block, *in;

  block = AudioStream_F32::receiveWritable_f32(0);
  if (!block) return;

  in = AudioStream_F32::receiveReadOnly_f32(1);
  if (!in) {
    release(block);
    return;
  }

  arm_mult_f32(block->data, in->data, block->data, block->length);
  release(in);

  transmit(block);
  release(block);
}
