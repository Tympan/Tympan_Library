/* AudioEffectFade_F32
 *  
 *  Created: Chip Audette, Jan 2023
 *  Purpose: This module smoothly fades in or fades out (linear ramp)
 *  Inspired by the "AudioEffectFade" block from the Teensy Audio Library
 *  
 *  This processes a single stream of audio data (ie, it is mono)       
 *          
 *  MIT License.  use at your own risk.
 */

 #ifndef _AudioEffectFade_F32_h
 #define _AudioEffectFade_F32_h

 #include "AudioStream_F32.h"

class AudioEffectFade_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName:fade
  public:
    AudioEffectFade_F32(void) : AudioStream_F32(1, inputQueueArray_f32),
      sample_rate_Hz(AUDIO_SAMPLE_RATE) { 
        //setDefaultValues(); 
        };
      
    AudioEffectFade_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32),
      sample_rate_Hz(settings.sample_rate_Hz) { 
        //setDefaultValues();
        };

    void fadeIn_msec(float msec) {
      if (msec < 0.001) {
        //too small, just snap in
        cur_amp = 1.0;
        is_active = false;
      } else {
        step_size_amp = (1.0 / sample_rate_Hz) / (msec/1000.0f);
        cur_amp = 0.0;
        is_active = true;
      }
    }
    void fadeOut_msec(float msec) {
      if (msec < 0.001) {
        //too small, just snap out
        cur_amp = 0.0;
        is_active = false;
      } else {
        step_size_amp = -(1.0 / sample_rate_Hz) / (msec/1000.0f);
        cur_amp = 1.0;
        is_active = true;   
      }  
    }
		float setLevel(float val) { is_active = false;  return cur_amp = max(0.f, min(1.f, val)); }

    //here's the method that does all the work
    virtual void update(void);
    virtual int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block);

    float setSampleRate_Hz(const float &fs_Hz) {
      //change params that follow sample rate
      return sample_rate_Hz = fs_Hz;
    }
    float getSampleRate_Hz(void) { return sample_rate_Hz; }
    float getCurrentLevel(void) { return cur_amp; }

    bool isActive(void) { return is_active; }
    
  private:
  
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    float32_t sample_rate_Hz;
    float cur_amp = 1.0;
    float step_size_amp = 0.0;
    bool is_active = false;
    
 
};
 #endif