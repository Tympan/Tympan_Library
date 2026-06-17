
#ifndef _AudioEffectNoiseReduction_FD_F32_h
#define _AudioEffectNoiseReduction_FD_F32_h

/*
  AudioEffectNoiseReduction_FD_F32 
  Created: Chip Audette (OpenAudio)

  Purpose: Reduce background noise in an audio stream.

  Frequency-Domain Processing: This is a frequency-domain processing algorithm, meaning that 
  the Tympan passes it a segment of regular audio and this algorithm uses FFT processing to
  convert it as energy at different frequencies.

  Noise vs Signal: for this algorithm, "noise" is considered any frequency component that is
  steady in time.  "Signal" is anothing that changes in time and becomes louder than the noise.

  Algorithm Approcah: This algorithm assesses the average level through time for each FFT bin.
  This is the estimate of the noise.  It then looks at the current value (the current amount of
  energy) in each current FFT bin.  If the bin is larger than the long-term average, it is
  considered "signal" and is passed through unchanged.  Alternatively, if the bin is close to
  the long-term average, it is considered "noise" and it is attenuated.

  The user-set parameters of this algorithm include:
      * the time constants for the averaging
      * the threshold and transition width for how far the signal must be from the average level
        to be considered "signal"
      * the amount of attenuation if it's decided to be "noise"
      * the amount smoothing in both time and frequency of the amount of attenuation

  MIT License, Use at your own risk.
*/

#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

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
    int setup(const AudioSettings_F32 &settings, const int target_N_FFT) override;
   
    // get/set methods specific to this particular frequency-domain algorithm
    virtual float32_t getAveSpectrumN(void) { return myFFT.getNFFT(); }; //myFFT is part of AudioFreqDomainBase_FD_F32
    virtual float32_t getAveSpectrum(int ind) { if (ind < ave_spectrum_N) { return ave_spectrum[ind];} return 0.0;}
    virtual float32_t *getAveSpectrumPtr(void) { return ave_spectrum; }
    virtual float32_t setAttack_sec(float32_t att) { 
      if (att >= 0.0001f) attack_sec = att; 
      attack_coeff = calcCoeffGivenTimeConstant(attack_sec, getOverlappedFFTRate_Hz());     //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return attack_sec; 
    }
    virtual float32_t getAttack_sec(void) const { return attack_sec; }
    virtual float32_t setRelease_sec(float32_t rel) { 
      if (rel >= 0.0001f) release_sec = rel; 
      release_coeff = calcCoeffGivenTimeConstant(release_sec, getOverlappedFFTRate_Hz()); //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return release_sec; 
    }
    virtual float32_t getRelease_sec(void) const { return release_sec; }
    virtual float32_t setMaxAttenuation_dB(float32_t atten_dB) { 
      max_gain = sqrtf(powf(10.0f, 0.1f*(-atten_dB))); //linear value, amplitude not power
      return getMaxAttenuation_dB();
    }
    virtual float32_t getMaxAttenuation_dB(void) { return -10.0f*log10f(max_gain*max_gain); }
    virtual float32_t setSNRforMaxAttenuation_dB(float32_t val_dB) { 
      SNR_for_max_atten = powf(10.0f, 0.1f*val_dB); //linear not dB, but it is power (ie, signal^2)
      return getSNRforMaxAttenuation_dB();
      };
    virtual float32_t getSNRforMaxAttenuation_dB(void) { return 10.0f*log10f(SNR_for_max_atten); };
    virtual float32_t setTransitionWidth_dB(float32_t val_dB) { 
      val_dB = max(1.0f, val_dB);
      transition_width = powf(10.0f, 0.1f*val_dB);  //linear not dB, but it is power (ie, signal^2)
      return getTransitionWidth_dB();
    };
    virtual float32_t getTransitionWidth_dB(void) const { return 10.0f*log10f(transition_width); }
    virtual float32_t setGainSmoothing_octaves(float32_t val_oct) { return freq_smooth_octaves = max(0.0,val_oct); }
    virtual float32_t getGainSmoothing_octaves(void) const { return freq_smooth_octaves; }
    virtual float32_t setGainSmoothing_sec(float32_t val_sec) {
      if (val_sec >= 0.001f) smooth_sec = val_sec;
      smooth_coeff = calcCoeffGivenTimeConstant(smooth_sec, getOverlappedFFTRate_Hz()); //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32;
      return smooth_sec;
    }
    virtual float32_t getGainSmoothing_sec(void) const { return smooth_sec; }
    virtual float32_t calcCoeffGivenTimeConstant(float32_t time_const_sec, float32_t block_rate_Hz) {
      //http://www.tsdconseil.fr/tutos/tuto-iir1-en.pdf
      //normally, this is computed using the sample rate.  We however will be applying the filter
      //only once every time the FFT is computed, so we substitute the FFT rate for the sample rate 
      float32_t val =  1.0f - expf(-1.0f / (time_const_sec * block_rate_Hz));
      return max(0.0f,min(1.0f, val));
    }
    virtual bool setEnableNoiseEstimationUpdates(bool true_is_update) { return enableNoiseEstimationUpdates = true_is_update; }
    virtual bool getEnableNoiseEstimationUpdates(void) const { return enableNoiseEstimationUpdates; }
   
    virtual void resetAveSpectrumAndGains(void) { for (int ind=0; ind < ave_spectrum_N; ind++) { ave_spectrum[ind]=0.0f; gains[ind] = 1.0f; prev_gains[ind]=1.0; } };
    virtual void updateAveSpectrum(float32_t *current_pow);
    virtual void calcGainsBasedOnSpectrum(float32_t *current_pow);
    virtual void smoothGainsInTime(void);
    virtual void smoothGainsInFrequency(void);

    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    void processAudioFD(float32_t *complex_2N_buffer, const int NFFT) override; 

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

#endif
