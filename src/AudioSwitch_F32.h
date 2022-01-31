/*
 * AudioSwitch
 * 
 * AudioSwitch4
 * Created: Chip Audette, OpenAudio, April 2019
 * Purpose: Switch one input to 4 outputs, which should only trigger one of the 4
 *      audio processing paths (therebys saving computation on paths that you don't
 *      care about).
 * Assumes floating-point data.
 *          
 * This processes a single stream of audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef AUDIOSWITCH_F32_H
#define AUDIOSWITCH_F32_H

#include "AudioStream_F32.h"

class AudioSwitch4_F32 : public AudioStream_F32 {
//GUI: inputs:1, outputs:4  //this line used for automatic generation of GUI node
//GUI: shortName:Switch4
public:
    AudioSwitch4_F32() : AudioStream_F32(1, inputQueueArray) { setDefaultValues(); }
	AudioSwitch4_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray) { setDefaultValues(); }
	
	void setDefaultValues(void) {
		outputChannel = 0;
	}
	
    virtual void update(void);

    int setChannel(unsigned int channel) {
      if (channel >= 4 || channel < 0) return outputChannel;  //invalid!  stick with previous channel
      return outputChannel = channel;
    }

  private:
    audio_block_f32_t *inputQueueArray[1];
    int outputChannel;
};


#endif