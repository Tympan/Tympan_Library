/*
 * AudioSummer
 * 
 * AudioSummer4
 * Created: Chip Audette, OpenAudio, Sept 2019
 * Purpose: Sum up to 4 audio channels.  This is simpler than using
 *     the AudioMixer class (ie, there are no per-channel gains) which
 *     should cause reduced CPU load.
 *
 * Operates on floating-point data.
 *          
 * MIT License.  use at your own risk.
*/

#ifndef AUDIOSUMMER_F32_H
#define AUDIOSUMMER_F32_H

#include <arm_math.h> 
#include <AudioStream_F32.h>

class AudioSummer4_F32 : public AudioStream_F32 {
//GUI: inputs:4, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:Summer4
public:
    AudioSummer4_F32() : AudioStream_F32(4, inputQueueArray) { setDefaultValues(); }
	AudioSummer4_F32(const AudioSettings_F32 &settings) : AudioStream_F32(4, inputQueueArray) { setDefaultValues(); }
	
	void setDefaultValues(void) {
		for (int i=0; i<4; i++) flag_useChan[i] = true;
	}
	
    virtual void update(void);

    int enableChannel(unsigned int channel, bool enable = true) {
      if ((channel >= 4) || (channel < 0)) return -1;
      return (int) (flag_useChan[channel] = enable);
    }
	void mute(void) { for (int i=0; i < 4; i++) enableChannel(i,false); };  //mute all channels
	
	int switchChannel(unsigned int channel) { 
		//mute all channels except the given one.  Set the given one to 1.0.
		if ((channel >= 4) || (channel < 0)) return -1;
		mute(); 
		enableChannel(channel);
		return channel;
	} 

  private:
    audio_block_f32_t *inputQueueArray[4];
    bool flag_useChan[4];
};

class AudioSummer8_F32 : public AudioStream_F32 {
//GUI: inputs:8, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:Summer4
public:
    AudioSummer8_F32() : AudioStream_F32(8, inputQueueArray) { setDefaultValues(); }
	AudioSummer8_F32(const AudioSettings_F32 &settings) : AudioStream_F32(8, inputQueueArray) { setDefaultValues(); }
	
	void setDefaultValues(void) {
		for (int i=0; i<8; i++) flag_useChan[i] = true;
	}
	
    virtual void update(void);

    int enableChannel(unsigned int channel, bool enable = true) {
      if ((channel >= 8) || (channel < 0)) return -1;
      return (int) (flag_useChan[channel] = enable);
    }
	void mute(void) { for (int i=0; i < 8; i++) enableChannel(i,false); };  //mute all channels
	
	int switchChannel(unsigned int channel) { 
		//mute all channels except the given one.  Set the given one to 1.0.
		if ((channel >= 8) || (channel < 0)) return -1;
		mute(); 
		enableChannel(channel);
		return channel;
	} 

  private:
    audio_block_f32_t *inputQueueArray[8];
    bool flag_useChan[8];
};


#endif