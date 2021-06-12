
#ifndef _AudioEffectLowpass_FD_F32_h
#define _AudioEffectLowpass_FD_F32_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

// THIS IS AN EXAMPLE OF HOW TO CREATE YOUR OWN FREQUENCY-DOMAIN ALGRITHMS
// TRY TO DO YOUR OWN THING HERE! HAVE FUN!

//Create an audio processing class to do the lowpass filtering in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT/IFFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.
class AudioEffectLowpass_FD_F32 : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioEffectLowpass_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};
        
    // get/set methods specific to this particular frequency-domain algorithm
    float setCutoff_Hz(const float freq_Hz) { return cutoff_Hz = freq_Hz;  }
    float getCutoff_Hz(void) {   return cutoff_Hz; }
    float setHighFreqGain_dB(const float gain_dB) {   gain = sqrtf(powf(10.0f, 0.1*gain_dB));  return getHighFreqGain_dB();    }
    float getHighFreqGain_dB(void) {  return 10.0f*log10f(gain*gain);  }

    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 

  private:
    //create some data members specific to our processing
    float cutoff_Hz = 1000.f; //default cutoff frequency
    float gain = 0.1;               //default attenuation amount
   
};


//Here is the method we are overriding with our own algorithm...REPLACE THIS WITH WHATEVER YOU WANT!!!!
//  Argument 1: complex_2N_buffer is the float32_t array that holds the FFT results that we are going to
//     manipulate.  It is 2*NFFT in length because it contains the real and imaginary data values
//     for each FFT bin.  Real and imaginary are interleaved.  We only need to worry about the bins
//     up to Nyquist because AudioFreqDomainBase will reconstruct the freuqency bins above Nyquist for us.
//  Argument 2: NFFT, the number of FFT bins
//
//  We get our data from complex_2N_buffer and we put our results back into complex_2N_buffer
void AudioEffectLowpass_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {

  int N_2 = NFFT / 2 + 1;
  float Hz_per_bin = sample_rate_Hz /((float)NFFT); //sample_rate_Hz is from the base class AudioFreqDomainBase_FD_F32
  /*
  //In some other example, this might be a useful operation...getting the magnitude of the bin.
  //here's a computationally efficient way to do it...call an optimized library
  float orig_mag[N_2];
  arm_cmplx_mag_f32(complex_2N_buffer, orig_mag, N_2);  //get the magnitude for each FFT bin and store somewhere safes
  */

  //Loop over each bin and attenuate those above the cutoff
  int cutoff_bin = (int)(cutoff_Hz / Hz_per_bin + 0.5f); //the 0.5 is so that it rounds instead of truncates
  for (int ind = 0; ind < N_2; ind++) { //only process up to Nyquist...the class will automatically rebuild the frequencies above Nyquist
    if (ind >= cutoff_bin) {  //are we at or above the cutoff frequency?
      //attenuate both the real and imaginary comoponents
      complex_2N_buffer[2 * ind]     = gain * complex_2N_buffer[2 * ind];     //real
      complex_2N_buffer[2 * ind + 1] = gain * complex_2N_buffer[2 * ind + 1]; //imaginary
    } else {
      //do not change the audio
    }    
  }  
}

#endif
