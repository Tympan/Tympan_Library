/*
 * AudioMixer
 * 
 * AudioMixer4
 * Created: Patrick Radius, December 2016
 * Purpose: Mix up to 4 audio channels with individual gain controls.
 * Assumes floating-point data.
 *          
 * This processes a single stream fo audio data (ie, it is mono)       
 *
 * Extended to AudioMixer8
 * By: Chip Audette, OpenAudio, Feb 2017
 *          
 * MIT License.  use at your own risk.
*/

#ifndef AUDIOMIXER_F32_H
#define AUDIOMIXER_F32_H

#include <arm_math.h> 
#include "AudioStream_F32.h"

#define MIXER_N_CHAN_MAX (16U)
class AudioMixerBase_F32 : public AudioStream_F32  {
	public:
		AudioMixerBase_F32(const unsigned int _N_CHAN) : AudioStream_F32(MIXER_N_CHAN_MAX, inputQueueArray), N_CHAN(_N_CHAN) {
			N_CHAN = max(1U, min(_N_CHAN, MIXER_N_CHAN_MAX));
			setDefaultInstanceName(); 
			setDefaultValues();
		}
		AudioMixerBase_F32(const unsigned int _N_CHAN, const AudioSettings_F32 &settings) : AudioMixerBase_F32(_N_CHAN) {
			sample_rate_Hz = settings.sample_rate_Hz;
			audio_block_samples = settings.audio_block_samples;
		}
		

		virtual void setDefaultInstanceName(void) { 
			instanceName = "AudioMixer" + String(N_CHAN) + "_F32";  //instanceName is part of AudioStream_F32
		}
	
		virtual void setDefaultValues(void) {
			if (N_CHAN == 0U) return;
			for (unsigned int i=0U; i<N_CHAN; i++) multiplier[i] = 1.0;
		}

		void update(void) override;
		virtual int processData(audio_block_f32_t *audio_in[], audio_block_f32_t *audio_out); //audio_in can be read-only as no calculations are in-place

		virtual void gain(unsigned int channel, float gain) {
			if (N_CHAN == 0U) return;
			if ((channel >= N_CHAN) || (channel < 0U)) return;
			multiplier[channel] = gain;
		}
		virtual float getGain(unsigned int channel) {
			if (N_CHAN == 0U) return 0.0;
			if (channel >= N_CHAN) return 0.0;
			return multiplier[channel];
		}

		virtual void mute(void) { 
			if (N_CHAN == 0U) return;
			for (unsigned int i=0; i < N_CHAN; i++) gain(i,0.0);    //mute all channels
		};

		int switchChannel(unsigned int channel) { 
			if (N_CHAN == 0U) return -1;

			//mute all channels except the given one.  Set the given one to 1.0.
			if (channel >= N_CHAN) return -1;
			mute(); 
			gain(channel,1.0);
			return channel;
		} 


	protected:
		static const unsigned int MAX_N_CHAN = MIXER_N_CHAN_MAX;
		unsigned int N_CHAN;
		audio_block_f32_t *inputQueueArray[MIXER_N_CHAN_MAX];
	    float32_t multiplier[MIXER_N_CHAN_MAX];
		float sample_rate_Hz = AUDIO_SAMPLE_RATE;
		int audio_block_samples = AUDIO_BLOCK_SAMPLES;

};


class AudioMixer4_F32 : public AudioMixerBase_F32 {
	//GUI: inputs:4, outputs:1  //this line used for automatic generation of GUI node
	//GUI: shortName:Mixer4
	public:
    	AudioMixer4_F32() : AudioMixerBase_F32(4) { }    //see constructor for AudioMixerBase for its default actions
		AudioMixer4_F32(const AudioSettings_F32 &settings) : AudioMixerBase_F32(4, settings) { } //see constructor for MixerBase for its default actions
	
		//virtual void update(void);
		//virtual int processData(audio_block_f32_t *audio_in[4], audio_block_f32_t *audio_out); //audio_in can be read-only as no calculations are in-place

};

class AudioMixer8_F32 : public AudioMixerBase_F32 {
	//GUI: inputs:8, outputs:1  //this line used for automatic generation of GUI node
	//GUI: shortName:Mixer8
	public:
		AudioMixer8_F32() : AudioMixerBase_F32(8) { }  //see constructor for MixerBase for its default actions
		AudioMixer8_F32(const AudioSettings_F32 &settings) : AudioMixerBase_F32(8, settings) { } //see constructor for MixerBase for its default actions
		
		//void update(void) override;

};


class AudioMixer16_F32 : public AudioMixerBase_F32 {
	//GUI: inputs:16, outputs:1  //this line used for automatic generation of GUI node
	//GUI: shortName:Mixer16
	public:
		AudioMixer16_F32() : AudioMixerBase_F32(16) { }  //see constructor for MixerBase for its default actions
		AudioMixer16_F32(const AudioSettings_F32 &settings) : AudioMixerBase_F32(16, settings) { } //see constructor for MixerBase for its default actions
		
		//void update(void) override;
};

#endif