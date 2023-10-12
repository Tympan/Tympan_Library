
/*
 * AudioEffectFreqShift_FD_F32
 * 
 * CREATED: Chip Audette, Aug 2019
 * PURPOSE: Shift the frequency content of the audio up or down.  Performed in the frequency domain
 *     
 * This processes a single stream of audio data (ie, it is mono)   
 *
 * Note that this is a *frequency* shifter.  For example, if you ask it to shift the audio by 250 Hz,
 *     It will shift all of the frequency content by 250 Hz.  This is straight-forward and easy.  But,
 *     as a human being listening to the audio, it will sound strange because adding a fixed number
 *     Hz will break all of the harmonic relationships within the audio.  Breaking the harmonic
 *     relationships is a big deal to a human listener.  
 *
 *     For example, a tone at 400 Hz, might have its harmonics at 2x, 3x, 4x...so, 800 Hz, 1200 Hz,
 *     and 1600 Hz.  This sounds natural and right.  But, after using this frequency shifter to add
 *     250 Hz, the tones are now at [400, 800, 1200, 1600] Hz + 250 Hz = [650, 1050, 1650, 1850] Hz.
 *     No longer are those upper frequencies integer multiples of the fundamental; they are no longer
 *     harmonics.  It will sound strage.
 *
 *     For human listening, what you might prefer is a Pitch Shifter.  A pitch shifter *multiplies*
 *     all of the frequencies by a user-requested scale factor.  By multiplying the frequency values,
 *     it maintains the harmonic relationships and can sound more natural.  Be aware, however, that
 *     pitch shifting is much more difficult from a signal processing perspective; so it will have
 *     its own audio processing artifacts that might be objectionable.
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