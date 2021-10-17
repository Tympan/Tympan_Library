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
class AudioEffectCompBankWDRCState {
	public:
		AudioEffectCompBankWDRCState(void) {}; //nothing to do in the constructor;
	
		float sample_rate_Hz = 0.0f;

		//keep track of th enumber of channels
		int set_n_chan(int n);
		int get_n_chan(void) { return n_chan; }
		
		//keep track of the maximum number of filters...the user shouldn't have to worry about this
		//int set_max_n_chan(int n);
		void setCompressors(std::vector<AudioEffectCompWDRC_F32> &c); //also defines max_n_chan
		int get_max_n_chan(void) { return compressors.size(); }
		
		//get parameter values from the compressors
		float getAttack_msec(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getAttack_msec(); } else { return 0.0f; }};
		float getRelease_msec(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getRelease_msec(); } else { return 0.0f; }};
		float getScaleFactor_dBSPL_at_dBFS(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getMaxdB(); } else { return 0.0f; }};
		float getExpansionCompRatio(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getExpansionCompRatio(); } else { return 0.0f; }};
		float getKneeExpansion_dBSPL(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getKneeExpansion_dBSPL(); } else { return 0.0f; }};
		float getLinearGain_dB(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getGain_dB(); } else { return 0.0f; }};
		float getCompRatio(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getCompRatio(); } else { return 0.0f; }};
		float getKneeCompressor_dBSPL(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getKneeCompressor_dBSPL(); } else { return 0.0f; }};
		float getKneeLimiter_dBSPL(int i=0) { if (i < get_max_n_chan()) { return compressors[i]->getKneeLimiter_dBSPL(); } else { return 0.0f; }};

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
		int get_max_n_chan(void) { return state.get_max_n_chan(); }
			
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
		void collectParams_into_CHA_DSL(BTNRH_WDRC::CHA_DSL *this_dsl);
	
		//track states
		AudioEffectCompBankWDRCState state;
		
		bool enable(bool val=true) { return is_enabled = val; }
		
		// /////////////////////////////// set / get methods
		
		// get parameter values from the compressors
		float getAttack_msec(int i=0)          { if (i < get_max_n_chan()) { return compressors[i].getAttack_msec();  } else { return 0.0f; }};
		float getRelease_msec(int i=0)         { if (i < get_max_n_chan()) { return compressors[i].getRelease_msec(); } else { return 0.0f; }};
		float getMaxdB(int i=0)                { if (i < get_max_n_chan()) { return compressors[i].getMaxdB();        } else { return 0.0f; }};
		float getExpansionCompRatio(int i=0)   { if (i < get_max_n_chan()) { return compressors[i].getExpansionCompRatio();   } else { return 0.0f; }};
		float getKneeExpansion_dBSPL(int i=0)  { if (i < get_max_n_chan()) { return compressors[i].getKneeExpansion_dBSPL();  } else { return 0.0f; }};
		float getGain_dB(int i=0)              { if (i < get_max_n_chan()) { return compressors[i].getGain_dB();      } else { return 0.0f; }};
		float getCompRatio(int i=0)            { if (i < get_max_n_chan()) { return compressors[i].getCompRatio();    } else { return 0.0f; }};
		float getKneeCompressor_dBSPL(int i=0) { if (i < get_max_n_chan()) { return compressors[i].getKneeCompressor_dBSPL(); } else { return 0.0f; }};
		float getKneeLimiter_dBSPL(int i=0)    { if (i < get_max_n_chan()) { return compressors[i].getKneeLimiter_dBSPL();    } else { return 0.0f; }};

		float getScaleFactor_dBSPL_at_dBFS(int i=0) { return getMaxdB(i);   }  //another name for getMaxdB
		float getLinearGain_dB(int i=0)             { return getGain_dB(i); }   //another name for getGain_dB

		
		// set parameter values to the all the compressors (ie, parameters that you might want to set globally)
		float setAttack_msec_all(float val)  { for (int i=0; i < get_max_n_chan(); i++) setAttack_msec(val,i);  return getAttack_msec(); }
		float setRelease_msec_all(float val) { for (int i=0; i < get_max_n_chan(); i++) setRelease_msec(val,i); return getRelease_msec(); }
		float setMaxdB_all(float val)        { for (int i=0; i < get_max_n_chan(); i++) setMaxdB(val,i);        return getMaxdB(); }

		float setScaleFactor_dBSPL_at_dBFS_all(float val) { return setMaxdB_all(val); } //another name for setMaxdB_all
		
		// set parameter values for a particular compressor
		float setAttack_msec(float val, int i)  { if (i < get_max_n_chan()) { return compressors[i].setAttack_msec(val);  } else { return 0.0f; }};
		float setRelease_msec(float val, int i) { if (i < get_max_n_chan()) { return compressors[i].setRelease_msec(val); } else { return 0.0f; }};
		float setMaxdB(float val, int i)        { if (i < get_max_n_chan()) { return compressors[i].setMaxdB(val);        } else { return 0.0f; }};
		float setExpansionCompRatio(float val, int i)   { if (i < get_max_n_chan()) { return compressors[i].setExpansionCompRatio(val);   } else { return 0.0f; }};
		float setKneeExpansion_dBSPL(float val, int i)  { if (i < get_max_n_chan()) { return compressors[i].setKneeExpansion_dBSPL(val);  } else { return 0.0f; }};
		float setGain_dB(float val, int i)      { if (i < get_max_n_chan()) { return compressors[i].setGain_dB(val);      } else { return 0.0f; }};
		float setCompRatio(float val, int i)    { if (i < get_max_n_chan()) { return compressors[i].setCompRatio(val);    } else { return 0.0f; }};
		float setKneeCompressor_dBSPL(float val, int i) { if (i < get_max_n_chan()) { return compressors[i].setKneeCompressor_dBSPL(val); } else { return 0.0f; }};
		float setKneeLimiter_dBSPL(float val, int i)    { if (i < get_max_n_chan()) { return compressors[i].setKneeLimiter_dBSPL(val);    } else { return 0.0f; }};
				
		float setScaleFactor_dBSPL_at_dBFS(float val, int i) { return setMaxdB(val,i); }  //another name for setMaxdB
		float setLinearGain_dB(float val, int i)             { return setGain_dB(val,i); }  //another name for setGain_dB


		float incrementAttack_all(float fac)  { return setAttack_msec_all(getAttack_msec() * fac); };
		float incrementRelease_all(float fac) { return setRelease_msec_all(getRelease_msec() * fac); };
		float incrementMaxdB_all(float fac)   { return setMaxdB_all(getMaxdB() + fac); }
		
 		float incrementAttack(float fac, int i)    { return setAttack_msec(getAttack_msec(i) * fac, i); };
		float incrementRelease(float fac, int i)   { return setRelease_msec(getRelease_msec(i) * fac, i); };
		float incrementMaxdB(float fac, int i)     { return setMaxdB(getMaxdB(i) + fac, i); }
		float incrementExpCR(float fac, int i)     { return setExpansionCompRatio(max(0.1f,getExpansionCompRatio(i) + fac), i); }
		float incrementExpKnee(float fac, int i)   { return setKneeExpansion_dBSPL(getKneeExpansion_dBSPL(i) + fac, i); }
		float incrementGain_dB(float increment_dB, int i) { return setGain_dB(getGain_dB(i) + increment_dB, i); }    
		float incrementCompRatio(float fac, int i) { return setCompRatio(max(0.1f, getCompRatio(i) + fac), i); }
		float incrementKnee(float fac, int i)      { return setKneeCompressor_dBSPL(getKneeCompressor_dBSPL(i) + fac, i);}
		float incrementLimiter(float fac, int i)   { return setKneeLimiter_dBSPL(getKneeLimiter_dBSPL(i) + fac, i);};
 	
		//here are the compressors...replace with a vector?
		//AudioEffectCompWDRC_F32 compressors[__MAX_NUM_COMP];
		std::vector<AudioEffectCompWDRC_F32> compressors;
		
	protected:
		audio_block_f32_t *inputQueueArray[__MAX_NUM_COMP];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
		//AudioSettings_F32 *audio_settings_ptr = NULL;
};



