
#include "AudioEffectFreqShift_FD_F32.h"

int AudioEffectFreqShift_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT, const int _N_IFFT) {
	//print_ptr->println("AudioEffectFreqShift_FD_F32: setup: _N_FFT, _N_IFFT: " + String(_N_FFT) + " , " + String(_N_IFFT));
	int ret_val = AudioFreqDomainBase_FD_F32::setup(settings, _N_FFT, _N_IFFT);
	//print_ptr->println("AudioEffectFreqShift_FD_F32: setup: return from AudioFreqDomainBase_FD_F32::setup = " + String(ret_val));
	if (ret_val < 0) return ret_val; //it failed, so simply return now
	
	//decide how much overlap is happening
	switch (myIFFT.getNBuffBlocks()) {
	  case 0:
		//should never happen
		break;
	  case 1:
		overlap_amount = NONE;
		break;
	  case 2:
		overlap_amount = HALF;
		break;
	  case 3:
		//to do...need to add phase shifting logic to the update() function to support this case
		break;
	  case 4:
		overlap_amount = THREE_QUARTERS;
		//to do...need to add phase shifting logic to the update() function to support this case
		break;
	}
		
	return ret_val;
}

void AudioEffectFreqShift_FD_F32::shiftTheBins(float32_t *complex_2N_buffer, const int NFFT_input, const int NFFT_output, const int shift_bins) {
	const int N_2_input = NFFT_input/2;
	const int N_2_output = NFFT_output/2;
	int source_ind;
	
	if (shift_bins < 0) {
		for (int dest_ind = 0; dest_ind < N_2_output; dest_ind++) {
		  source_ind = dest_ind - shift_bins;  //shift_bins is negative, so source_ind is always positive
		  if (source_ind < N_2_input) {
			complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
			complex_2N_buffer[(2 * dest_ind) + 1] = complex_2N_buffer[(2 * source_ind) + 1]; //imaginary
		  } else {
			complex_2N_buffer[2 * dest_ind] = 0.0;
			complex_2N_buffer[(2 * dest_ind) + 1] = 0.0;
		  }
		}
	} else if (shift_bins > 0) {
		//do reverse order because, otherwise, we'd overwrite our source indices with zeros!
		for (int dest_ind = (N_2_output-1); dest_ind >= 0; dest_ind--) {
			source_ind = dest_ind - shift_bins; //shift_bins is positive, so source_ind could be negative
			if ((source_ind >= 0) && (source_ind < N_2_input)) {
				complex_2N_buffer[2 * dest_ind] = complex_2N_buffer[2 * source_ind]; //real
				complex_2N_buffer[(2 * dest_ind) + 1] = complex_2N_buffer[(2 * source_ind) +1]; //imaginary
			} else {
				complex_2N_buffer[2 * dest_ind] = 0.0;
				complex_2N_buffer[(2 * dest_ind) + 1] = 0.0;
			}
		}    
	}
}

