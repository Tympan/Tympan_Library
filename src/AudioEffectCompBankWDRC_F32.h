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
#include <vector>

#define __MAX_NUM_COMP 24  //max number of compressors.  Limited by number fo inputs that we enable for the Audio Library connections

//This class helps manage some of the configuration and state information of the AudioEffectCompWDRC classes.
//It is also helpful for managing the GUI on the TympanRemote mobile App.
class AudioCompBankStateWDRC_F32 {
	public:
		AudioCompBankStateWDRC_F32(void) {}; //nothing to do in the constructor;
	
		float sample_rate_Hz = 0.0f;

		//keep track of th enumber of channels
		int set_n_chan(int n);
		int get_n_chan(void) { return n_chan; }
		
		//keep track of the maximum number of filters...the user shouldn't have to worry about this
		//int set_max_n_chan(int n);
		void setCompressors(std::vector<AudioEffectCompWDRC_F32> &c); //also defines max_n_chan
		int get_max_n_chan(void) { return compressors.size(); }
		
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
		int n_chan = 0;      //should correspond to however many of the filters are actually being employed by the AuioFilterbank
		//float *crossover_freq_Hz;  //this really only needs to be [nfilters-1] in length, but we'll generally allocate [nfilters] just to avoid mistaken overruns
		std::vector<AudioEffectCompWDRC_F32 *> compressors;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
	
};


//This is the compressor bank
class AudioEffectCompBankWDRC_F32 : public AudioStream_F32 {
//GUI: inputs:8, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:CompBankWDRC
	public:
		AudioEffectCompBankWDRC_F32(void): AudioStream_F32(__MAX_NUM_COMP,inputQueueArray) { }
		
		AudioEffectCompBankWDRC_F32(const AudioSettings_F32 &settings) : AudioStream_F32(__MAX_NUM_COMP,inputQueueArray) {
			setSampleRate_Hz(settings.sample_rate_Hz);
			//audio_settings_ptr = &settings;
		}
	
		//define the update() function, which is called automatically every time there is a new audio block
		virtual void update(void);
	
		//set number of channels
		int set_max_n_chan(int n_max_chan);
		int set_n_chan(int n);
		int get_n_chan(void) { return state.get_n_chan(); }
			
		//set sample rate 
		float setSampleRate_Hz(const float _fs_Hz) {
			if (compressors.size() > 0) {
				for (int i=0; i < (int)compressors.size(); i++) {
					state.sample_rate_Hz = compressors[i].setSampleRate_Hz(_fs_Hz);
				}
			}
			return state.sample_rate_Hz;
		}
		
		//configure from BTNRH DSL and GHA...returns number of channels loaded from the DSL
		int configureFromDSLandGHA(float fs_Hz, const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha);
	
		//track states
		AudioCompBankStateWDRC_F32 state;
		
		// /////////////////////////////// set / get methods
		
		// get parameter values from the compressors
		float getAttack_msec(int i=0) { if (i < get_n_chan()) { return compressors[i].getAttack_msec(); } else { return 0.0f; }};
		float getRelease_msec(int i=0) { if (i < get_n_chan()) { return compressors[i].getRelease_msec(); } else { return 0.0f; }};
		float getMaxdB(int i=0) { if (i < get_n_chan()) { return compressors[i].getMaxdB(); } else { return 0.0f; }};
		float getExpansionCompRatio(int i=0) { if (i < get_n_chan()) { return compressors[i].getExpansionCompRatio(); } else { return 0.0f; }};
		float getKneeExpansion_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i].getKneeExpansion_dBSPL(); } else { return 0.0f; }};
		float getGain_dB(int i=0) { if (i < get_n_chan()) { return compressors[i].getGain_dB(); } else { return 0.0f; }};
		float getCompRatio(int i=0) { if (i < get_n_chan()) { return compressors[i].getCompRatio(); } else { return 0.0f; }};
		float getKneeCompressor_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i].getKneeCompressor_dBSPL(); } else { return 0.0f; }};
		float getKneeLimiter_dBSPL(int i=0) { if (i < get_n_chan()) { return compressors[i].getKneeLimiter_dBSPL(); } else { return 0.0f; }};

		float getScaleFactor_dBSPL_at_dBFS(int i=0) { return getMaxdB(i); }  //another name for getMaxdB
		float getLinearGain_dB(int i=0) { return getGain_dB(i); }   //another name for getGain_dB

		
		// set parameter values to the all the compressors (ie, parameters that you might want to set globally)
		float setAttack_msec_all(float val) { for (int i=0; i < state.get_max_n_chan(); i++) setAttack_msec(val,i); return getAttack_msec(); }
		float setRelease_msec_all(float val) { for (int i=0; i < state.get_max_n_chan(); i++) setRelease_msec(val,i); return getRelease_msec(); }
		float setMaxdB_all(float val) { for (int i=0; i < state.get_max_n_chan(); i++) setMaxdB(val,i); return getMaxdB(); }

		float setScaleFactor_dBSPL_at_dBFS_all(float val) { return setMaxdB_all(val); } //another name for setMaxdB_all
		
		// set parameter values for a particular compressor
		float setAttack_msec(float val, int i) { if (i < get_n_chan()) { return compressors[i].setAttack_msec(val); } else { return 0.0f; }};
		float setRelease_msec(float val, int i) { if (i < get_n_chan()) { return compressors[i].setRelease_msec(val); } else { return 0.0f; }};
		float setMaxdB(float val, int i) { if (i < get_n_chan()) { return compressors[i].setMaxdB(val); } else { return 0.0f; }};
		float setExpansionCompRatio(float val, int i) { if (i < get_n_chan()) { return compressors[i].setExpansionCompRatio(val); } else { return 0.0f; }};
		float setKneeExpansion_dBSPL(float val, int i) { if (i < get_n_chan()) { return compressors[i].setKneeExpansion_dBSPL(val); } else { return 0.0f; }};
		float setGain_dB(float val, int i) { if (i < get_n_chan()) { return compressors[i].setGain_dB(val); } else { return 0.0f; }};
		float setCompRatio(float val, int i) { if (i < get_n_chan()) { return compressors[i].setCompRatio(val); } else { return 0.0f; }};
		float setKneeCompressor_dBSPL(float val, int i) { if (i < get_n_chan()) { return compressors[i].setKneeCompressor_dBSPL(val); } else { return 0.0f; }};
		float setKneeLimiter_dBSPL(float val, int i) { if (i < get_n_chan()) { return compressors[i].setKneeLimiter_dBSPL(val); } else { return 0.0f; }};
				
		float setScaleFactor_dBSPL_at_dBFS(float val, int i) { return setMaxdB(val,i); }  //another name for setMaxdB
		float setLinearGain_dB(float val, int i) { return setGain_dB(val,i); }  //another name for setGain_dB

	
		//here are the compressors...replace with a vector?
		//AudioEffectCompWDRC_F32 compressors[__MAX_NUM_COMP];
		std::vector<AudioEffectCompWDRC_F32> compressors;
		
	protected:
		audio_block_f32_t *inputQueueArray[__MAX_NUM_COMP];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
		//AudioSettings_F32 *audio_settings_ptr = NULL;
};

#endif