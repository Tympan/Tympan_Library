
#ifndef AudioPaths_h
#define AudioPaths_h

#include <vector>

class AudioPath_Base {
  public:
    AudioPath_Base(void) {}
    AudioPath_Base(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src, AudioStream_F32 *_dst) {
      if (_src != NULL) src = _src;
      if (_dst != NULL) dst = _dst;
    }
    
    virtual ~AudioPath_Base() {
      for (int i=patchCords.size()-1; i>=0; i--) delete patchCords[i]; //destroy all the Audio Connections (in reverse order...maybe less memroy fragmentation?)
      for (int i=audioObjects.size()-1; i>=0; i--) delete audioObjects[i];       //destroy all the audio class instances (in reverse order...maybe less memroy fragmentation?)
    }

    virtual int serviceMainLoop(void) { return 0; }

  protected:
    AudioStream_F32 *src=NULL, *dst=NULL;
    std::vector<AudioConnection_F32 *> patchCords;
    std::vector<AudioStream_F32 *> audioObjects;
};

class AudioPath_Sine : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_Sine(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src, AudioStream_F32 *_dst) : AudioPath_Base(_audio_settings, _src, _dst) 
    {
      //instantiate audio classes
      sineWave = new AudioSynthWaveform_F32( _audio_settings );
      audioObjects.push_back( sineWave );
      
      //make audio connections
      if (dst != NULL) {
        patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *dst, 0)); //route left input to left output
        patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *dst, 1)); //route right input to right output
      } else {
        Serial.println("AudioPath_Sine: ***ERROR***: destination (dst) is NULL.  Cannot create out-going AudioConnections.");
      }

      //setup the parameters of the audio processing
      setupAudioProcessing(); 
    }

    //~AudioPath_Sine();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords


    void setupAudioProcessing(void) {
      sineWave->frequency(1000.0f);
      sineWave->amplitude(sine_amplitude);
      tone_active = true;
      lastChange_millis = millis();
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+tone_dur_millis;
      if (tone_active == false) { targ_time_millis = lastChange_millis + silence_dur_millis; }

      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        if (tone_active) {
          //turn off the tone
          tone_active = false;
          Serial.print("AudioPath_Sine: serviceMainLoop: setting tone amplitude to "); Serial.println(0.0f,3);
          sineWave->amplitude(0.0f);
        } else {
          //turn on the tone
          tone_active = true;
          Serial.print("AudioPath_Sine: serviceMainLoop: setting tone amplitude to "); Serial.println(sine_amplitude,3);
          sineWave->amplitude(sine_amplitude);
        }
        lastChange_millis = cur_millis;
      } else {
        //not enough time has passed.  do nothing.
      }
      return 0;
    }
    
  protected:
    AudioSynthWaveform_F32  *sineWave;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    const float             sine_amplitude = 0.003f;
    unsigned long int       lastChange_millis = 0UL;
    const unsigned long int tone_dur_millis = 1000UL;
    const unsigned long int silence_dur_millis = 1000UL;
    bool                    tone_active = false;

};

class AudioPath_PassThruGain : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_PassThruGain(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src, AudioStream_F32 *_dst) : AudioPath_Base(_audio_settings, _src, _dst) 
    {
      //instantiate audio classes
      audioObjects.push_back( gainL       = new AudioEffectGain_F32( _audio_settings ) );
      audioObjects.push_back( gainR       = new AudioEffectGain_F32( _audio_settings ) );

      //make audio connections
      if (src != NULL) {
        patchCords.push_back( new AudioConnection_F32(*src,   0, *gainL,       0));  //left input to gainL
        patchCords.push_back( new AudioConnection_F32(*src,   1, *gainR,       0));  //right input to gainR
      } else {
        Serial.println("AudioPath_PassThruGain: ***ERROR***: source (src) is NULL.  Cannot create in-coming AudioConnections.");
      }
      if (dst != NULL) {
        patchCords.push_back( new AudioConnection_F32(*gainL, 0, *dst, 0));   //gainL to left output
        patchCords.push_back( new AudioConnection_F32(*gainR, 0, *dst, 1));   //gainR to right output
      } else {
        Serial.println("AudioPath_PassThruGain: ***ERROR***: destination (dst) is NULL.  Cannot create out-going AudioConnections.");
      }
      
      //setup the parameters of the audio processing
      setupAudioProcessing();
    }

    //~AudioPath_PassThruGain();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    void setupAudioProcessing(void) {
      gainL->setGain_dB(10.0);
      gainR->setGain_dB(10.0);
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+update_period_millis;
      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        Serial.println("AudioPath_PassThruGain: serviceMainLoop: Gain L (dB) = " + String(gainL->getGain_dB()) + ", Gain R (dB) = " + String(gainR->getGain_dB()));
        lastChange_millis = cur_millis;
      }
      return 0;
    }

    protected:
      //AudioInputI2S_F32   *audioInput;     //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
      AudioEffectGain_F32 *gainL;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted   
      AudioEffectGain_F32 *gainR;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted   
      //AudioOutputI2S_F32  *audioOutput;     //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
      unsigned long int       lastChange_millis = 0UL;
      const unsigned long int update_period_millis = 1000UL;
};

#endif