// ////////////////////////////////
//
// Here's the tricky bit! We typically need to adjust the phase of each shifted FFT block 
// in order to account for the fact that the FFT blocks overlap in time, which means that
// their (original) phase evolves in a specific way.  We need to recreate that specific
// phase evolution in our shifted blocks.
//
// adjustThePhase():  Adjusts the phase of the bins to account for the fact that we have also moved
//   the audio content to a different bin than it started from.
//
//	* complex buffer is interleaved real and imaginary values for each FFT bin
//	* N/2 is the number of bins (including nyquist) to process
//
void rotate_90deg(float32_t *complex_2N_buffer, const int N_2) {
	//90 deg rotation (swap real and imaginary and flip the sign when moving the imaginary to the real)
	for (int i=0; i < N_2; i++) {
		const float32_t foo = complex_2N_buffer[2*i+1]; // hold onto the original imaginary value
		complex_2N_buffer[2*i+1] = complex_2N_buffer[2*i]; //put the real value into the imaginary
		complex_2N_buffer[2*i] = -foo;  //put the imaginary value into the real and flip sign
	}
}
void rotate_180deg(float32_t *complex_2N_buffer, const int N_2) {
	//Adding 180 is the same as flipping the sign of both the real and imaginary components
	for (int i=0; i < N_2; i++) {
		complex_2N_buffer[2*i] = -complex_2N_buffer[2*i];
		complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i+1];
	}
}
void rotate_270deg(float32_t *complex_2N_buffer, const int N_2) {
	//270 deg rotation (swap the real and imaginary and flip the sign when moving the real to the imaginary)
	for (int i=0; i < N_2; i++) {
		const float32_t foo = complex_2N_buffer[2*i+1];  //hold onto the original imaginary value
		complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i]; //put the real into the imaginary, but flip the sign
		complex_2N_buffer[2*i] = foo;  //put the imaginary into the real
	}
}
void AudioEffectFreqShift_FD_F32::adjustBinPhases(float32_t *complex_2N_buffer, const int N_FFT) {
	const int N_2 = N_FFT / 2 + 1;
	switch (overlap_amount) {
		case NONE:
			//no phase change needed
			break;
			
		case HALF:
			//we only need to adjust the phase if we're shifting by an odd number of bins
			if ((abs(shift_bins) % 2) == 1) {
				//Alternate between adding no phase shift and adding 180 deg phase shift.
				overlap_block_counter++; 
				if (overlap_block_counter == 2) {
					overlap_block_counter = 0;
					rotate_180deg(complex_2N_buffer, N_2);
				}
			}
			break;
			
		case THREE_QUARTERS:
			//The cycle of phase shifting is every 4 blocks insead of every two blocks.
			//Phase_shift = Phase_orig - Phase_new
			//  phase_orig = (360/4) * F_orig * block_counter;  //F_orig = 0 -> Nfft/2+1
			//  phase_new  = (360/4) * F_new * block_counter;
			//  So, phase_shift = (360/4) * (F_new - F_orig) * block_counter;  //wrap this zero to 360 
			//  Or, rewritten: phase_shift_deg = (360/4) * -shift_bins * block_counter //wrap this zero to 360
			//  Or, rewritten again: phase_shift_quarters = wrap(-shift_bins * block_counter, 4); //wrapped to always be [0, 1, 2, 3]
			overlap_block_counter++; if (overlap_block_counter >= 4) overlap_block_counter = 0; //will be [0, 1, 2, 3]
			int phase_shift_quarters = shift_bins * overlap_block_counter;
			while (phase_shift_quarters < 0) phase_shift_quarters += 4;  //wrap to get to zero or above
			while (phase_shift_quarters >= 4) phase_shift_quarters -= 4; //wrap get to less than 4
			
			switch (phase_shift_quarters) {
				case 0:
					//no rotation
					break;
				case 1:
					//90 deg rotation (swap real and imaginary and flip the sign when moving the imaginary to the real)
					rotate_90deg(complex_2N_buffer, N_2);
					break;
				case 2:
					//180 deg...flip the sign of both real and imaginary
					rotate_180deg(complex_2N_buffer, N_2);
					break;
				case 3:
					//270 deg rotation (swap the real and imaginary and flip the sign when moving the real to the imaginary)
					rotate_270deg(complex_2N_buffer, N_2);
					break;	
			}
			break;
		
	}
}


// /////////////////////////////////////////////////////////////////
//
// processAudioFD() is called by the parent class's update() method
//
void AudioEffectFreqShift_FD_F32::processAudioFD(float32_t *complex_data) {
	if (complex_data == nullptr) return;

	//Be aware that size of the FFT for the input and of the IFFT for the output could be different
	//from each other if we're also using this FFT/IFFT process for resampling.  
	const int N_FFT_input = getNFFT(); 
	const int N_FFT_output = getNIFFT();

	// do any preprocessing of the freq-domain data (right now, this does nothing...
	// but if you derive your own class from this class, you could override this function
	// to insert your own pre-processing here.)
	preprocessFreqDomainData(complex_2N_buffer, N_FFT_input); 

	//shift the frequency bins around as desired
	shiftTheBins(complex_2N_buffer, N_FFT_input, N_FFT_output, shift_bins);

	//here's the tricky bit! We typically need to adjust the phase of each shifted FFT block 
	//in order to account for the fact that the FFT blocks overlap in time, which means that
	//their (original) phase evolves in a specific way.  We need to recreate that specific
	//phase evolution in our shifted blocks.
	adjustBinPhases(complex_2N_buffer, N_FFT_output); //also uses overlap_amount and overlap_counter
		   
	//zero out the new DC and new nyquist
	//complex_2N_buffer[0] = 0.0;  complex_2N_buffer[1] = 0.0;
	//complex_2N_buffer[N_2] = 0.0;  complex_2N_buffer[N_2] = 0.0;

	//rebuild the negative frequency space (myIFFT knows its N_IFFT size)
	// This is done by the parent class's update() method
	// myIFFT.rebuildNegativeFrequencySpace(complex_2N_buffer); //set the negative frequency space based on the positive


	// we're done!
	return;
};