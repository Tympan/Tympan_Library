
#ifndef EarpieceMixer_F32_h
#define EarpieceMixer_F32_h

#include <AICShield.h>  //to get the EarpieceShield class
#include <AudioEffectDelay_F32.h>
#include <AudioMixer_F32.h>
#include <AudioSummer_F32.h>

class EarpieceMixerState {
  public:
    EarpieceMixerState(void) { }

    enum ANALOG_VS_PDM {INPUT_ANALOG=0, INPUT_PDM};
    int input_analogVsPDM = INPUT_PDM;

    enum ANALOG_INPUTS {INPUT_PCBMICS=0, INPUT_MICJACK_MIC, INPUT_LINEIN_SE, INPUT_MICJACK_LINEIN};
    int input_analog_config = INPUT_PCBMICS;
    float inputGain_dB = 15.0;  //gain on the analog inputs (ie, the AIC ignores this when in PDM mode, right?)
    
    enum FRONTREAR_CONFIG_TYPE {MIC_FRONT=0, MIC_REAR, MIC_BOTH_INPHASE, MIC_BOTH_INVERTED};
    int input_frontrear_config = MIC_FRONT;
    //int targetFrontDelay_samps = 0;  //in samples
    //int targetRearDelay_samps = 0;  //in samples
    int currentFrontDelay_samps = 0; //in samples
    int currentRearDelay_samps = 0; //in samples
    float rearMicGain_dB = 0.0f;

   //set input streo/mono configuration
    enum INPUTMIX {INPUTMIX_STEREO=0, INPUTMIX_MONO, INPUTMIX_MUTE, INPUTMIX_BOTHLEFT, INPUTMIX_BOTHRIGHT};
    int input_mixer_config = INPUTMIX_STEREO;
    int input_mixer_nonmute_config = INPUTMIX_STEREO;

};


//EarpieceMixerBase_F32: A class to mix the four microphone channels in the earpiece
//   shield, including delaying of one mic relative to the other
class EarpieceMixerBase_F32  : public AudioStream_F32 {
  public:
    //constructor
    EarpieceMixerBase_F32(const AudioSettings_F32 &settings) : 
      AudioStream_F32(4, inputQueueArray_f32) {
      config(settings);
    }

    void config(const AudioSettings_F32 &settings) {
      setSampleRate_Hz(settings.sample_rate_Hz);
      setDefaultFrontRear();
      setDefaultAnalogVsPDM();
      setDefaultLeftRight();
	  setDefaultDelay();
    }
    
    float setSampleRate_Hz(const float sampleRate_Hz) {
      for (int i=LEFT; i<=RIGHT; i++) { frontMicDelay[i].setSampleRate_Hz(sampleRate_Hz);  rearMicDelay[ i].setSampleRate_Hz(sampleRate_Hz); }
      return frontMicDelay[LEFT].getSampleRate_Hz();
    }
    
    void setDefaultFrontRear(void) {
      for (int i=LEFT; i<=RIGHT; i++) { frontRearMixer[i].gain(FRONT, 1.0); frontRearMixer[i].gain(REAR, 0.0); }
    }
    
    void setDefaultAnalogVsPDM(void) {
      for (int i=LEFT; i<=RIGHT; i++) analogVsDigitalSwitch[i].switchChannel(INPUT_PDM);
    }
    
    void setDefaultLeftRight(void) {
      leftRightMixer[LEFT].gain(LEFT,1.0);   leftRightMixer[LEFT].gain(RIGHT,0.0);
      leftRightMixer[RIGHT].gain(LEFT,0.0);  leftRightMixer[RIGHT].gain(RIGHT,1.0);
    }
	
	void setDefaultDelay(void) {  //this sets it to zero but also activates the delay channel...otherwise, the delay passes no data!
		for (int i=LEFT; i<=RIGHT; i++) { frontMicDelay[i].delay(0,0.0f); rearMicDelay[i].delay(0,0.0f); } //delay(channel, milliseconds)
	}

    //primary processing methods
    virtual void update(void);
    virtual void processData(audio_block_f32_t *audio_in[4], audio_block_f32_t *tmp[8], audio_block_f32_t *audio_out[2]);
    
    //Audio classes used here
    AudioEffectDelay_F32          frontMicDelay[2];         //delays front microphone (left and right)
    AudioEffectDelay_F32          rearMicDelay[2];          //delays rear microphone (left and right)
    AudioMixer4_F32               frontRearMixer[2];        //mixes front-back earpiece mics (left and right)
    AudioSummer4_F32              analogVsDigitalSwitch[2]; //switches between analog and PDM (a summer is cpu cheaper than a mixer, and we don't need to mix here)
    AudioMixer4_F32               leftRightMixer[2];        //mixers to control mono versus stereo (left out and right out)

    //convenience variables
    const int LEFT=0, RIGHT=1, BOTH_LR=-1;
    const int FRONT=0, REAR=1, BOTH_FR=-1;
    const int INPUT_ANALOG = EarpieceMixerState::INPUT_ANALOG, INPUT_PDM = EarpieceMixerState::INPUT_PDM;
    const int PDM_LEFT_FRONT = EarpieceShield::PDM_LEFT_FRONT;
    const int PDM_LEFT_REAR = EarpieceShield::PDM_LEFT_REAR;
    const int PDM_RIGHT_FRONT = EarpieceShield::PDM_RIGHT_FRONT;
    const int PDM_RIGHT_REAR = EarpieceShield::PDM_RIGHT_REAR;
  
  protected:
    audio_block_f32_t *inputQueueArray_f32[4]; //memory pointer for the input to this module

    //supporting methods
    bool allocateAndGetAudioBlocks(audio_block_f32_t *audio_in[4], audio_block_f32_t *tmp[8], audio_block_f32_t *audio_out[2]);
 
};

class EarpieceMixer_F32 : public EarpieceMixerBase_F32 {
  //GUI: inputs:4, outputs:2 //this line used for automatic generation of GUI node  
  public:
    EarpieceMixer_F32(const AudioSettings_F32 &settings) 
      : EarpieceMixerBase_F32(settings) {};  

    void setTympanAndShield(Tympan *_myTympan, EarpieceShield *_earpieceShield) {
      myTympan = _myTympan;
      earpieceShield = _earpieceShield;
    }

    //  ///////// Here are the configuration presets
    int configureFrontRearMixer(int val);
    int setInputAnalogVsPDM(int input);
    int setAnalogInputSource(int input_config);
    int configureLeftRightMixer(int val);

    // ////////  other functions for settings
    float setInputGain_dB(float gain_dB); 
	
	//set delays
    //int setTargetMicDelay_samps(const int front_rear, int samples);
    int setMicDelay_samps(const int front_rear,  int samples);
	
	//set gains
	float incrementRearMicGain_dB(float increment_dB) {
		return setRearMicGain_dB(state.rearMicGain_dB + increment_dB);
	}
	float setRearMicGain_dB(float gain_dB) {
	  state.rearMicGain_dB = gain_dB;
	  configureFrontRearMixer(state.input_frontrear_config);  //this puts the new gain value into action
	  return state.rearMicGain_dB;
	}

    //to do: add STATE accessing elements, if needed
    EarpieceMixerState state;  //just make public for now and let anyone access the state variables?

    
  private:
    Tympan *myTympan = NULL;
    EarpieceShield *earpieceShield = NULL;
};


#endif
