#include "AudioSummer_F32.h"

void AudioSummer4_F32::update(void) {
  audio_block_f32_t *in, *out=NULL;

  //get the first available channel
  int channel = 0;
  while  (channel < 4) {
	  out = receiveWritable_f32(channel);
	  if (out && flag_useChan[channel]) {
		  //yes, this is an audio block and use it
		  break;
	  } else {
		  //this may or may not be an audio block, but don't use it
		  if (!out) AudioStream_F32::release(out);	  
	  }
	  channel++;
  }
  if (!out) return;  //there was no data available.  so exit.
  
  //add in the remaining channels, as available and if enabled
  channel++;
  while  (channel < 4) {
    in = receiveReadOnly_f32(channel);
    if (in) {
		if (flag_useChan[channel]) {
			arm_add_f32(out->data, in->data, out->data, in->length);
		}
		AudioStream_F32::release(in);
	} else {
		//do nothing, this vector is empty
	}
	channel++;
  }

  if (out) {
    AudioStream_F32::transmit(out);
    AudioStream_F32::release(out);
  }
}
