#include "AudioSwitchMatrix_F32.h"

void AudioSwitchMatrix4_F32::update(void) {
  audio_block_f32_t *block=NULL;
  
	for (int output_chan = 0; output_chan < max_n_chan; output_chan++) {
		int input_chan = inputForEachOutput[output_chan];
		if ((input_chan >= 0) && (input_chan < max_n_chan)) {
			block = receiveReadOnly_f32(input_chan);
			if (block) AudioStream_F32::transmit(block,output_chan); //just output to the one channel	
			AudioStream_F32::release(block);	
		}
	}		
}

void AudioSwitchMatrix8_F32::update(void) {
  audio_block_f32_t *block=NULL;
  
	for (int output_chan = 0; output_chan < max_n_chan; output_chan++) {
		int input_chan = inputForEachOutput[output_chan];
		if ((input_chan >= 0) && (input_chan < max_n_chan)) {
			block = receiveReadOnly_f32(input_chan);
			if (block) AudioStream_F32::transmit(block,output_chan); //just output to the one channel	
			AudioStream_F32::release(block);	
		}
	}		
}
