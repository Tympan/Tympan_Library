
#ifndef _AudioEffectFormantShift_FD_F32_h
#define _AudioEffectFormantShift_FD_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "FFT_Overlapped_F32.h"

class AudioEffectFormantShift_FD_F32 : public AudioStream_F32
{
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectFormantShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectFormantShift_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectFormantShift_FD_F32(const AudioSettings_F32 &settings) :
      AudioStream_F32(1, inputQueueArray_f32) {
      sample_rate_Hz = settings.sample_rate_Hz;
    }
    AudioEffectFormantShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :
      AudioStream_F32(1, inputQueueArray_f32) {
      setup(settings, _N_FFT);
    }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectFormantShift_FD_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
    }

    int setup(const AudioSettings_F32 &settings, const int _N_FFT);

    float setScaleFactor(float scale_fac) {
      if (scale_fac < 0.00001) scale_fac = 0.00001;
      return shift_scale_fac = scale_fac;
    }
    float getScaleFactor(void) {
      return shift_scale_fac;
    }

    virtual void update(void);
	bool enable(bool state = true) { enabled = state; return enabled;}	

  private:
    int enabled = 0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
	int N_FFT;

    float shift_scale_fac = 1.0; //how much to shift formants (frequency multiplier).  1.0 is no shift
};


#endif