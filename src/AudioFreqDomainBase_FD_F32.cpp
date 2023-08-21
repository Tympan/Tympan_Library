
#include "AudioFreqDomainBase_FD_F32.h"
//#include <arm_math.h>

int AudioFreqDomainBase_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {
  sample_rate_Hz = settings.sample_rate_Hz;
  audio_block_samples = settings.audio_block_samples;
  int N_FFT;

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

  //convert to frequency domain
  myFFT.execute(in_audio_block, complex_2N_buffer);
  unsigned long incoming_id = in_audio_block->id;  //save for use later
  AudioStream_F32::release(in_audio_block);  //We just passed ownership to myFFT, so release it here.

  // ////////////// Do your processing here!!!

  //define some variables
  processAudioFD(complex_2N_buffer, myFFT.getNFFT());  //in your derived class, overwrite this!!

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



//Here is the method for you to override with your own algorithm!
//  * The first argument that you will receive is the float32_t *, which is an array that is allocated in
//      the setup() method.  It is 2*NFFT in length because it contains the real and imaginary data values
//      for each bin.  Real and imaginary are interleaved
//  * The second argument is the number of fft bins
//Note that you only need to touch the bins associated with zero through Nyquist.  The update() method
//  above will reconstruct the bins above Nyquist for you.  It does this by taking the complex conjugate
//  of the bins below Nyquist.  Easy for you!
//
//Get your data from complex_2N_buffer and put your results back into complex_2N_buffer
void AudioFreqDomainBase_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {

  //below are some examples of things that might be useful to you
  
  /*
  int N_2 = fftSize / 2 + 1;
  float Hz_per_bin = sample_rate_Hz / fftSize;
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