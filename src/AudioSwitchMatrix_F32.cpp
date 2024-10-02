#include "AudioSwitchMatrix_F32.h"

void AudioSwitchMatrix4_F32::update(void) {
  audio_block_f32_t *block=NULL;
  
	//loop over the input channels
	for (int input_chan = 0; input_chan < num_inputs_f32; input_chan++) {
		//get the audio for this input
		block = receiveReadOnly_f32(input_chan);
		
		//is it real audio?
		if (block) {
			//loop over all output channels to find if it matches this inputForEachOutput	
			for (int output_chan = 0; output_chan < max_n_chan; output_chan++) {
				//check to see if the input for this output matches our current input
				if (input_chan == inputForEachOutput[output_chan]) {
					//it does match!  transmit it
					if (block) AudioStream_F32::transmit(block,output_chan);
				}
			}
		}
		
		//release the block
		AudioStream_F32::release(block);
	}
}
		
			
			
			

void AudioSwitchMatrix8_F32::update(void) {
  audio_block_f32_t *block=NULL;
  
	//loop over the input channels
	for (int input_chan = 0; input_chan < num_inputs_f32; input_chan++) {
		//get the audio for this input
		block = receiveReadOnly_f32(input_chan);
		
		//is it real audio?
		if (block) {
			//loop over all output channels to find if it matches this inputForEachOutput	
			for (int output_chan = 0; output_chan < max_n_chan; output_chan++) {
				//check to see if the input for this output matches our current input
				if (input_chan == inputForEachOutput[output_chan]) {
					//it does match!  transmit it
					if (block) AudioStream_F32::transmit(block,output_chan);
				}
			}
		}
		
		//release the block
		AudioStream_F32::release(block);
	}
}
