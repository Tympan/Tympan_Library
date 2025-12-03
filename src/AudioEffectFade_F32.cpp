
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
  float low_lim = 0.5f*abs(step_size_amp);
  float up_lim = 1.0f - 0.5f*abs(step_size_amp);

//	static int count = 0;
//	if (count > 100) {
//		Serial.print("AudioEffectFade_F32::processAudioBlock: cur_amp: ");
//		Serial.print(cur_amp,8);
//		Serial.print(", step_size: ");Serial.print(step_size_amp,8);
//		Serial.print(", up_lim = ");	Serial.print(up_lim,8);
//		Serial.println(", is_active = " + String(is_active));
//		count = 0;
//	}
//	count++;
	
  //loop over each sample
  for (int i = 0; i < block->length; i++) {

		//apply the fade
		out_block->data[i] = cur_amp * (block->data[i]);

		//are we actively fading?
		if (is_active) {  
		
			//increment the fade (with limiting)
			cur_amp = max(0.f, min(1.f, cur_amp + step_size_amp));

			//test to see if we're done with the fade
			if ((step_size_amp < 0.f) && (cur_amp < low_lim)) {
				cur_amp = 0.f;
				is_active = false;
			} else if ((step_size_amp > 0.f) && (cur_amp > up_lim)) {
				cur_amp = 1.f;
				is_active = false;
			}
			
		}
	
  }

  //finish the block      
  out_block->id = block->id;
  out_block->length = block->length;

  return 0;
}