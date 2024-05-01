/* 
 * AdioSynthSilence_F32
 * 
 * Created: Chip Audette (OpenAudio) Sept 2010
 * Modeled on: AudioSynthWaveformSine from Teensy Audio Library
 * 
 * Purpose: Create a block of silence
 *
 * License: MIT License. Use at your own risk.        
 *
 */

#ifndef synth_silence_f32_h_
#define synth_silence_f32_h_

#include "Arduino.h"
#include "AudioStream_F32.h"
#include "arm_math.h"


class AudioSynthSilence_F32 : public AudioStream_F32
{
//GUI: inputs:0, outputs:1 //this line used for automatic generation of GUI node
//GUI: shortName:silence  //this line used for automatic generation of GUI node
public:
	AudioSynthSilence_F32() : AudioStream_F32(0, NULL) { } //uses default AUDIO_SAMPLE_RATE from AudioStream.h
	AudioSynthSilence_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL) {
		setSampleRate_Hz(settings.sample_rate_Hz);
	}

	void setSampleRate_Hz(const float &fs_Hz) {
		sample_rate_Hz = fs_Hz;
	}
	void begin(void) { enabled = true; }
	void end(void) { enabled = false; }
	virtual void update(void);
	
	
private:

	float sample_rate_Hz = AUDIO_SAMPLE_RATE;
	volatile uint8_t enabled = 1;
	float32_t frequency_Hz;
	unsigned int block_counter=0;
};



#endif
