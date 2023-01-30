
#include "AudioEffectFade_F32.h"

//here's the method that does all the work
void AudioEffectFade_F32::update(void) {

  //get input block
  audio_block_f32_t *block;
  block = AudioStream_F32::receiveReadOnly_f32();
  if (block == NULL) return;
  
  //allocate memory for the output of our algorithm
  audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if (out_block == NULL) { AudioStream_F32::release(block); return; }

  //apply the gain
  processAudioBlock(block, out_block);

  //transmit the block and be done
  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
  AudioStream_F32::release(block);
}

int AudioEffectFade_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block) {
  if ((block == NULL) || (out_block == NULL)) return -1;  //-1 is error

  //prepare
  float low_lim = 0.5*step_size_amp;
  float up_lim = 1.0 - 0.5*step_size_amp;

  //loop over each sample
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {

	//apply the fade
	out_block->data[i] = cur_amp * (block->data[i]);

	//are we actively fading?
	if (is_active) {  
	
	  //increment the fade (with limiting
	  cur_amp = max(0.0f, min(1.0, cur_amp + step_size_amp));

	  //test to see if we're done with the fade
	  if (cur_amp < low_lim) {
		cur_amp = 0.0;
		is_active = false;
	  } else if (cur_amp > up_lim) {
		cur_amp = 1.0;
		is_active = false;
	  }
		
	}
	
  }

  //finish the block      
  out_block->id = block->id;
  out_block->length = block->length;

  return 0;
}