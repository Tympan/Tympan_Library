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
	protected:
	    static const int N_CHAN = 4;
	public:
		AudioSummer4_F32() : AudioStream_F32(N_CHAN, inputQueueArray) { setInstanceName(); setDefaultValues(); }
		AudioSummer4_F32(const AudioSettings_F32 &settings) : AudioStream_F32(N_CHAN, inputQueueArray) { setInstanceName(); setDefaultValues(); }
		
		void setInstanceName(void) { instanceName = "AudioSummer4_F32"; }
		
		void setDefaultValues(void) {
			for (int i=0; i<N_CHAN; i++) flag_useChan[i] = true;
		}
	
		void update(void) override;
		virtual int processData(audio_block_f32_t *audio_in[N_CHAN], audio_block_f32_t *audio_out); //audio_in can be read-only as no calculations are in-place

		//enableChannel() activates a channel without turning off the other channels
		int enableChannel(unsigned int channel, bool enable = true) {
			if ((channel >= N_CHAN) || (channel < 0)) return -1;
			return (int)(flag_useChan[channel] = enable);
		}

		//mute() deactivates all channels
		void mute(void) { for (int i=0; i < N_CHAN; i++) enableChannel(i,false); };  //mute all channels
	
		//switchChannel() activates a channel while de-activating all the other channels
		int switchChannel(unsigned int channel) { 
			//mute all channels except the given one.  Set the given one to 1.0.
			if ((channel >= N_CHAN) || (channel < 0)) return -1;
			mute(); 
			enableChannel(channel);
			return channel;
		} 

  private:
    audio_block_f32_t *inputQueueArray[N_CHAN];
	bool flag_useChan[N_CHAN];

};

class AudioSummer8_F32 : public AudioStream_F32 {
	//GUI: inputs:8, outputs:1  //this line used for automatic generation of GUI node
	//GUI: shortName:Summer8
	protected:
	    static const int N_CHAN = 8;
	public:
    	AudioSummer8_F32() : AudioStream_F32(N_CHAN, inputQueueArray) { setInstanceName(); setDefaultValues(); }
		AudioSummer8_F32(const AudioSettings_F32 &settings) : AudioStream_F32(N_CHAN, inputQueueArray) { setInstanceName(); setDefaultValues(); }
		
		void setInstanceName(void) { instanceName = "AudioSummer8_F32"; }
		
		void setDefaultValues(void) {
			for (int i=0; i<N_CHAN; i++) flag_useChan[i] = true;
		}
		
		void update(void) override;
		virtual int processData(audio_block_f32_t *audio_in[N_CHAN], audio_block_f32_t *audio_out); //audio_in can be read-only as no calculations are in-place


		//enableChannel() activates a channel without turning off the other channels
		int enableChannel(unsigned int channel, bool enable = true) {
			if ((channel >= N_CHAN) || (channel < 0)) return -1;
			return (int)(flag_useChan[channel] = enable);
		}

		//mute() deactivates all channels
		void mute(void) { for (int i=0; i < N_CHAN; i++) enableChannel(i,false); };  //mute all channels
		
		//switchChannel() activates a channel while de-activating all the other channels
		int switchChannel(unsigned int channel) { 
			//mute all channels except the given one.  Set the given one to 1.0.
			if ((channel >= N_CHAN) || (channel < 0)) return -1;
			mute(); 
			enableChannel(channel);
			return channel;
		} 

  private:
    audio_block_f32_t *inputQueueArray[N_CHAN];
    bool flag_useChan[N_CHAN];
};


#endif