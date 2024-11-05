
#include "AudioEffectFreqShift_FD_F32.h"

int AudioEffectFreqShift_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {
	sample_rate_Hz = settings.sample_rate_Hz;

	//setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
	N_FFT = myFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
	if (N_FFT < 1) return N_FFT;
	N_FFT = myIFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
	if (N_FFT < 1) return N_FFT;

	//decide windowing
	//Serial.println("AudioEffectFreqShift_FD_F32: setting myFFT to use hanning...");
	(myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
	#if 1
	if (myIFFT.getNBuffBlocks() > 3) {
	  //Serial.println("AudioEffectFormantShiftFD_F32: setting myIFFT to use hanning...");
	  (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
	}
	#endif

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
		

	#if 0
	//print info about setup
	Serial.println("AudioEffectFreqShift_FD_F32: FFT parameters...");
	Serial.print("    : N_FFT = "); Serial.println(N_FFT);
	Serial.print("    : audio_block_samples = "); Serial.println(settings.audio_block_samples);
	Serial.print("    : FFT N_BUFF_BLOCKS = "); Serial.println(myFFT.getNBuffBlocks());
	Serial.print("    : IFFT N_BUFF_BLOCKS = "); Serial.println(myIFFT.getNBuffBlocks());
	Serial.print("    : FFT use window = "); Serial.println(myFFT.getFFTObject()->get_flagUseWindow());
	Serial.print("    : IFFT use window = "); Serial.println((myIFFT.getIFFTObject())->get_flagUseWindow());
	#endif

	//allocate memory to hold frequency domain data
	complex_2N_buffer = new float32_t[2 * N_FFT];

	//we're done.  return!
	enabled = 1;
	return N_FFT;
}

void AudioEffectFreqShift_FD_F32::shiftTheBins(float32_t *complex_2N_buffer, int NFFT, int shift_bins) {
	int N_2 = NFFT/2;
	int source_ind;
	
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
}

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

	//zero out DC and Nyquist
	//complex_2N_buffer[0] = 0.0;  complex_2N_buffer[1] = 0.0;
	//complex_2N_buffer[N_2] = 0.0;  complex_2N_buffer[N_2] = 0.0;  

	//shift the frequency bins around as desired
	shiftTheBins(complex_2N_buffer, myFFT.getNFFT(), shift_bins);
  
	//here's the tricky bit! We typically need to adjust the phase of each shifted FFT block 
	//in order to account for the fact that the FFT blocks overlap in time, which means that
	//their (original) phase evolves in a specific way.  We need to recreate that specific
	//phase evolution in our shifted blocks.
	int N_2 = myFFT.getNFFT() / 2 + 1;
	switch (overlap_amount) {
		case NONE:
			//no phase change needed
			break;
			
		case HALF:
			//we only need to adjust the phase if we're shifting by an odd number of bins
			if ((abs(shift_bins) % 2) == 1) {
				//Alternate between adding no phase shift and adding 180 deg phase shift.
				overlap_block_counter++; 
				if (overlap_block_counter == 2){
					overlap_block_counter = 0;
					for (int i=0; i < N_2; i++) {
						//Adding 180 is the same as flipping the sign of both the real and imaginary components
						complex_2N_buffer[2*i] = -complex_2N_buffer[2*i];
						complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i+1];
					}
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
			
			float foo;
			switch (phase_shift_quarters) {
				case 0:
					//no rotation
					break;
				case 1:
					//90 deg rotation (swap real and imaginary and flip the sign when moving the imaginary to the real)
					for (int i=0; i < N_2; i++) {
						foo = complex_2N_buffer[2*i+1]; // hold onto the original imaginary value
						complex_2N_buffer[2*i+1] = complex_2N_buffer[2*i]; //put the real value into the imaginary
						complex_2N_buffer[2*i] = -foo;  //put the imaginary value into the real and flip sign
					}
					break;
				case 2:
					//180 deg...flip the sign of both real and imaginary
					for (int i=0; i < N_2; i++) {
						complex_2N_buffer[2*i] = -complex_2N_buffer[2*i];  //real into real, but flip sign
						complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i+1]; //imaginary into imaginary but flip sign
					}
					break;
				case 3:
					//270 deg rotation (swap the real and imaginary and flip the sign when moving the real to the imaginary)
					for (int i=0; i < N_2; i++) {
						foo = complex_2N_buffer[2*i+1];  //hold onto the original imaginary value
						complex_2N_buffer[2*i+1] = -complex_2N_buffer[2*i]; //put the real into the imaginary, but flip the sign
						complex_2N_buffer[2*i] = foo;  //put the imaginary into the real
					}
					break;	
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
	audio_block_f32_t *out_audio_block = AudioStream_F32::allocate_f32();
	if (out_audio_block == NULL) {AudioStream_F32::release(out_audio_block); return; }//out of memory!
	myIFFT.execute(complex_2N_buffer, out_audio_block); //output is via out_audio_block
	
	//update the block number to match the incoming one
	out_audio_block->id = incoming_id;

	//send the output
	AudioStream_F32::transmit(out_audio_block);
	AudioStream_F32::release(out_audio_block);
	return;
};