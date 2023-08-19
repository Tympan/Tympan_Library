
#include "AudioEffectFormantShift_FD_F32.h"

int AudioEffectFormantShift_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {
	sample_rate_Hz = settings.sample_rate_Hz;

	//setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
	N_FFT = myFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
	if (N_FFT < 1) return N_FFT;
	N_FFT = myIFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
	if (N_FFT < 1) return N_FFT;

	//decide windowing
	//Serial.println("AudioEffectFormantShift_FD_F32: setting myFFT to use hanning...");
	(myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
	#if 1
	if (myIFFT.getNBuffBlocks() > 3) {
	  //Serial.println("AudioEffectFormantShift_FD_F32: setting myIFFT to use hanning...");
	  (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
	}
	#endif

	#if 0
	  //print info about setup
	  Serial.println("AudioEffectFormantShift_FD_F32: FFT parameters...");
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


void AudioEffectFormantShift_FD_F32::update(void)
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
	AudioStream_F32::release(in_audio_block);  //We just passed ownership of in_audio_block to myFFT, so release it here.

	// ////////////// Do your processing here!!!

	//define some variables
	int fftSize = myFFT.getNFFT();
	int N_2 = fftSize / 2 + 1;
	int source_ind; // neg_dest_ind;
	float source_ind_float, interp_fac;
	float new_mag, scale;
	float orig_mag[N_2];
	//int max_source_ind = (int)(((float)N_2) * (10000.0 / (48000.0 / 2.0))); //highest frequency bin to grab from (Assuming 48kHz sample rate)

	#if 1
	float max_source_Hz = 10000.0; //highest frequency to use as source data
	int max_source_ind = min(int(max_source_Hz / sample_rate_Hz * fftSize + 0.5),N_2);
	#else
	int max_source_ind = N_2;  //this line causes this feature to be defeated
	#endif

	//get the magnitude for each FFT bin and store somewhere safe
	arm_cmplx_mag_f32(complex_2N_buffer, orig_mag, N_2);

	//now, loop over each bin and compute the new magnitude based on shifting the formants
	for (int dest_ind = 1; dest_ind < N_2; dest_ind++) { //don't start at zero bin, keep it at its original

	//what is the source bin for the new magnitude for this current destination bin
	source_ind_float = (((float)dest_ind) / shift_scale_fac) + 0.5;
	//source_ind = (int)(source_ind_float+0.5);  //no interpolation but round to the neariest index
	//source_ind = min(max(source_ind,1),N_2-1);
	source_ind = min(max(1, (int)source_ind_float), N_2 - 2); //Chip: why -2 and not -1?  Because later, for for the interpolation, we do a +1 and we want to stay within nyquist
	interp_fac = source_ind_float - (float)source_ind;
	interp_fac = max(0.0, interp_fac);  //this will be used in the interpolation in a few lines

	//what is the new magnitude
	new_mag = 0.0; scale = 0.0;
	if (source_ind < max_source_ind) {

	  //interpolate in the original magnitude vector to find the new magnitude that we want
	  //new_mag=orig_mag[source_ind];  //the magnitude that we desire
	  //scale = new_mag / orig_mag[dest_ind];//compute the scale factor
	  new_mag = orig_mag[source_ind];
	  new_mag += interp_fac * (orig_mag[source_ind] - orig_mag[source_ind + 1]);
	  scale = new_mag / orig_mag[dest_ind];

	  //apply scale factor
	  complex_2N_buffer[2 * dest_ind] *= scale; //real
	  complex_2N_buffer[2 * dest_ind + 1] *= scale; //imaginary
	} else {
	  complex_2N_buffer[2 * dest_ind] = 0.0; //real
	  complex_2N_buffer[2 * dest_ind + 1] = 0.0; //imaginary
	}

	//zero out the lowest bin
	complex_2N_buffer[0] = 0.0; //real
	complex_2N_buffer[1] = 0.0; //imaginary
	}

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