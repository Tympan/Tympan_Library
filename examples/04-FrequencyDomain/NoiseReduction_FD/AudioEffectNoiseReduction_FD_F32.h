
#ifndef _AudioEffectNoiseReduction_FD_F32_h
#define _AudioEffectNoiseReduction_FD_F32_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

// THIS IS AN EXAMPLE OF HOW TO CREATE YOUR OWN FREQUENCY-DOMAIN ALGRITHMS
// TRY TO DO YOUR OWN THING HERE! HAVE FUN!

//Create an audio processing class to do the lowpass filtering in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT/IFFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.

class AudioEffectNoiseReduction_FD_F32 : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioEffectNoiseReduction_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};

    //destructor...release all of the memory that has been allocated
    ~AudioEffectNoiseReduction_FD_F32(void) {
      if (prev_gains != NULL) delete prev_gains;
      if (gains != NULL) delete gains;
      if (ave_spectrum != NULL) delete ave_spectrum;
    }

    //setup...extend the setup that is part of AudioFreqDomainBase_FD_F32
    int setup(const AudioSettings_F32 &settings, const int target_N_FFT);
   
    // get/set methods specific to this particular frequency-domain algorithm
    float32_t getAveSpectrumN(void) { return myFFT.getNFFT(); }; //myFFT is part of AudioFreqDomainBase_FD_F32
    float32_t getAveSpectrum(int ind) { if (ind < ave_spectrum_N) return ave_spectrum[ind]; }
    float32_t *getAveSpectrumPtr(void) { return ave_spectrum; }
    float32_t setAttack_sec(float32_t att) { 
      if (att >= 0.001f) attack_sec = att; 
      attack_coeff = calcCoeffGivenTimeConstant(attack_sec, getOverlappedFFTRate_Hz());     //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return attack_sec; 
    }
    float32_t getAttack_sec(void) { return attack_sec; }
    float32_t setRelease_sec(float32_t rel) { 
      if (rel >= 0.001f) release_sec = rel; 
      release_coeff = calcCoeffGivenTimeConstant(release_sec, getOverlappedFFTRate_Hz()); //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return release_sec; 
    }
    float32_t getRelease_sec(void) { return release_sec; }
    float32_t setMaxAttenuation_dB(float32_t atten_dB) { 
      max_gain = sqrtf(powf(10.0f, 0.1f*(-atten_dB))); //linear value, amplitude not power
      return getMaxAttenuation_dB();
    }
    float32_t getMaxAttenuation_dB(void) { return -10.0f*log10f(max_gain*max_gain); }
    float32_t setSNRforMaxAttenuation_dB(float32_t val_dB) { 
      SNR_for_max_atten = powf(10.0f, 0.1f*val_dB); //linear not dB, but it is power (ie, signal^2)
      return getSNRforMaxAttenuation_dB();
      };
    float32_t getSNRforMaxAttenuation_dB(void) { return 10.0f*log10f(SNR_for_max_atten); };
    float32_t setTransitionWidth_dB(float32_t val_dB) { 
      val_dB = max(1.0f, val_dB);
      transition_width = powf(10.0f, 0.1f*val_dB);  //linear not dB, but it is power (ie, signal^2)
      return getTransitionWidth_dB();
    };
    float32_t getTransitionWidth_dB(void) { return 10.0f*log10f(transition_width); }
    float32_t setGainSmoothing_octaves(float32_t val_oct) { return freq_smooth_octaves = max(0.0,val_oct); }
    float32_t getGainSmoothing_octaves(void) { return freq_smooth_octaves; }
    float32_t setGainSmoothing_sec(float32_t val_sec) {
      if (val_sec >= 0.001f) smooth_sec = val_sec;
      smooth_coeff = calcCoeffGivenTimeConstant(smooth_sec, getOverlappedFFTRate_Hz()); //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32;
      return smooth_sec;
    }
    float32_t getGainSmoothing_sec(void) { return smooth_sec; }
    float32_t calcCoeffGivenTimeConstant(float32_t time_const_sec, float32_t block_rate_Hz) {
      //http://www.tsdconseil.fr/tutos/tuto-iir1-en.pdf
      //normally, this is computed using the sample rate.  We however will be applying the filter
      //only once every time the FFT is computed, so we substitute the FFT rate for the sample rate 
      float32_t val =  1.0f - expf(-1.0f / (time_const_sec * block_rate_Hz));
      return max(0.0f,min(1.0f, val));
    }
    bool setEnableNoiseEstimationUpdates(bool true_is_update) { return enableNoiseEstimationUpdates = true_is_update; }
    bool getEnableNoiseEstimationUpdates(void) { return enableNoiseEstimationUpdates; }
   
    void resetAveSpectrumAndGains(void) { for (int ind=0; ind < ave_spectrum_N; ind++) { ave_spectrum[ind]=0.0f; gains[ind] = 1.0f; prev_gains[ind]=1.0; } };
    void updateAveSpectrum(float32_t *current_pow);
    void calcGainsBasedOnSpectrum(float32_t *current_pow);
    void smoothGainsInTime(void);
    void smoothGainsInFrequency(void);

    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 

  protected:
    //create some data members specific to our processing
    float *ave_spectrum = NULL, *gains = NULL, *prev_gains = NULL;
    int ave_spectrum_N = 0;
    float32_t attack_sec = 10.0f, attack_coeff = 0;
    float32_t release_sec = 3.0f, release_coeff = 0;
    float32_t smooth_sec = 0.01f, smooth_coeff = 1.0; 
    bool enableNoiseEstimationUpdates = true;
    float32_t max_gain = 1.0;               //linear not dB, amplitude not power (so 2.0 is 6 dB)
    float32_t SNR_for_max_atten = 2.0;     //linear not dB, but it is power  (so, 2.0 is 3dB)
    float32_t transition_width = 4.0;      //linear not dB, but it is power  (so, 4.0 is 6dB)
    float32_t freq_smooth_octaves = 0.5;     //octave width of frequency-smoothing filter...larger results in more smoothing, lower results in less
   
};

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

//Here is the method we are overriding with our own algorithm...REPLACE THIS WITH WHATEVER YOU WANT!!!!
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

#endif
