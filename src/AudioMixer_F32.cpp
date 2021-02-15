#include "AudioMixer_F32.h"

void AudioMixer4_F32::update(void) {
  audio_block_f32_t *in, *out=NULL;

  //get the first available channel
  int channel = 0;
  while  (channel < 4) {
	  out = receiveWritable_f32(channel);
	  if (out) break;
	  channel++;
  }
  if (!out) return;  //there was no data available.  so exit.
  arm_scale_f32(out->data, multiplier[channel], out->data, out->length);  //there was data, so scale it per the mier
  
  //add in the remaining channels, as available
  channel++;
  while  (channel < 4) {
    in = receiveReadOnly_f32(channel);
    if (in) {
		audio_block_f32_t *tmp = allocate_f32();

		arm_scale_f32(in->data, multiplier[channel], tmp->data, tmp->length);
		arm_add_f32(out->data, tmp->data, out->data, tmp->length);

		AudioStream_F32::release(tmp);
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

void AudioMixer8_F32::update(void) {
  audio_block_f32_t *in;
  audio_block_f32_t *out=allocate_f32();
  if (!out) return;  //there was no memory available

  //get the first available channel
  int channel = 0;
  while  (channel < 8) {
	  in = receiveReadOnly_f32(channel);
	  if (in) break;
	  channel++;
  }
  if (!in) return;  //there was no data available.  so exit.
  arm_scale_f32(in->data, multiplier[channel], out->data, in->length);  //there was data, so scale it per the multiplier
  out->length = in->length;
  AudioStream_F32::release(in);
  
  
  //add in the remaining channels, as available
  channel++;
  while  (channel < 8) {
    in = receiveReadOnly_f32(channel);
    if (in) {
		audio_block_f32_t *tmp = allocate_f32();

		arm_scale_f32(in->data, multiplier[channel], tmp->data, tmp->length);
		arm_add_f32(out->data, tmp->data, out->data, tmp->length);

		AudioStream_F32::release(tmp);
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