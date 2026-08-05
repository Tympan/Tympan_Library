
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
    AudioEffectFreqShift_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { setInstanceName(); };
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings) :
      AudioStream_F32(1, inputQueueArray_f32) {
			setInstanceName();
      sample_rate_Hz = settings.sample_rate_Hz;
      sample_rate_out_Hz = sample_rate_Hz;
    }
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :
      AudioStream_F32(1, inputQueueArray_f32) {
			setInstanceName();
      setup(settings, _N_FFT);
    }
		void setInstanceName(void) { instanceName = "AudioEffectFreqShift_FD_F32"; }

    //destructor...release all of the memory that has been allocated
    virtual ~AudioEffectFreqShift_FD_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
    }

    virtual int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      return setup(settings, _N_FFT, _N_FFT);
    }
    virtual int setup(const AudioSettings_F32 &settings, const int _N_FFT, const int _N_IFFT);

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
		
		virtual void preprocessFreqDomainData(float32_t *complex_2N_buffer, int NFFT) { return; } //default to do nothing (child class can override!)
		virtual void shiftTheBins(float32_t *complex_2N_buffer, int NFFT, int n_shift);
		
		void update(void) override;

		//To save CPU when not using this algorithm, you can enable/disable the algorithm
		//using the method below.  Using this enable() method, the processing will be bypassed
		//but the input audio will be copied over to the output.
		//
		//In contrast, you could use "setActive(false)" universally part of AudioStream_F32
		//which will prevent the processing from being called.  In the case of setActive(),
		//however, the algorithm won't even pass the input to the output; it'll have no output,
		//which will naturally stop all processing by subsequent audio processing classes, too.
		//
		//So, choose which behavior you want and enjoy!
		bool enable(bool state = true) { enabled = state; return enabled;}
		
		FFT_Overlapped_F32* getFFTobj(void) { return &myFFT; }
		IFFT_Overlapped_F32* getIFFTobj(void) { return &myIFFT; }

		bool flag_printDebug = false;

  protected:
    int enabled = 0;
    float32_t *complex_2N_buffer = nullptr;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE; //incoming data
    float sample_rate_out_Hz = AUDIO_SAMPLE_RATE;  //outgoing data
    int audio_block_out_samples = 128;  //default, gets overwritten in setup
    int N_FFT = -1;
    int N_IFFT = -1;
		enum OVERLAP_OPTIONS {NONE, HALF, THREE_QUARTERS};  //evenutally extend to THREE_QUARTERS
		int overlap_amount = NONE;
		int overlap_block_counter = 0;
		
    int shift_bins = 0; //how much to shift the frequency
};


#endif