// ////////////////////////////////////////////////////////////////////////////////////////////////
//
// UI Versions of the Classes
//
// These versions of the classes add no signal processing functionality.  Instead, they add to the
// classes simply to make it easier to add a menu-based or App-based interface to configure and 
// control the audio-processing classes above.
//
// If you want to add a GUI, you might consider using the classes below instead of the classes above.
// Again, the signal processing is exactly the same either way.
//
// ////////////////////////////////////////////////////////////////////////////////////////////////


//This is the base class to be inherited by the FIR and Biquad versions
class AudioEffectCompBankWDRC_F32_UI : public AudioEffectCompBankWDRC_F32, public SerialManager_UI {
	public:
		AudioEffectCompBankWDRC_F32_UI(void) : AudioEffectCompBankWDRC_F32(), SerialManager_UI() {};
		AudioEffectCompBankWDRC_F32_UI(const AudioSettings_F32 &settings) : AudioEffectCompBankWDRC_F32(settings), SerialManager_UI() {};
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false); 
		// ///////// end of required methods
		
		virtual bool processCharacter_global(char data_char);
		virtual bool processCharacter_perChannel(char data_char, int chan);
		
		// ////////// Traditional Mode of Operating
		
		//create the button sets for the TympanRemote's GUI
		TR_Page* addPage_globals(TympanRemoteFormatter *gui);
		TR_Card* addCard_attack_global(   TR_Page *page_h);
		TR_Card* addCard_release_global(  TR_Page *page_h);
		TR_Card* addCard_scaleFac_global( TR_Page *page_h);
		
		TR_Card* addCards_globals(TR_Page *page_h);
	
		TR_Page* addPage_attack(       TympanRemoteFormatter *gui);
		TR_Page* addPage_release(      TympanRemoteFormatter *gui);
		TR_Page* addPage_scaleFac(     TympanRemoteFormatter *gui);
		TR_Page* addPage_expCompRatio( TympanRemoteFormatter *gui);
		TR_Page* addPage_expKnee(      TympanRemoteFormatter *gui);
		TR_Page* addPage_linearGain(   TympanRemoteFormatter *gui);
		TR_Page* addPage_compRatio(    TympanRemoteFormatter *gui);
		TR_Page* addPage_compKnee(     TympanRemoteFormatter *gui);
		TR_Page* addPage_limKnee(      TympanRemoteFormatter *gui);
		
		//TR_Page* addPage_default(TympanRemoteFormatter *gui) {return addPage_globals(gui); };

		
		//methods to update the GUI fields
		void updateCard_attack_global(void); 
		void updateCard_release_global(void);
		void updateCard_scaleFac_global(void);
		void updateCard_attack(int i); 
		void updateCard_release(int i); 
		void updateCard_scaleFac(int i); 
		void updateCard_expCR(int i); 
		void updateCard_expKnee(int i); 
		void updateCard_linGain(int i); 
		void updateCard_compRat(int i); 
		void updateCard_compKnee(int i); 
		void updateCard_limKnee(int i); 
			
		bool flag_send_global_attack = false;
		bool flag_send_global_release = false;
		bool flag_send_global_scaleFac = false;
		bool flag_send_perBand_attack = false;	
		bool flag_send_perBand_release = false;		
		bool flag_send_perBand_scaleFac = false;		
		bool flag_send_perBand_expCR = false;		
		bool flag_send_perBand_expKnee = false;		
		bool flag_send_perBand_linGain = false;		
		bool flag_send_perBand_compRat = false;		
		bool flag_send_perBand_compKnee = false;		
		bool flag_send_perBand_limKnee = false;		
		
		// //////////////////////// Persistent mode...more compact display where the user chooses which parameters to edit on that page
		TR_Card* addCard_chooseMode(     TR_Page *page_h);
		TR_Card* addCard_persist_perChan(TR_Page *page_h);
		TR_Page* addPage_persist_perChan(TympanRemoteFormatter *gui);
		bool flag_send_persistent_chooseMode = false;
		bool flag_send_persistent_multiChan = false;
		void updateCard_persistentChooseMode(bool activeButtonsOnly = false);
		void updateCard_persist_perChan(int i);
		void updateCard_persist_perChan_all(bool activeButtonsOnly = false);
		void updateCard_persist_perChan_title(void);
		
		//overall default page, if sometone just blindly calls addPage_default();
		TR_Page* addPage_default(TympanRemoteFormatter *gui) {return addPage_persist_perChan(gui); };

		
		
		//here are the factors to use to increment different AudioEffectCompWDRC_F32 parameters
		float time_incr_fac = pow(2.0,1.0/4.0);
		float cr_fac = 0.1;
		float knee_fac = 2.0;
		float gain_fac = 2.0;

		
		//buttons for updating the App's GUI
		void sendGlobals(void);
		void sendLinearGain(int Ichan=-1); //empty (which means -1) sends all the values
		
		String name_for_UI = "WDRC Compressor Bank";  //used for printHelp()
		
	protected:
		//void printChanMsg(int direction);   //used for building the help menu.  direction is +1 for raising and -1 for lowering
		int findChan(char c, int direction=1); //used for interpreting in-coming commands

		//characters that map to channel number...these now live in SerialManager_UI.h
		//#define COMPBANK_N_CHARMAP (10+26+1)
		//const int n_charMap = COMPBANK_N_CHARMAP;
		//char charMapUp[COMPBANK_N_CHARMAP]   = "0123456789abcdefghijklmnopqrstuvwxyz"; //characters for raising
		//char charMapDown[COMPBANK_N_CHARMAP] = ")!@#$%^&*(ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //characters for lowering
		
		char persistCharTrigger = '-'; //used a channel number to command a change in the persistent mode state
		char persistDown = '<';
		char persistUp   = '>';
		char state_persistentMode = 'g'; //default to linear gain

};




#endif