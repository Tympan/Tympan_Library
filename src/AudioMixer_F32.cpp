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

//alternative approach that breaks up the AudioStream management from the actual mixing.

/* void AudioMixer4_F32::update(void) {
  audio_block_f32_t *audio_in[4];
  audio_block_f32_t *audio_out = allocate_f32();
  if (!audio_out) { AudioStream_F32::release(audio_out);return; }  //no memory!

  //get the input data
  for (int channel = 0; channel < 4; channel++) audio_in[channel] = receiveReadOnly_f32(channel);

  //do the actual work
  processData(audio_in, audio_out);
  
  //release the inputs
  for (int channel = 0; channel < 4; channel++) AudioStream_F32::release(audio_in[channel]);
  
  //transmit the results
  AudioStream_F32::transmit(out);
  AudioStream_F32::release(out);

}
*/

//Note audio_in can be read-only as none of the operations are in-place
int AudioMixer4_F32::processData(audio_block_f32_t *audio_in[4], audio_block_f32_t *audio_out) {
	if (audio_out == NULL) return -1;
	bool firstValidAudio = true;
	audio_block_f32_t *tmp = allocate_f32();	
	if (tmp == NULL) return -1;  //no memory!
	int num_channels_mixed = 0;
	
	
	//loop over channels
	for (int channel = 0; channel < 4; channel++) {
		if (audio_in[channel] != NULL) {  //is it valid audio
		
			if (firstValidAudio) {
				//this is the first audio, so simply scale and have the scaling operation copy directly to audio_out
				firstValidAudio = false;
				arm_scale_f32(audio_in[channel]->data, multiplier[channel], audio_out->data, audio_in[channel]->length);
				audio_out->id = audio_in[channel]->id;
				audio_out->length = audio_in[channel]->length;

			} else {
				//scale the input data (holding in tmp) and then add tmp to the existing audio_out
				arm_scale_f32(audio_in[channel]->data, multiplier[channel], tmp->data, audio_in[channel]->length);
				arm_add_f32(audio_out->data, tmp->data, audio_out->data, audio_out->length);
			}
			num_channels_mixed++;
		}
	}
	
	//release the temporarily allocated block
	AudioStream_F32::release(tmp); 
	
	//we're done!
	return num_channels_mixed;
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
  if (!in) {AudioStream_F32::release(out); return;}  //there was no data available.  so exit.
  
  //process the first channel
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