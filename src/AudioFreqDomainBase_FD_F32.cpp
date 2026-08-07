
#include "AudioFreqDomainBase_FD_F32.h"
//#include <arm_math.h>

int AudioFreqDomainBase_FD_F32::setup(const AudioSettings_F32 &in_settings, const int _N_FFT, const int _N_IFFT) {
	sample_rate_input_Hz = in_settings.sample_rate_Hz;  //sample rate for in-coming data
	bool flag_reallocate_complex_buffer = false;

	//setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
	int prev_N_FFT = N_FFT;
	if (prev_N_FFT != _N_FFT) {
		N_FFT = myFFT.setup(in_settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
		if (N_FFT < 1) {
			if (flag_printDebug) print_ptr->println(F("AudioFreqDomainBase_FD_F32: FAILED setting up myFFT for N_FFT = ") + String(_N_FFT) + "...");
			return -1;
		}
		flag_reallocate_complex_buffer = true;
	}
	

	int prev_N_IFFT = N_IFFT;
	if (prev_N_IFFT != _N_IFFT) {
		float32_t ratio_IFFT_to_FFT = static_cast<float32_t>(_N_IFFT)/static_cast<float32_t>(_N_FFT);
		AudioSettings_F32 out_settings = in_settings;
		out_settings.sample_rate_Hz = in_settings.sample_rate_Hz * ratio_IFFT_to_FFT;
		out_settings.audio_block_samples = static_cast<int>(in_settings.audio_block_samples * ratio_IFFT_to_FFT + 0.5f); // the +0.5f is to make the cast become a rounding
		N_IFFT = myIFFT.setup(out_settings, _N_IFFT); //hopefully, we got the same N_IFFT that we asked for
		if (N_IFFT < 1) {
			if (flag_printDebug) print_ptr->println(F("AudioFreqDomainBase_FD_F32: FAILED setting up myIFFT for N_IFFT = ") + String(_N_IFFT) + "...");
			return -1;
		}
		sample_rate_output_Hz = out_settings.sample_rate_Hz;
		audio_block_output_samples = out_settings.audio_block_samples;
		flag_reallocate_complex_buffer = true;
	}
	
	//decide windowing
	(myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
	#if 1
		if (myIFFT.getNBuffBlocks() > 3) (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
	#endif

 	if (flag_printDebug) {
		//print info about setup
		print_ptr->println(F("AudioEffectFreqShift_FD_F32: FFT parameters..."));
		print_ptr->print("    : N_FFT Requested = "); print_ptr->print(_N_FFT); print_ptr->print(", Actual = "); print_ptr->println(N_FFT);
		print_ptr->print("    : N_IFFT Requested = "); print_ptr->print(_N_IFFT); print_ptr->print(", Actual = "); print_ptr->println(N_IFFT);
		print_ptr->print("    : audio_block_samples = "); print_ptr->println(in_settings.audio_block_samples);
		print_ptr->print("    : FFT N_BUFF_BLOCKS = "); print_ptr->println(myFFT.getNBuffBlocks());
		print_ptr->print("    : IFFT N_BUFF_BLOCKS = "); print_ptr->println(myIFFT.getNBuffBlocks());
		print_ptr->print("    : FFT use window = "); print_ptr->println(myFFT.getFFTObject()->get_flagUseWindow());
		print_ptr->print("    : IFFT use window = "); print_ptr->println((myIFFT.getIFFTObject())->get_flagUseWindow());
		delay(30);  //give time for the print_ptr to spool out on slower systems
	}

	//allocate memory to hold frequency domain data
	if (flag_reallocate_complex_buffer) {
		if (complex_2N_buffer != nullptr) {
			delete[] complex_2N_buffer;	
			len_complex_2N_buffer = 0; //...this is a data member of this class
		}
		const int max_N_FFT = max(N_FFT, N_IFFT);    //how many bins of data should we be prepared to handle
		len_complex_2N_buffer = 2 * max_N_FFT;  //need both real and complex values for each bin...this is a data member of this class
		complex_2N_buffer = new float32_t[len_complex_2N_buffer];  //attempt to allocate the array
		if (complex_2N_buffer == nullptr) {  // if unsuccessful, the buffer will appear to still be a nullptr
			if (flag_printDebug) print_ptr->println(F("AudioEffectFreqShift_FD_F32: ...failed to allocate complex_2N_buffer."));
			len_complex_2N_buffer = 0;  //...this is a data member of this class
			return -1;
		}
	}

  //we're done.  return!
  enabled = 1;
  return N_FFT;
}

float AudioFreqDomainBase_FD_F32::setSampleRate_Hz(const float val_Hz) { 
  sample_rate_input_Hz = val_Hz; 
  sample_rate_Hz = sample_rate_input_Hz; //for historical compatibility only!  don't use this data member!
  const int N_FFT = getNFFT();
  const int N_IFFT = getNIFFT();
  if ((N_FFT > 1) & (N_IFFT > 1)) {
    sample_rate_output_Hz = sample_rate_input_Hz * static_cast<float32_t>(getNIFFT())/getNFFT();
  } else {
    //the FFT and/or IFFT have not been setup yet, so do something that isn't terrible
    sample_rate_output_Hz = sample_rate_input_Hz;
  }
  return getSampleRate_Hz();
}

void AudioFreqDomainBase_FD_F32::removeNegativeFrequencies(float32_t *complex_data, const int N_FFT_current, const int N_FFT_future) {
	//complex_data is 2N long, interleaved real and complex
	size_t ind = N_FFT_current/2+1;
	size_t end_ind = N_FFT_future/2+1;
	while (ind < end_ind) complex_data[ind++] = 0.0;
}

void AudioFreqDomainBase_FD_F32::update(void)
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

	//Be aware that size of the FFT for the input and of the IFFT for the output could be different
	//from each other if we're also using this FFT/IFFT process for resampling.  
	const int N_FFT_input = myFFT.getNFFT();
	const int N_FFT_output = myIFFT.getNFFT();

	//convert to frequency domain (myFFT already knows its N_FFT size)
	myFFT.execute(in_audio_block, complex_2N_buffer); //FFT is in complex_2N_buffer, interleaved real, imaginary, real, imaginary, etc

	//zero out the remaining complex_2N_buffer, if needed
	size_t ind = static_cast<size_t>(2*N_FFT_input);
	while (ind < len_complex_2N_buffer) { complex_2N_buffer[ind++] = 0.0f; } //zero out the rest of the buffer

	//get other info about the audio_block and then release it
	unsigned long incoming_id = in_audio_block->id;
	//float32_t incoming_fs_Hz = in_audio_block->fs_Hz;
	//int incoming_audio_block_samples = in_audio_block->length;
	AudioStream_F32::release(in_audio_block);  //We just passed ownership of in_audio_block to myFFT, so we can release it here as we won't use it here again.


	// ////////////// Do your processing here!!!


	// define some variables
	processAudioFD(complex_2N_buffer);  //in your derived class, override processAudioFD() with your own code!!

	// if upsampling iva the upcoming IFFT, lets zero out the "negative" fft bins of the existing
	// FFT representation in the complex2N buffer so that those values don't mess up the
	// reconstruction of the negative fft bins (that will happen below) for the new IFFT size
	const int N_FFT_currently = N_FFT_input;
	const int N_FFT_future = N_FFT_output;
	if (N_FFT_future > N_FFT_currently) removeNegativeFrequencies(complex_2N_buffer, N_FFT_currently, N_FFT_future);
	
	// rebuild the negative frequency space for the output IFFT
	myIFFT.rebuildNegativeFrequencySpace(complex_2N_buffer); //set the negative frequency space based on the positive


	// ///////////// End do your processing here

	//call the IFFT
	//const int N_FFT_output = myIFFT.getNFFT();
	audio_block_f32_t *out_audio_block = AudioStream_F32::allocate_f32();
	if (out_audio_block == NULL) {AudioStream_F32::release(out_audio_block); return; }//out of memory!
	myIFFT.execute(complex_2N_buffer, out_audio_block); //output is via out_audio_block
	
	//update the block metdata 
	out_audio_block->id     = incoming_id; //match the id of the incoming data block
	out_audio_block->fs_Hz  = sample_rate_output_Hz;
	out_audio_block->length = audio_block_output_samples;

	//send the output
	AudioStream_F32::transmit(out_audio_block);
	AudioStream_F32::release(out_audio_block);
  return;
};



//Here is the method for you to override with your own algorithm!
//  * The first argument that you will receive is the float32_t *, which is an array that is allocated in
//      the setup() method.  It is 2*NFFT in length because it contains the real and imaginary data values
//      for each bin.  Real and imaginary are interleaved
//
//Note that you only need to touch the bins associated with zero through Nyquist.  The update() method
//  above will reconstruct the bins above Nyquist for you.  It does this by taking the complex conjugate
//  of the bins below Nyquist.  Easy for you!
//
//Get your data from complex_2N_buffer and put your results back into complex_2N_buffer
void AudioFreqDomainBase_FD_F32::processAudioFD(float32_t *complex_2N_buffer) {
  //below are some examples of things that might be useful to you
  
  /*
  const int N_FFT = getNFFT();  //this is the size of the FFT (assuming you're using the same input and output sizes)
  int N_2 = N_FFT / 2 + 1;
  float Hz_per_bin = getSampleRate_Hz() / N_FFT;
  float bin_freq_Hz;
  */

  /*
  //In some other example, this might be a useful operation...getting the magnitude and phase of the bins.
  //here's a computationally efficient way to do it...call an optimized library for the mangitude
  float32_t orig_mag[N_2];
  arm_cmplx_mag_f32(complex_2N_buffer, orig_mag, N_2);  //get the magnitude for each FFT bin and store somewhere safes
  
  //here's a way to compute the phase
  float32_t phase_rad[N_2];
  for (ind=0; ind<N_2; ind++ ) phase_rad[ind] = atan2f(complex_2N_buffer[2*ind+1),complex_2N_buffer[2*ind]);

  //here's a way to compute the complex number back from the magnitude and phase
  float32_t new_complex_2N_buffer[2*NFFT];
  for (ind=0; ind<N_2; ind++) {
    new_complex_2N_buffer[ind*2]   = orig_mag[ind] * cosf(phase_rad[ind]);   //real
    new_complex_2N_buffer[ind*2+1] = orig_mag[ind] * sinf(phase_rad[ind]); //imaginary
  }
  */

  /*
  //As an example of something you cand do is to loop over each bin and do something useful
  for (int ind = 0; ind < N_2; ind++) { //only process up to Nyquist...the class will automatically rebuild the frequencies above Nyquist

    //do something...such as attenuate the signal if above 2000 Hz
    bin_freq_Hz = (float)ind * Hz_per_bin;
    gain_factor = 0.1;
    if (bin_freq_Hz > 2000.0f) {
      //attenuate both the real and imaginary comoponents
      complex_2N_buffer[2 * ind]     = gain_factor * complex_2N_buffer[2 * ind];     //real
      complex_2N_buffer[2 * ind + 1] = gain_factor * complex_2N_buffer[2 * ind + 1]; //imaginary
    } else {
      //do not change the audio
    }    
  }
  */
  
}
