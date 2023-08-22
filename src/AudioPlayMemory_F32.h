
#ifndef _AudioPlayMemory_F32_h
#define _AudioPlayMemory_F32_h

#include <Arduino.h>
#include <AudioSettings_F32.h>
#include <AudioStream_F32.h>


/*
 * AudioPlayMemory16_F32
 * 
 * Purpose: Play F32 audio from RAM memory.  Values are saved in memory as Int16 data type
 * 
 * Created: Chip Audette, OpenAudio, Aug 2023
 * 
 * Notes: This player will upsample or downsample the audio to handle different sample rates.
 *        But, as of writing this comment, it does not interpolate or filter; it only does 
 *        a very-simple zero-order-hold (ie, stair-step) based on the closest sample.  You
 *        might not like the audio artifacts that result.   Feel free to improve it!
 */
class AudioPlayMemoryI16_F32 : public AudioStream_F32
{
//GUI: inputs:0, outputs:1  //this line used for automatic generation of GUI nodes 
  public:
    AudioPlayMemoryI16_F32(void) : AudioStream_F32(0, NULL) { }
    AudioPlayMemoryI16_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL)
    { 
      setSampleRate_Hz(settings.sample_rate_Hz);  setDataSampleRate_Hz(settings.sample_rate_Hz); 
    }
    virtual bool play(const int16_t *data, const uint32_t data_len);
    virtual bool play(const int16_t *data, const uint32_t data_len, const float32_t data_fs_Hz) {
      setDataSampleRate_Hz(data_fs_Hz);
      return play(data, data_len);
    }
    virtual void stop(void) { state = STOPPED; }
    virtual bool isPlaying(void) { if (state == PLAYING) { return true; } else { return false; } }
    virtual void update(void);
    float setSampleRate_Hz(float fs_Hz) { 
      sample_rate_Hz = fs_Hz; 
      data_phase_incr = data_sample_rate_Hz / sample_rate_Hz;
      return sample_rate_Hz;
    }
    float setDataSampleRate_Hz(float data_fs_Hz) {
      data_sample_rate_Hz = data_fs_Hz;
      data_phase_incr = data_sample_rate_Hz / sample_rate_Hz;
      //Serial.println("AudioPlayMemoryI16_F32: data_fs_Hz = " + String(data_fs_Hz) + ", data_phase_incr = " + String(data_phase_incr));
      return data_sample_rate_Hz;
    }

    enum STATE {STOPPED=0, PLAYING};
    private:
      int state = STOPPED;
      const int16_t *data_ptr = NULL;
      uint32_t data_ind = 0;
      float32_t data_phase = 0.0f;
      float32_t data_phase_incr = 1.0f;
      uint32_t data_len = 0;
      float32_t data_sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
      float sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
      //int audio_block_samples = AUDIO_BLOCK_SAMPLES;
};


#endif