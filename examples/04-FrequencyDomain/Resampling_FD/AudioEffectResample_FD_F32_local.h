
#ifndef _AudioEffectResample_FD_F32_local_h
#define _AudioEffectResample_FD_F32_local_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

// THIS IS AN EXAMPLE OF HOW TO CREATE YOUR OWN FREQUENCY-DOMAIN ALGRITHMS
// TRY TO DO YOUR OWN THING HERE! HAVE FUN!

// You can use this class to change the sample rate of your audio stream.  It can be used to
// down sample (NFFT_out < N_FFT_in) or it can be used to up sample (NFFT_out > NFFT_in).
// You should keep the ratio of NFFT_out / NFFT_in to be a power of 2 so that the FFT operations
// are standard.
//
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT/IFFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.
class AudioEffectResample_FD_F32_local : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioEffectResample_FD_F32_local(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};

    // ///////////////////////////// setup
    // Call setup() to set the number of FFT bins (and sample rate and block size)
    // we don't need to define a new one because the one in the parent class is
    // fine for this example (and probably for most use cases!)
    // int setup(const AudioSettings_F32 &settings, const int _N_FFT_IN, const int _N_FFT_OUT) override;  //override if you need to do something complicated

    // ///////////////////////////// update
    // Normally, when making your own algorithm, you'd override update() and put your own
    // processing into your own new update().  But, for these frequency-domain algorithms
    // it's probably best to let your class rely upon the update() method that's already
    // defined in the parent (AudioFreqDomainBase_FD_F32).  This parent update() converts
    // the audio in and out of the frequency domain for you.  Instead of defining your
    // own update(), you should simply override the method processAudioFD(), as discussed below.
    //void update() override;  //override if you need to do something complicated

    // ///////////////////////////// process
    // This is the method from AudioFreqDomainBase that we are overriding where we will
    // put our own code for manipulating the frequency data.  This is called by update()
    // from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    // Tympan (Teensy) audio system, as with every other Audio processing class.
    //
    // In this case, however, there is no additional work to do, beyond what happens
    // normally in the parent class.  You could add processig here if you wanted
    // to, though.
    void processAudioFD(float32_t *complex_2N_buffer) override {
      // in our case, there's nothing to do

      //Serial.println("AudioEffectResample_FD_F32_local: processAudioFD: here!");
    };   

  private:
 
};

#endif
