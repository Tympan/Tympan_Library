/* 
 * AdioSynthWaveformSine_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Modeled on: AudioSynthWaveformSine from Teensy Audio Library
 * 
 * Purpose: Create sine wave of given amplitude and frequency
 *
 * License: MIT License. Use at your own risk.        
 *
 */

#ifndef synth_sine_f32_h_
#define synth_sine_f32_h_

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "arm_math.h"
#include <SerialManager_UI.h>

class AudioSynthWaveformSine_F32; //forward declare.  to be fully defined later in this file

//This class helps manage some of the configuration and state information of the AudioSynthWaveformSine_F32
//class.  By having this class, it tries to put everything in one place.  It is also helpful for
//managing the GUI on the TympanRemote mobile App.
class AudioSynthWaveformSineState {
	public:
		AudioSynthWaveformSineState(void) {}; //nothing to do in the constructor;
	
		//The sinewave is tricky because it has several configuration parameters that I'd like to track here in
		//this State-tracking class because I want to access them via the App's GUI.  But, if I keep a local copy
		//of the parameter values, I need to do a lot of management to ensure that they are in-sync with the underlying
		//AudioSynthWaveformSine_F32 class.  So, instead, I'm going to just get the values from the class itself.
	
		//get parameter values from the compressors
		bool getEnabled(void);
		bool setEnabled(bool);
		float getSampleRate_Hz(void);
		float getFrequency_Hz(void);
		float getAmplitude(void);
		float getAmplitude_dBFS(void);

		//These methods are not used to directly maintain the state of the AudioEffectCompWDRC.
		//They are supporting methods.
		void setSynthWaveform(AudioSynthWaveformSine_F32 *c);
		void printWaveformParameters(void);
  
	protected:
		AudioSynthWaveformSine_F32 *synthWaveform;  //will be an array of pointers to our compressors	
};


class AudioSynthWaveformSine_F32 : public AudioStream_F32
{
//GUI: inputs:0, outputs:1 //this line used for automatic generation of GUI node
//GUI: shortName:sine  //this line used for automatic generation of GUI node
public:
	AudioSynthWaveformSine_F32() : AudioStream_F32(0, NULL) { 
		setDefaultValues();
	} //uses default AUDIO_SAMPLE_RATE from AudioStream.h
	AudioSynthWaveformSine_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL) {
		setDefaultValues();
		setSampleRate_Hz(settings.sample_rate_Hz);
	}
	virtual void frequency(float freq) {
		if (freq < 0.0) freq = 0.0;
		else if (freq > sample_rate_Hz/2.f) freq = sample_rate_Hz/2.f;
		frequency_Hz = freq;
		phase_increment = freq * (4294967296.0 / sample_rate_Hz);
	}
	virtual void phase(float angle) {
		if (angle < 0.0f) angle = 0.0f;
		else if (angle > 360.0f) {
			angle = angle - 360.0f;
			if (angle >= 360.0f) return;
		}
		phase_accumulator = angle * (4294967296.0f / 360.0f);
	}
	virtual void amplitude(float n) {
		if (n < 0) n = 0;
		else if (n > 1.0f) n = 1.0f;
		magnitude = n * magnitude_scale_factor;
	}
	virtual float getSampleRate_Hz(void) { return sample_rate_Hz; }
	virtual void setSampleRate_Hz(const float &fs_Hz) {
		phase_increment *= sample_rate_Hz / fs_Hz; //change the phase increment for the new frequency
		sample_rate_Hz = fs_Hz;
	}
	virtual void begin(void) { setEnabled(true); }
	virtual void end(void) { setEnabled(false); }
	virtual void update(void);
	virtual float getFrequency_Hz(void) { return frequency_Hz; }
	virtual float setFrequency_Hz(float _freq_Hz) { frequency(_freq_Hz); return getFrequency_Hz(); }
	virtual float getAmplitude(void) { return (magnitude/magnitude_scale_factor); }
	virtual float setAmplitude(float amp) { amplitude(amp); return getAmplitude(); }
	virtual float getAmplitude_dBFS(void) { return 10.0*log10f(pow(getAmplitude() / sqrtf(2.0), 2.0)); } //covert peak to RMS
	virtual float setAmplitude_dBFS(float amp_dBFS) { setAmplitude(sqrtf(pow(10.0,amp_dBFS/10.0))*sqrtf(2.0)); return getAmplitude_dBFS(); } //dBFS is an RMS-type value, so the sqrtf(2.0) converts it to a peak-type value
	
	virtual float incrementAmplitude_dBFS(float incr_dB) { return setAmplitude_dBFS(getAmplitude_dBFS()+incr_dB); }
	virtual float incrementFrequency(float incr_fac) { return setFrequency_Hz(getFrequency_Hz()*incr_fac); }
	virtual float incrementFrequency_Hz(float incr_Hz) { return setFrequency_Hz(getFrequency_Hz()+incr_Hz); }
	
	virtual bool setEnabled(bool _enable) { return enabled = _enable; }
	virtual bool getEnabled(void) { return enabled; }
	
	AudioSynthWaveformSineState state;
	
protected:
	uint32_t phase_accumulator = 0;
	uint32_t phase_increment = 0;
	int32_t magnitude = 0;
	float sample_rate_Hz = AUDIO_SAMPLE_RATE;
	volatile bool enabled = true;
	float32_t frequency_Hz;
	const float magnitude_scale_factor = 65536.0f;
	unsigned int block_counter=0;
	virtual void setDefaultValues(void);
};


// ////////////////////////////////////////////////////////////////////////////////////////////////
//
// UI Versions of the Class
//
// This "UI" version of the class adds no signal processing functionality.  Instead, it adds to the
// class simply to make it easier to add a menu-based or App-based interface to configure and to 
// control the audio-processing in AudioEffectCompWDRC_F32 above.
//
// ////////////////////////////////////////////////////////////////////////////////////////////////

class AudioSynthWaveformSine_F32_UI: public AudioSynthWaveformSine_F32, public SerialManager_UI {
	public:
		AudioSynthWaveformSine_F32_UI(void) : 	AudioSynthWaveformSine_F32(), SerialManager_UI() {};
		AudioSynthWaveformSine_F32_UI(const AudioSettings_F32 settings): AudioSynthWaveformSine_F32(settings), SerialManager_UI() {		};
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false); 
		// ///////// end of required methods
	
		//create the button sets for the TympanRemote's GUI
		TR_Card* addCard_enable(TR_Page* page_h);
		TR_Card* addCard_freq(TR_Page *page_h);
		//TR_Card* addCard_amp( TR_Page *page_h);
		TR_Card* addCard_amp_dBFS( TR_Page *page_h);
		TR_Page* addPage_default(TympanRemoteFormatter *gui);
		
		//methods to update the GUI fields
		void updateCard_enable(void);
		void updateCard_freq(void);
		//void updateCard_amp(void); 
		void updateCard_amp_dBFS(void); 
			
		//here are the factors to use to increment different parameters
		float amp_fac_dB = 3.0;
		float freq_fac = powf(2.0,1.0/3.0); //third octave steps
		
		String name_for_UI = "Sine Wave";  //used for printHelp()
	
	protected:	
		
};




#endif
