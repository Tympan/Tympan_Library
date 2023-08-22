
#include "AudioPlayMemory_F32.h"


bool AudioPlayMemoryI16_F32::play(const int16_t *_data_ptr, const uint32_t _data_len) {
  state = STOPPED;
  if ((_data_ptr == NULL) || (_data_len == 0)) return false; //nothing to play, so return
  data_ptr = _data_ptr;
  data_len = _data_len;
  data_ind = 0;  data_phase = 0.0f; //reset data index incrementing
  state = PLAYING;                  //start playing
  return isPlaying();
} 

#define CONVERT_I16_TO_F32(x) (((float32_t)x)/(16384.0f))

void AudioPlayMemoryI16_F32::update(void) {
  if (state != PLAYING) return;

  //get memory for the output
  audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if (out_block == NULL) return; //no memory available.

  //fill the memory
  for (uint32_t dest_ind = 0; dest_ind < ((uint32_t)out_block->length); dest_ind++) {

    //Serial.println("AudioPlayMemoryI16_F32: update: data_phase = " + String(data_phase) + ", data_ind = " + String(data_ind));
    
    if (data_ind < data_len) { //is there more audio to read in the memory buffer
      out_block->data[dest_ind] = CONVERT_I16_TO_F32(data_ptr[data_ind]); 

      //compute index for next time
      data_phase += data_phase_incr;                  //increment the phase
      uint32_t whole_step = (unsigned int)data_phase; //have we increased a whole step yet?
      data_ind += whole_step;                         //increment the data index by any whole steps
      data_phase -= (float32_t)whole_step;            //remove the whole step from the data_phase to keep it well bounded

    } else { //no, there is not more audio to read in the memory buffer
      
      state = STOPPED;
      out_block->data[dest_ind] = 0.0;
      
    }
  }

  //transmit and release
  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
}
