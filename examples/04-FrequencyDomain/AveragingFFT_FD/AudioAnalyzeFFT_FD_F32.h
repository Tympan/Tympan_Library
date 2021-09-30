
#ifndef _AudioAnalyzeFFT_FD_F32_h
#define _AudioAnalyzeFFT_FD_F32_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

//Create an audio processing class to do the audio analysis in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into the frequency
// domain.
class AudioAnalyzeFFT_FD_F32 : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioAnalyzeFFT_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};
    ~AudioAnalyzeFFT_FD_F32(void) { delete ave_vals_pow; }
        
    //interface follows from Teensy's fft256 class
    bool available(void) { return fft_is_available; };
    float read(int binNumber) { return getPow(binNumber); }
    float read(int firstBin, int lastBin) { return getPow(firstBin,lastBin); }
    //void averageTogether(int number); 
    //void windowFunction(window);

    //other methods
    float getPow(int binNumber) { return getPow(binNumber,binNumber); }
    float getPow(int firstBin, int lastBin) {
      if (ave_vals_pow == NULL) return -1.0;
      if (lastBin < firstBin) { int foo = lastBin; lastBin=firstBin; firstBin=foo; }
      if (firstBin < 0) return -1.0;
      if (lastBin > NFFT/2) return -1.0;
      float ret_val = 0.0;
      for (int i=firstBin; i<=lastBin; i++) ret_val += ave_vals_pow[i];
      return ret_val;
    }
    void resetAveraging(void) {
      if (ave_vals_pow == NULL) return;
      for (int i=0; i<= NFFT/2; i++) ave_vals_pow[i]=0.0;
    }

    //lets override the setup function so that we can allocate the buffers
    //for the averaging
    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      //first, call the parent setup
      AudioFreqDomainBase_FD_F32::setup(settings,_N_FFT);

      //now allocate our local stuff
      if (ave_vals_pow != NULL) delete ave_vals_pow;
      ave_vals_pow = new float[N_FFT/2+1];

      //initialize 
      resetAveraging();
    }
    
    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 

  private:
    //create some data members specific to our processing
    bool fft_is_available = false;
    float *ave_vals_pow;
   
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

  //In some other example, this might be a useful operation...getting the magnitude and phase of the bins.
  //here's a computationally efficient way to do it...call an optimized library for the mangitude
  float32_t orig_mag[N_2];
  arm_cmplx_mag_f32(complex_2N_buffer, orig_mag, N_2);  //get the magnitude for each FFT bin and store somewhere safes

  //Loop over each bin and lowpass filter the magnitude estimates
  
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
