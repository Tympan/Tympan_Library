
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
 * RESAMPLING: As of Aug 2026, this class can also be used to change the sample rate of th audio
 *     as part of the frequency shifting.  If upshifting the frequencies, you can have this class
 *     increase the sample rate to provide space for your upshifted signal.  Or, if downshifting
 *     the frequencies, you can have this class cut the sample rate to save CPU for subsequent
 *     processing steps.  As this class operates in the frequency domain, and as the underlying 
 *     FFT/IFFT routines only handle sizes that are a power of 2, you can only use this class to
 *     change the sample rate by a factor of 2 (ie, 1, 2, 4, 8).  When NOT resampling, you use the
 *     begin() method to tell this class to use the same size for the FFT and for the IFFT.  When
 *     you do want to resample, you use the begin() method to tell it to use one size for the FFT
 *     and another size for the IFFT.  A smaller IFFT size results in downsampling; a larger IFFT
 *     size will result in upsampling.
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectFreqShift_FD_F32_h
#define _AudioEffectFreqShift_FD_F32_h

#include "AudioStream_F32.h"
#include "AudioFreqDomainBase_FD_F32.h"
//include "FFT_Overlapped_F32.h"
#include <Arduino.h>


class AudioEffectFreqShift_FD_F32 : public AudioFreqDomainBase_FD_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:freq_shift
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectFreqShift_FD_F32(void) : AudioFreqDomainBase_FD_F32() { setInstanceName(); };
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings) :  AudioFreqDomainBase_FD_F32(settings)  {	setInstanceName(); }
    AudioEffectFreqShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :  AudioFreqDomainBase_FD_F32(settings, _N_FFT)  {	setInstanceName();  }
		void setInstanceName(void) { instanceName = "AudioEffectFreqShift_FD_F32"; }

    //destructor...release all of the memory that has been allocated
    virtual ~AudioEffectFreqShift_FD_F32(void) { } //nothing in addition to parent's destructor

    int setup(const AudioSettings_F32 &settings, const int _N_FFT) override { return setup(settings, _N_FFT, _N_FFT); }
    int setup(const AudioSettings_F32 &settings, const int _N_FFT, const int _N_IFFT) override;

		//void update(void) override;  // we'll use the one from the parent class
    void processAudioFD(float32_t *complex_data) override;  //this is where we put all the processing.  It'll get called by the parent's update() 

    // set and get methods for parameters specific to the frequency-shifting processing
    int setShift_bins(const int _shift_bins)     { return shift_bins = _shift_bins; }
    int getShift_bins(void) const                { return shift_bins; }
		float getShift_Hz(void) const                { return getFrequencyOfBin(shift_bins);	}
		float getFrequencyOfBin(const int bin) const { return sample_rate_input_Hz * ((float)bin) / ((float) N_FFT); } //"bin" should be zero to (N_FFT-1)
		
   
  protected:
		enum OVERLAP_OPTIONS {NONE, HALF, THREE_QUARTERS};  //evenutally extend to other overlap factors
		int overlap_amount = NONE;
		int overlap_block_counter = 0;
		
    int shift_bins = 0; //how much to shift the frequency

    virtual void preprocessFreqDomainData(float32_t *complex_2N_buffer, const int NFFT) { return; } //default to do nothing (child class can override!)
		virtual void shiftTheBins(float32_t *complex_2N_buffer, const int NFFT_input, const int NFFT_output, const int n_shift);
    virtual void adjustBinPhases(float32_t *complex_2N_buffer, const int N_2);
		
};


#endif