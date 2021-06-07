
#include "AudioEffectFreqShift_FD_F32.h"


void AudioEffectFreqShift_FD_F32::update(void)
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
	unsigned long incoming_id = in_audio_block->id;
	AudioStream_F32::release(in_audio_block);  //We just passed ownership of in_audio_block to myFFT, so we can release it here as we won't use it here again.

	// ////////////// Do your processing here!!!

	//define some variables
	int fftSize = myFFT.getNFFT();
	int N_2 = fftSize / 2 + 1;
	int source_ind; // neg_dest_ind;

	//zero out DC and Nyquist
	//complex_2N_buffer[0] = 0.0;  complex_2N_buffer[1] = 0.0;
	//complex_2N_buffer[N_2] = 0.0;  complex_2N_buffer[N_2] = 0.0;  

	//do the shifting
	if (shift_bins < 0) {
		for (int dest_ind=0; dest_ind < N_2; dest_ind++) {
		  source_ind = dest_ind - shift_bins;
		  if (source_ind < N_2) {
			complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
			complex_2N_buffer[(2 * dest_ind) + 1] = complex_2N_buffer[(2 * source_ind) + 1]; //imaginary
		  } else {
			complex_2N_buffer[2 * dest_ind] = 0.0;
			complex_2N_buffer[(2 * dest_ind) + 1] = 0.0;
		  }
		}
	} else if (shift_bins > 0) {
		//do reverse order because, otherwise, we'd overwrite our source indices with zeros!
		for (int dest_ind=N_2-1; dest_ind >= 0; dest_ind--) {
			source_ind = dest_ind - shift_bins;
			if (source_ind >= 0) {
				complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
				complex_2N_buffer[(2 * dest_ind) + 1] = complex_2N_buffer[(2 * source_ind) +1]; //imaginary
			} else {
				complex_2N_buffer[2 * dest_ind] = 0.0;
				complex_2N_buffer[(2 * dest_ind) + 1] = 0.0;
			}
		}    
	}
  
	//here's the tricky bit!  Phase shifting may be needed through time
	switch (overlap_amount) {
		case NONE:
			//no phase change needed
			break;
		case HALF:
			//we only need to adjust the phase if we're shifting by an odd number of bins
			if ((abs(shift_bins) % 2) == 1) {
				//Alternate between adding no phase shift and adding 180 deg phase shift.
				//Adding 180 is the same as flipping the sign of both the real and imaginary components
				overlap_block_counter++;
				if (overlap_block_counter == 2){
					overlap_block_counter = 0;
					for (int i=0; i < N_2; i++) {
						complex_2N_buffer[2*i] = -complex_2N_buffer[2*i];
						complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i+1];
					}
				}
			}
			break;
		case THREE_QUARTERS:
			//The cycle of phase shifting is every 4 blocks insead of every two blocks.
			overlap_block_counter++; //will be 1 to 4
			if (overlap_block_counter == 4) overlap_block_counter = 0;
			if ((abs(shift_bins) % 2) == 1) { //THIS ISN'T RIGHT ?!?!?  Needs to be fancier, "% 4" instead of "% 2" ???
				float foo;
				switch (overlap_block_counter) {
					case 0:
						//no rotation
						break;
					case 1:
						//90 deg rotation (swap real and imaginary and flip the sign when moving the real to imaginary)
						for (int i=0; i < N_2; i++) {
							foo = complex_2N_buffer[2*i+1];
							complex_2N_buffer[2*i+1] = complex_2N_buffer[2*i];
							complex_2N_buffer[2*i] = -foo;
						}
						break;
					case 2:
						//180 deg...flip the sign of both real and imaginary
						for (int i=0; i < N_2; i++) {
							complex_2N_buffer[2*i] = -complex_2N_buffer[2*i];
							complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i+1];
						}
						break;
					case 3:
						//270 deg rotation (swap the real and imaginary and flip the sign when moving the imaginary to the real)
						for (int i=0; i < N_2; i++) {
							foo = complex_2N_buffer[2*i+1];
							complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i];
							complex_2N_buffer[2*i] = foo;
						}
						overlap_block_counter = 0;
						break;	
				}
			}
			break;			
	}
		   
  
	//zero out the new DC and new nyquist
	//complex_2N_buffer[0] = 0.0;  complex_2N_buffer[1] = 0.0;
	//complex_2N_buffer[N_2] = 0.0;  complex_2N_buffer[N_2] = 0.0;


	//rebuild the negative frequency space
	myFFT.rebuildNegativeFrequencySpace(complex_2N_buffer); //set the negative frequency space based on the positive


	// ///////////// End do your processing here

	//call the IFFT
	audio_block_f32_t *out_audio_block = myIFFT.execute(complex_2N_buffer); //out_block is pre-allocated in here.
	
	//update the block number to match the incoming one
	out_audio_block->id = incoming_id;

	//send the returned audio block.  Don't issue the release command here because myIFFT will re-use it
	AudioStream_F32::transmit(out_audio_block); //don't release this buffer because myIFFT re-uses it within its own code
	return;
};