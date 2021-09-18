/*
 * AudioEffectGain_F32
 * 
 * Created: Chip Audette, November 2016
 * Purpose: Apply digital gain to the audio data.  Assumes floating-point data.
 *          
 * This processes a single stream fo audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectGain_F32_h
#define _AudioEffectGain_F32_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include "AudioStream_F32.h"

class AudioEffectGain_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node  
  public:
    //constructor
    AudioEffectGain_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
	AudioEffectGain_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {};

    //here's the method that does all the work
    virtual void update(void) {
		//Serial.println("AudioEffectGain_F32: updating.");  //for debugging.
		
		//get input block
		audio_block_f32_t *block;
		block = AudioStream_F32::receiveWritable_f32();
		if (block == NULL) return;

		//apply the gain
		//for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) block->data[i] = gain * (block->data[i]); //non DSP way to do it
		arm_scale_f32(block->data, gain, block->data, block->length); //use ARM DSP for speed!

		//transmit the block and be done
		AudioStream_F32::transmit(out_block);
		AudioStream_F32::release(out_block);
		AudioStream_F32::release(block);
    }

    //methods to set parameters of this module
    virtual float setGain(float g) { return gain = g;}
    virtual float setGain_dB(float gain_dB) {
      float gain = pow(10.0, gain_dB / 20.0);
      setGain(gain);
	  return getGain_dB();
    }

	//increment the linear gain
    virtual float incrementGain_dB(float increment_dB) {
      return setGain_dB(getGain_dB() + increment_dB);
    }    
	
    virtual void setSampleRate_Hz(const float _fs_Hz) {};  //unused.  included for interface compatability with fancier gain algorithms
	virtual float getCurrentLevel_dB(void) { return 0.0; };  //meaningless.  included for interface compatibility with fancier gain algorithms
	
    //methods to return information about this module
    virtual float getGain(void) { return gain; }
    virtual float getGain_dB(void) { return 20.0*log10(gain); }
    
  protected:
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    float gain = 1.0; //default value
};


/* class AudioEffectGain_F32_UI : public AudioEffectGain_F32 {
	//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node  
  public:
    //constructor
    AudioEffectGain_F32_UI(void) : AudioEffectGain_F32() {};
	AudioEffectGain_F32_UI(const AudioSettings_F32 &settings) : AudioEffectGain_F32(settings) {};
}; */

#endif