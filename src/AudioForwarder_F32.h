/*
 * AudioForwarding
 * Created: Chip Audette, OpenAudio, Sept 2024
 * Purpose: Accept 4 inputs and transmit to a 3rd-party's destinations.
 *    The "3rd party" is also an AudioStream_F32.
 * Assumes floating-point data.       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef AUDIOFORWARDER_F32_H
#define AUDIOFORWARDER_F32_H

#include "AudioStream_F32.h"

class AudioForwarder4_F32 : public AudioStream_F32 {
//GUI: inputs:4, outputs:4  //this line used for automatic generation of GUI node
//GUI: shortName:Forwarder4
public:
  AudioForwarder4_F32() : AudioStream_F32(4, inputQueueArray) { }
	AudioForwarder4_F32(const AudioSettings_F32 &settings) : AudioStream_F32(4, inputQueueArray) { }
	AudioForwarder4_F32(const AudioSettings_F32 &settings, AudioStream_F32 *dst) :
		AudioStream_F32(4, inputQueueArray) { setForwardingDestiation(dst); }
	
	void setForwardingDestiation(AudioStream_F32 *_dst) {
		forwarding_dest = _dst;
	}
	
	virtual void update(void) {
		audio_block_f32_t *block=NULL;
		for (int Ichan = 0; Ichan < num_inputs_f32; Ichan++) {
			//get any audio block that is waiting on this input
			block = receiveReadOnly_f32(Ichan);
						
			//Transmit to the 3rd-party's destination(s)
			if (block && (forwarding_dest != NULL)) forwarding_dest->transmit(block, Ichan);
			
			//release the block	
			AudioStream_F32::release(block);	
		}
	}

  private:
		const int max_n_IO_chan = 4;
		audio_block_f32_t *inputQueueArray[4] = {};
		AudioStream_F32 *forwarding_dest = NULL;
};


#endif