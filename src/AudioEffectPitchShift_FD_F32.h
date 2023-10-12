
/*
 * AudioEffectPitchShift_FD_F32
 * 
 * Created: Chip Audette, Sept 2023
 * Purpose: Shift the pitch of the audio up or down so that harmonic relationships
 *          are maintained correctly (unlike the AudioEffectPitchShift_FD_F32 module)
 *          
 * This processes a single stream of audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectPitchShift_FD_F32_h
#define _AudioEffectPitchhift_FD_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "FFT_Overlapped_F32.h"
#include "AudioFilterBiquad_F32.h"
#include <Arduino.h>
#include <deque>
#include <algorithm> //do we need this

class MultiAudioBlocks_F32 {
  public:
     
    std::deque<audio_block_f32_t *> audio_blocks;
    int len_written = 0; //number of values written to the last data block in the queue
    
    void reset(void) { release_all_and_clear(); }
    
    void release_all_and_clear(void) {  
      while (!audio_blocks.empty()) {
        if (audio_blocks.front() != NULL) AudioStream_F32::release(audio_blocks.front());
      }
      audio_blocks.clear();
      len_written = 0;
    }

    bool allocateAndAddBlock(void) {
      audio_block_f32_t *audio_block = AudioStream_F32::allocate_f32();
      if (audio_block == NULL) { return false; } // out of memory!
      audio_blocks.push_back(audio_block);
      len_written = 0;
      return true;
    }

    //convenience functions, copying the best methods for the underlying deque
    audio_block_f32_t * front(void) { return audio_blocks.front();}
    audio_block_f32_t * back(void) { return audio_blocks.back();   }
    void push_back(audio_block_f32_t *block) { audio_blocks.push_back(block); len_written = 0; }
    void pop_front(void){ audio_blocks.pop_front(); }
    unsigned int size(void) const { return audio_blocks.size(); }   
    void clear(void) { audio_blocks.clear(); }
    bool empty(void) const { return audio_blocks.empty(); }
    audio_block_f32_t * operator [](unsigned int i) {
      if (i < audio_blocks.size()) {
        return audio_blocks[i];
      } else {
        return NULL;
      }
    } 
};

class AudioEffectPitchShift_FD_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:pitch_shift
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectPitchShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectPitchShift_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { 
      resample_filter.setSampleRate_Hz(sample_rate_Hz);
    };
    AudioEffectPitchShift_FD_F32(const AudioSettings_F32 &settings) :
      AudioStream_F32(1, inputQueueArray_f32) {
      sample_rate_Hz = settings.sample_rate_Hz;
      audio_block_samples = settings.audio_block_samples; 
      resample_filter.setSampleRate_Hz(sample_rate_Hz);
    }
    AudioEffectPitchShift_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :
      AudioStream_F32(1, inputQueueArray_f32) {
      setup(settings, _N_FFT);
    }
    
    //destructor...release all of the memory that has been allocated
    ~AudioEffectPitchShift_FD_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
      if (cur_mag != NULL) delete cur_mag;
      if (prev_mag != NULL) delete prev_mag;
      if (cur_phase != NULL) delete cur_phase;
      if (prev_phase != NULL) delete prev_phase;
      if (cur_dPhase != NULL) delete cur_dPhase;
      if (prev_dPhase != NULL) delete prev_dPhase;
      if (new_mag != NULL) delete new_mag;
      if (prev_new_mag != NULL) delete prev_new_mag;
      if (shifted_phases != NULL) delete shifted_phases;
      resampled_audio_blocks.release_all_and_clear();
    }
    
    int setup(const AudioSettings_F32 &settings, const int _N_FFT);
    
    float setScaleFac(float _scale_fac) {
      scale_fac = _scale_fac;

      //change the cutoff of the pre/post filter!
      configureResampleFilterGivenScaleFac(scale_fac);    
      
      return getScaleFac();
    }
 
    virtual void update(void);
    bool enable(bool state = true) { enabled = state; return enabled;}

    void timeStretchAudio(audio_block_f32_t *in_audio_block, MultiAudioBlocks_F32 &out_audio_blocks);
    void synthesizeNewAudioBlock(float32_t frac, const int N_2, audio_block_f32_t *synth_audio_block);
    void configureResampleFilterGivenScaleFac(float scale_fac);
    void filterAudioBlock(audio_block_f32_t *audio_block);
    void resampleAudio(MultiAudioBlocks_F32 &stretched_audio_blocks, MultiAudioBlocks_F32 &resampled_audio_blocks);

    float getScaleFac(void) { return scale_fac; }
    float setScaleFac_semitones(float semitones) { setScaleFac(powf(2.0f,semitones/12.0f)); return getScaleFac_semitones(); }
    float getScaleFac_semitones(void) { return 12.0f * logf(getScaleFac())/ (logf(2.0f)); } //ideally, we'd use log2f, but it doesn't seem to be available
   
    int getNFFT(void) { return myFFT.getNFFT(); }
    FFT_Overlapped_F32* getFFTobj(void) { return &myFFT; }
    IFFT_Overlapped_F32* getIFFTobj(void) { return &myIFFT; }
    void resetState(void);

    float32_t transient_thresh = 0.5;
  private:
    int enabled = 0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
    int audio_block_samples = AUDIO_BLOCK_SAMPLES;
    int N_FFT = -1;
    enum OVERLAP_OPTIONS {NONE, HALF, THREE_QUARTERS};  //evenutally extend to THREE_QUARTERS
    int overlap_amount = NONE;
    int overlap_block_counter = 0;
    float32_t *cur_mag, *prev_mag;
    float32_t *cur_phase, *prev_phase;
    float32_t *cur_dPhase, *prev_dPhase; 
    float32_t *new_mag, *prev_new_mag; 
    float32_t *shifted_phases;
    float scale_fac = 1.0; //how much to scale the frequencies (1.0 is no scaling)
    int transient_count = 0;
    float32_t prev_last_sample_value = 0.0;
    float32_t ind_float = 0.0;  //index into synth_audio_block, 0.0 is the first sample, a negative value requires interpolation with the last value of the previous block
    float32_t targ_idx = 1.0; //initialize to 1.0 to cause it to not compute output the first time through
    MultiAudioBlocks_F32 resampled_audio_blocks;
    AudioFilterBiquad_F32 resample_filter;
};


#endif