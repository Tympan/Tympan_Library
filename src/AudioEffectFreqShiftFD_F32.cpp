
#include "AudioEffectFreqShiftFD_F32.h"


void AudioEffectFreqShiftFD_F32::update(void)
{
  //get a pointer to the latest data
  audio_block_f32_t *in_audio_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_audio_block) return;

  //simply return the audio if this class hasn't been enabled
  if (!enabled) {
    AudioStream_F32::transmit(in_audio_block);
    AudioStream_F32::release(in_audio_block);
    return;
  }

  //convert to frequency domain
  myFFT.execute(in_audio_block, complex_2N_buffer); //FFT is in complex_2N_buffer, interleaved real, imaginary, real, imaginary, etc
  AudioStream_F32::release(in_audio_block);  //We just passed ownership to myFFT, so release it here.

  // ////////////// Do your processing here!!!

  //define some variables
  int fftSize = myFFT.getNFFT();
  int N_2 = fftSize / 2 + 1;
  int source_ind; // neg_dest_ind;
  //float source_ind_float, interp_fac;
  //float new_mag, scale;
  //float orig_mag[N_2];
  //int max_source_ind = (int)(((float)N_2) * (10000.0 / (48000.0 / 2.0))); //highest frequency bin to grab from (Assuming 48kHz sample rate)


  if (shift_bins < 0) {
    for (int dest_ind=0; dest_ind < N_2; dest_ind++) {
      source_ind = dest_ind - shift_bins;
      if (source_ind < N_2) {
        complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
        complex_2N_buffer[2 * dest_ind + 1] = complex_2N_buffer[2 * source_ind +1]; //imaginary
      } else {
        complex_2N_buffer[2 * dest_ind] = 0.0;
        complex_2N_buffer[2 * dest_ind + 1] = 0.0;
      }
    }
  } else if (shift_bins > 0) {
    //do reverse order because, otherwise, we'd overwrite our source indices with zeros!
    for (int dest_ind=N_2-1; dest_ind >= 0; dest_ind--) {
      source_ind = dest_ind - shift_bins;
      if (source_ind >= 0) {
        complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
        complex_2N_buffer[2 * dest_ind + 1] = complex_2N_buffer[2 * source_ind +1]; //imaginary
      } else {
        complex_2N_buffer[2 * dest_ind] = 0.0;
        complex_2N_buffer[2 * dest_ind + 1] = 0.0;
      }
    }    
  }
  
  //rebuild the negative frequency space
  myFFT.rebuildNegativeFrequencySpace(complex_2N_buffer); //set the negative frequency space based on the positive
  

  // ///////////// End do your processing here

  //call the IFFT
  audio_block_f32_t *out_audio_block = myIFFT.execute(complex_2N_buffer); //out_block is pre-allocated in here.


  //send the returned audio block.  Don't issue the release command here because myIFFT will re-use it
  AudioStream_F32::transmit(out_audio_block); //don't release this buffer because myIFFT re-uses it within its own code
  return;
};