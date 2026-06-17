
// MIT License, Use at your own risk.

#include "AudioEffectNoiseReduction_FD_F32.h"


int AudioEffectNoiseReduction_FD_F32::setup(const AudioSettings_F32 &settings, const int target_N_FFT) {
  int actual_N_FFT = AudioFreqDomainBase_FD_F32::setup(settings, target_N_FFT);
  ave_spectrum_N = actual_N_FFT / 2 + 1;  // zero bin through Nyquist bin
  ave_spectrum = new float32_t[ave_spectrum_N];
  gains = new float32_t[ave_spectrum_N];
  prev_gains = new float32_t[ave_spectrum_N];
  resetAveSpectrumAndGains();
  setAttack_sec(attack_sec);   //computes the underlying attack filter coefficient
  setRelease_sec(release_sec); //computes the underlying release filter coefficient
  setGainSmoothing_sec(smooth_sec); //computes the underlying smoothing filter coefficient
  return actual_N_FFT;
}

void AudioEffectNoiseReduction_FD_F32::updateAveSpectrum(float32_t *current_pow) {
    float32_t att_1 = 0.99999f*(1.0f - attack_coeff), att = attack_coeff;
    float32_t rel_1 = 0.99999f*(1.0f - release_coeff), rel = release_coeff;

    //loop over each bin to update the average for that bin
    for (int ind=0; ind < ave_spectrum_N; ind++) {
      if (current_pow[ind] > ave_spectrum[ind]) {
        //noise is increasing.  use attack time
        ave_spectrum[ind] = att_1 * ave_spectrum[ind] + att*current_pow[ind];
      } else {
        //noise is decreasing.  use release time
        ave_spectrum[ind] = rel_1 * ave_spectrum[ind] + rel*current_pow[ind];
      }
    }
}

void AudioEffectNoiseReduction_FD_F32::calcGainsBasedOnSpectrum(float32_t *current_pow) {
  //loop over each bin, evaluate where the current level is relative to the average, and compute the desired gain
  float32_t SNR;
  const float32_t SNR_at_endTransition = SNR_for_max_atten*transition_width;
  const float32_t gain_at_endTransition =1.0; //linear.  Gain=1.0 is gain of 0 dB

  //loop over each frequency bin (up to Nyquist)
  const float32_t coeff = (gain_at_endTransition - max_gain)/(SNR_at_endTransition - SNR_for_max_atten);
  for (int ind=0; ind < ave_spectrum_N; ind++) {
    SNR = current_pow[ind] / ave_spectrum[ind]; //compute signal to noise ratio (linear, not dB)

    //calculate gain differently based on different SNR regimes
    if (SNR <= SNR_for_max_atten) { //note SNR=1.0 is an SNR of 0 dB
      //this is definitely noise.  max attenuation
      gains[ind] = max_gain; //linear value, not dB
      
    } else if (SNR >=  SNR_at_endTransition) {
      //this is definitely signal.  no attenuation
      gains[ind] = gain_at_endTransition; //linear value, not dB
      
    } else {
      //we're in-between, so it's a transition region.  This transition is best done in dB space, but let's try it in linear space
      gains[ind]= (SNR - SNR_for_max_atten) * coeff + max_gain ;
    }
  }
}

//smooth gain values in frequency with simple first-order filter
void AudioEffectNoiseReduction_FD_F32::smoothGainsInFrequency(void) {
  const float32_t scale_fac_div2 = freq_smooth_octaves * 0.5f;
  int count=0, start_ind=0, end_ind=0;
  int n_ave_half = 0;
  
  //initialize
  float32_t orig_gains[ave_spectrum_N];
  for (int ind = 0; ind < ave_spectrum_N; ind++) orig_gains[ind] = gains[ind];

  //loop over gains and smooth with neighbors
  for (int ind = 1; ind < ave_spectrum_N; ind++) {
    float32_t foo_out = orig_gains[ind]; count = 1;

    //how much averaging?
    n_ave_half = (int)(ind*scale_fac_div2 + 0.5f); //round
    if (n_ave_half > 0) {

      //left half
      start_ind = max(0,ind - n_ave_half);      end_ind = ind;
      for(int i=start_ind; i<end_ind;i++) {
        foo_out += orig_gains[i]; count++;
      }

      if (ind < (ave_spectrum_N-1)) { //don't average to the right if we're at the last frequency bin
        //right half
        start_ind = ind+1;  end_ind = min(ave_spectrum_N-1, ind + n_ave_half);
        for(int i=start_ind; i<end_ind;i++) {
          foo_out += orig_gains[i]; count++;
        }
      }

      //complete the averaging
      foo_out /= ((float)count);
    }
    
    gains[ind] = foo_out;
  }
}


//smooth gain values in time with simple first-order filter
void AudioEffectNoiseReduction_FD_F32::smoothGainsInTime(void) {
  float32_t coeff_1 = 0.9999f*(1.0f-smooth_coeff);
  for (int ind = 0; ind < ave_spectrum_N; ind++) {
    gains[ind] = prev_gains[ind] = coeff_1 * prev_gains[ind] + smooth_coeff*gains[ind];
  }
}

//Here is the method that is the starting point
//
//  Argument 1: complex_2N_buffer is the float32_t array that holds the FFT results that we are going to
//     manipulate.  It is 2*NFFT in length because it contains the real and imaginary data values
//     for each FFT bin.  Real and imaginary are interleaved.  We only need to worry about the bins
//     up to Nyquist because AudioFreqDomainBase will reconstruct the freuqency bins above Nyquist for us.
//  Argument 2: NFFT, the number of FFT bins
//
//  We get our data from complex_2N_buffer and we put our results back into complex_2N_buffer
void AudioEffectNoiseReduction_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {
  if (ave_spectrum == NULL) return; //if the memory for the average has yet to be initialized, return early
  if (gains == NULL) return;  //if the memory for the gain has yet to be initialized, return early
  int N_2 = NFFT / 2 + 1;
  //float Hz_per_bin = sample_rate_Hz /((float)NFFT); //sample_rate_Hz is from the base class AudioFreqDomainBase_FD_F32
  
  //compute the magnitude^2 of each FFT bin (up to Nyquist)
  float raw_pow[N_2]; //create memory to hold the magnitude^2 values
  arm_cmplx_mag_squared_f32(complex_2N_buffer, raw_pow, N_2);  //get the magnitude for each FFT bin and store somewhere safes

  //loop over each bin and compute the long-term average, which we assume to be the "noise" background
  if (enableNoiseEstimationUpdates) updateAveSpectrum(raw_pow); //updates ave_spectrum, which is one of the data members of this class
 
  //calcluate the new gain values based on the current magnitude versus the ave magnitude
  calcGainsBasedOnSpectrum(raw_pow);

  //smooth the gains in frequency and in time (to reducting the "bubbling water" artifacts)
  smoothGainsInFrequency();
  smoothGainsInTime();

  //Loop over each bin and apply the gain
  for (int ind = 0; ind < ave_spectrum_N; ind++) { //only process up to Nyquist...the class will automatically rebuild the frequencies above Nyquist
    //attenuate both the real and imaginary comoponents
    complex_2N_buffer[2 * ind]     *= gains[ind]; //real
    complex_2N_buffer[2 * ind + 1] *= gains[ind]; //imaginary
  }  
}
