
#ifndef _AudioTestToneManager_F32_h
#define _AudioTestToneManager_F32_h

#include <vector>
#include <arm_math.h>
#include "AudioStream_F32.h"

//create a data structure used just within this class
class AudioTestTone_Params {
  public:
    AudioTestTone_Params(const float32_t f_Hz, const float32_t a, const float32_t d_sec) : freq_Hz(f_Hz), amp(a), dur_sec(d_sec) {};
    float32_t freq_Hz;
    float32_t amp;
    float32_t dur_sec;
};

class AudioTestToneManager_F32 : public AudioStream_F32 {
  //GUI: inputs:0, outputs:1  //this line used for automatic generation of GUI node
  public:
    AudioTestToneManager_F32(void) : AudioStream_F32(0, NULL) {
      setAudioBlockSamples(AUDIO_BLOCK_SAMPLES);
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
    }
    AudioTestToneManager_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL) {
      setAudioBlockSamples(settings.audio_block_samples);
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    virtual ~AudioTestToneManager_F32(void) {}

    enum STATE {STATE_STOPPED = 0, STATE_TONE = 1, STATE_SILENCE = 2};

    virtual int setAudioBlockSamples(const int _audio_block_samples) {
      return audio_block_samples = _audio_block_samples; 
    }
    virtual float setSampleRate_Hz(const float fs_Hz) {
      //pass this data on to its components that care
      if ((fs_Hz < 0.1) || (fs_Hz > 1.0E6)) return sample_rate_Hz; //no change if new sample rate is invalid
      tone_dphase_rad = 2.0*M_PI*curToneParams.freq_Hz / sample_rate_Hz;
      return sample_rate_Hz = fs_Hz;
    }

    virtual void addToneToQueue(float32_t freq_Hz, float32_t amp, uint32_t dur_sec) { allToneParams.push_back(AudioTestTone_Params(freq_Hz, amp, dur_sec)); }
    virtual void removeAllTonesFromQueue(void) { allToneParams.clear(); }
    
    virtual float setSilenceBetweenTones_sec(float dt_sec) { return silenceBetweenTones_sec = dt_sec; }
    virtual float getSilenceBetweenTones_sec(void) { return silenceBetweenTones_sec; }

    virtual void startTest(void) {
      curToneIndex = -1; //will get incremented to zero in startNextTone()
      startNextTone(); 
    }
    virtual void stopTest(void) {
      current_state = AudioTestToneManager_F32::STATE_STOPPED;
      remaining_samples_needed = 0;
      if (flag_printChangesToSerial) Serial.println("AudioTestToneManager_F32: test ended.");
    }

    virtual void startNextTone(void);
    virtual void startSilence(void);
    virtual int incrementState(void);

    void update(void) override;

    virtual bool isTestActive(void) { return (current_state != AudioTestToneManager_F32::STATE_STOPPED); }
    virtual bool isToneActive(void) { return (current_state == AudioTestToneManager_F32::STATE_TONE); }


    enum STEP_TYPE {STEP_LINEAR=0, STEP_EXP, STEP_DB};
    int createFreqTest_linearSteps(float32_t start_freq_Hz, float32_t end_freq_Hz, float32_t freq_step_Hz, float32_t amp, float32_t dur_sec) {
      return createFreqTest(start_freq_Hz, end_freq_Hz, freq_step_Hz, amp, dur_sec,STEP_LINEAR);
    }
    int createFreqTest_expSteps(float32_t start_freq_Hz, float32_t end_freq_Hz, float32_t freq_step_scale, float32_t amp, float32_t dur_sec) {
      return createFreqTest(start_freq_Hz, end_freq_Hz, freq_step_scale, amp, dur_sec,STEP_EXP);
    }
    int createFreqTest(float32_t start_freq_Hz, float32_t end_freq_Hz, float32_t step_amount, float32_t amp, float32_t dur_sec, int step_type);
		
    int createAmplitudeTest_linearSteps(float32_t start_amp, float32_t end_amp, float32_t amp_step, float32_t freq_Hz, float32_t dur_sec) {
      return createAmplitudeTest(start_amp, end_amp, amp_step, freq_Hz, dur_sec, STEP_LINEAR);
    }
    int createAmplitudeTest_dBSteps(float32_t start_amp, float32_t end_amp, float32_t amp_step_dB, float32_t freq_Hz, float32_t dur_sec) {
      return createAmplitudeTest(start_amp, end_amp, amp_step_dB, freq_Hz, dur_sec, STEP_DB);
    }
    int createAmplitudeTest(float32_t start_amp, float32_t end_amp, float32_t step_amount, float32_t freq_Hz, float32_t dur_sec, int step_type);

    //public data members
    std::vector<AudioTestTone_Params> allToneParams;
    int curToneIndex = 0;
    bool flag_printChangesToSerial = true;

  protected:
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
    int audio_block_samples = AUDIO_BLOCK_SAMPLES;
    uint32_t block_counter = 0;
    float32_t tone_phase_rad = 0.0;
    float32_t tone_dphase_rad = 0.0;
    int current_state = AudioTestToneManager_F32::STATE_STOPPED;
    AudioTestTone_Params curToneParams = AudioTestTone_Params(0.0,0.0,0.0);
    float32_t silenceBetweenTones_sec = 0.0;
    uint32_t remaining_samples_needed = 0;
};


#endif