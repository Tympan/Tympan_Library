

#ifndef _AudioFreqDomainBase_FD_F32_h
#define _AudioFreqDomainBase_FD_F32_h


#include "AudioStream_F32.h"
#include "FFT_Overlapped_F32.h"

// Here is a base class to ease your frequency-domain processing.  This helps do the buffering and FFT/IFFT conversions

class AudioFreqDomainBase_FD_F32 : public AudioStream_F32
{
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectLowpassFD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioFreqDomainBase_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioFreqDomainBase_FD_F32(const AudioSettings_F32 &settings) :
        AudioStream_F32(1, inputQueueArray_f32) {  sample_rate_Hz = settings.sample_rate_Hz;   }
    AudioFreqDomainBase_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) : 
        AudioStream_F32(1, inputQueueArray_f32) { setup(settings,_N_FFT);  }

    //destructor...release all of the memory that has been allocated
    ~AudioFreqDomainBase_FD_F32(void) { if (complex_2N_buffer != NULL) delete complex_2N_buffer; }
     
    virtual int setup(const AudioSettings_F32 &settings, const int _N_FFT);
    virtual void update(void);   

    //Here is the method for you to override with your own algorithm!
    //  * The first argument that you will receive is the float32_t *, which is an array that is allocated in
    //      the setup() method.  It is 2*NFFT in length because it contains the real and imaginary data values
    //      for each bin.  Real and imaginary are interleaved
    //  * The second argument is the number of fft bins
    //Note that you only need to touch the bins associated with zero through Nyquist.  The update() method
    //  above will reconstruct the bins above Nyquist for you.  It does this by taking the complex conjugate
    //  of the bins below Nyquist.  Easy for you!
    virtual void processAudioFD(float32_t *complex_data, const int nfft);   //definitely override this in your own algorithm!

  protected:
    int enabled=0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
    
};


#endif