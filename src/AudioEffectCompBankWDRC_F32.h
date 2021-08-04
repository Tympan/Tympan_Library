/*
 * AudioEffectCompBankWDRC_F32: A bank of Wide Dynamic Rnage Compressor (WDRCs)
 * 
 * Created: Chip Audette (OpenAudio) August 2021
 *
 * Purpose: Ceate bank of WDRC compressors to make it easier to manage having multiple
 *      compressors so that you don't have to handle each individual filter yourself.
 *
 * MIT License.  Use at your own risk.
 * 
 */
 
  
#ifndef _AudioEffectCompBankWDRC_F32_h
#define _AudioEffectCompBankWDRC_F32_h


#include <Arduino.h>
#include <AudioStream_F32.h>
#include <AudioSettings_F32.h>
#include <AudioEffectCompWDRC_F32.h>	  //from Tympan_Library
#include <SerialManager_UI.h>			  //from Tympan_Library
#include <TympanRemoteFormatter.h> 		  //from Tympan_Library

#define __MAX_NUM_COMP 8    //maximum number of compressors to allow

//This class helps manage some of the configuration and state information of the AudioEffectCompWDRC classes.
//It is also helpful for managing the GUI on the TympanRemote mobile App.
class AudioCompBankStateWDRC_F32 {
	public:
		AudioCompBankStateWDRC_F32(void) {}; //nothing to do in the constructor
		~AudioCompBankStateWDRC_F32(void) { if ( compressors != NULL) delete compressors; };
	
		float sample_rate_Hz = 0.0f;

		//keep track of th enumber of channels
		int set_n_chan(int n);
		int get_n_chan(void) { return n_chan; }
		
		//keep track of the maximum number of filters...the user shouldn't have to worry about this
		//int set_max_n_chan(int n);
		void setCompressors(int n, AudioEffectCompWDRC_F32 c[]); //also defines max_n_chan
		int get_max_n_chan(void) { return max_n_chan; }
		
		//get parameter values from the compressors
		float getAttack_msec(int i=0) { if (i < get_n_chan()) { return compressors[i]->getAttack_msec(); } else { return 0.0f; }};
		float getRelease_msec(int i=0) { if (i < get_n_chan()) { return compressors[i]->getRelease_msec(); } else { return 0.0f; }};
		float getScaleFactor_dBSPL_at_dBFS(int i=0) { if (i < get_n_chan()) { return compressors[i]->getMaxdB(); } else { return 0.0f; }};
		float getExpansionCompRatio(int i=0) { if (i < get_n_chan()) { return compressors[i]->getExpansionCompRatio(); } else { return 0.0f; }};
		float getKneeExpansion_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i]->getKneeExpansion_dBSPL(); } else { return 0.0f; }};
		float getLinearGain_dB(int i=0) { if (i < get_n_chan()) { return compressors[i]->getGain_dB(); } else { return 0.0f; }};
		float getCompRatio(int i=0) { if (i < get_n_chan()) { return compressors[i]->getCompRatio(); } else { return 0.0f; }};
		float getKneeCompressor_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i]->getKneeCompressor_dBSPL(); } else { return 0.0f; }};
		float getKneeLimiter_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i]->getKneeLimiter_dBSPL(); } else { return 0.0f; }};

	protected:
		AudioEffectCompWDRC_F32** compressors;  //will be an array of pointers to our compressors
		int max_n_chan = 0;  //should correspond to the length of the array of compressors
		int n_chan = 0;      //should correspond to however many of the filters are actually being employed by the AuioFilterbank
		//float *crossover_freq_Hz;  //this really only needs to be [nfilters-1] in length, but we'll generally allocate [nfilters] just to avoid mistaken overruns


	
};


//This is the compressor bank
class AudioEffectCompBankWDRC_F32 : public AudioStream_F32 {
//GUI: inputs:8, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:CompBankWDRC
	public:
		AudioEffectCompBankWDRC_F32(void): AudioStream_F32(__MAX_NUM_COMP,inputQueueArray) { 
			setSampleRate_Hz(compressors[0].getSampleRate_Hz());
			state.setCompressors(__MAX_NUM_COMP,compressors); //initilize
		} 
		AudioEffectCompBankWDRC_F32(const AudioSettings_F32 &settings) : AudioStream_F32(__MAX_NUM_COMP,inputQueueArray) {
			setSampleRate_Hz(settings.sample_rate_Hz);
			state.setCompressors(__MAX_NUM_COMP,compressors); //initilize
		}
	
	
		//define the update() function, which is called automatically every time there is a new audio block
		virtual void update(void);
	
		//set number of channels
		int set_n_chan(int n);
		int get_n_nchan(void) { return state.get_n_chan(); }
			
		//set sample rate 
		float setSampleRate_Hz(const float _fs_Hz) {
			for (int i=0; i< __MAX_NUM_COMP; i++) {
				state.sample_rate_Hz = compressors[i].setSampleRate_Hz(_fs_Hz);
			}
			return state.sample_rate_Hz;
		}
		
		//configure from BTNRH DSL and GHA...returns number of channels loaded from the DSL
		int configureFromDSLandGHA(float fs_Hz, const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha);
	
		//track states
		AudioCompBankStateWDRC_F32 state;
	
		//here are the compressors...replace with a vector?
		AudioEffectCompWDRC_F32 compressors[__MAX_NUM_COMP];
	protected:
		audio_block_f32_t *inputQueueArray[__MAX_NUM_COMP];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
			
};

#endif