

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
        AudioStream_F32(1, inputQueueArray_f32) {  
			sample_rate_Hz = settings.sample_rate_Hz;
			audio_block_samples = settings.audio_block_samples;
	}
    AudioFreqDomainBase_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) : 
        AudioStream_F32(1, inputQueueArray_f32) {
			setup(settings,_N_FFT);   //this also sets sample_rate_Hz and audio_block_samples
	}

    //destructor...release all of the memory that has been allocated
    ~AudioFreqDomainBase_FD_F32(void) { if (complex_2N_buffer != NULL) delete complex_2N_buffer; }
     
    virtual int setup(const AudioSettings_F32 &settings, const int _N_FFT);
    virtual void update(void);   
	bool enable(bool state = true) { enabled = state; return enabled;}

    //Here is the method for you to override with your own algorithm!
    //  * The first argument that you will receive is the float32_t *, which is an array that is allocated in
    //      the setup() method.  It is 2*NFFT in length because it contains the real and imaginary data values
    //      for each bin.  Real and imaginary are interleaved
    //  * The second argument is the number of fft bins
    //Note that you only need to touch the bins associated with zero through Nyquist.  The update() method
    //  above will reconstruct the bins above Nyquist for you.  It does this by taking the complex conjugate
    //  of the bins below Nyquist.  Easy for you!
    virtual void processAudioFD(float32_t *complex_data, const int nfft);   //definitely override this in your own algorithm!

	virtual int getNFFT(void) { return myFFT.getNFFT();}
	virtual int getBlockLength_samples(void) { return audio_block_samples; }
	virtual float getSampleRate_Hz(void) { return sample_rate_Hz; }
	
	//the rate at which overlapped FFTs are computed is the same (per how we set up these FFT
	//routines here for Tympan) as the rate at which new audio blocks arrive.  Since the FFTs
	// are overlapping, we are computing an FFT after every audio_block_samples, which is 
	//faster than computing an FFT after every NFFT
	virtual float getOverlappedFFTRate_Hz(void) { return (sample_rate_Hz/((float)audio_block_samples));}

  protected:
    int enabled=0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
	  int audio_block_samples = 128;
		int N_FFT;
    
};


#endif