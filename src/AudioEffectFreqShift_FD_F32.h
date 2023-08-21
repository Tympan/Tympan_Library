
/*
 * AudioEffectFreqShift_FD_F32
 * 
 * Created: Chip Audette, Aug 2019
 * Purpose: Shift the frequency content of the audio up or down.  Performed in the frequency domain
 *          
 * This processes a single stream of audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectFreqShift_FD_F32_h
#define _AudioEffectFreqShift_FD_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "FFT_Overlapped_F32.h"
#include <Arduino.h>


class AudioEffectFreqShift_FD_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:freq_shift
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectFreqShift_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings) :
      AudioStream_F32(1, inputQueueArray_f32) {
      sample_rate_Hz = settings.sample_rate_Hz;
    }
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :
      AudioStream_F32(1, inputQueueArray_f32) {
      setup(settings, _N_FFT);
    }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectFreqShift_FD_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
    }

    int setup(const AudioSettings_F32 &settings, const int _N_FFT);

    int setShift_bins(int _shift_bins) {
      return shift_bins = _shift_bins;
    }
    int getShift_bins(void) {
      return shift_bins;
    }
	float getShift_Hz(void) {
		return getFrequencyOfBin(shift_bins);
	}
	float getFrequencyOfBin(int bin) { //"bin" should be zero to (N_FFT-1)
		return sample_rate_Hz * ((float)bin) / ((float) N_FFT);
	}
	
    virtual void update(void);
	bool enable(bool state = true) { enabled = state; return enabled;}
	FFT_Overlapped_F32* getFFTobj(void) { return &myFFT; }
	IFFT_Overlapped_F32* getIFFTobj(void) { return &myIFFT; }

  private:
    int enabled = 0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
	int N_FFT = -1;
	enum OVERLAP_OPTIONS {NONE, HALF, THREE_QUARTERS};  //evenutally extend to THREE_QUARTERS
	int overlap_amount = NONE;
	int overlap_block_counter = 0;
	
    int shift_bins = 0; //how much to shift the frequency
};


#endif