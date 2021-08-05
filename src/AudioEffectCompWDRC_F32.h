/*
 * AudioEffectCompWDRC_F32: Wide Dynamic Rnage Compressor
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Derived From: WDRC_circuit from CHAPRO from BTNRC: https://github.com/BTNRH/chapro
 *     As of Feb 2017, CHAPRO license is listed as "Creative Commons?"
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _AudioEffectCompWDRC_F32
#define _AudioEffectCompWDRC_F32

class AudioCalcGainWDRC_F32;  //forward declared.  Actually defined in later header file, but I need this here to avoid circularity

#include <Arduino.h>				//from Arduino installation
#include "AudioStream_F32.h"		//from Tympan_Library
#include <arm_math.h>				//Should be included with Teensyduino installation
#include "AudioCalcEnvelope_F32.h"	//from Tympan_Library
#include "AudioCalcGainWDRC_F32.h"  //has definition of CHA_WDRC
#include "BTNRH_WDRC_Types.h"		//from Tympan_Library
#include <SerialManager_UI.h>       //from Tympan_Library


//Define an interface for the WDRC compressor for its state-tracking class to use. This 
//avoids the problem of the state-tracking class being dependent upon the definition of the
//WDRC class while the WDRC class also being dependent upon the definition of the state-
//tracking class.  For some reason, a forward declaration didn't work so I'll do this instead.
//
//Here are all the WDRC get() methods that will help us keep track of the WDRC configuration
class WDRCinterface {
	public:
		virtual float getSampleRate_Hz(void) =0;
		virtual float getAttack_msec(void) =0;
		virtual float getRelease_msec(void) =0;
		virtual float getMaxdB(void) =0;  //used by getScaleFactor_dBSPL_at_dBFS()
		virtual float getExpansionCompRatio(void) =0;
		virtual float getKneeExpansion_dBSPL(void)=0;
		virtual float getGain_dB(void) =0; //used by getLinearGain_dB()
		virtual float getCompRatio(void) =0;
		virtual float getKneeCompressor_dBSPL(void) =0;
		virtual float getKneeLimiter_dBSPL(void) =0;
};


//This class helps manage some of the configuration and state information of the AudioEffectCompWDRC_F32
//class.  By having this class, it tries to put everything in one place.  It is also helpful for
//managing the GUI on the TympanRemote mobile App.
class AudioCompWDRCState {
	public:
		AudioCompWDRCState(void) { };
		
		//The compressor is tricky because it has many configuration parameters that I'd like to track here in
		//this State-tracking class because I want to access them via the App's GUI.  But, if I keep a local copy
		//of the parameter values, I need to do a lot of management to ensure that they are in-sync with the underlying
		//audio processing classes.  
		//
		//So, instead, I'm going to just get the values from the classes themsevles.
		//
		//This means that, instead of having a bunch of state variables (int, float, whatever) here in this
		//state-tracking class, I'll have a bunch of get() methods.  I could have just asked for these values
		//directly from the AudioEffectCompWDRC_F32 class without have introducting this state-tracking class
		//as an intermediary.  I chose to introduce this class because *all* of the UI-enabled classes use some
		//sort of state-tracking class.  Also, by putting all these critical get() methods in one place here 
		//in the State, it is very clear which are the most important parameters, without getting distracted by
		//all of the other methods in the main class.
	
		//get parameter values from the compressors
		float getSampleRate_Hz(void) { return compressor->getSampleRate_Hz(); }
		float getAttack_msec(void) { return compressor->getAttack_msec(); };
		float getRelease_msec(void) { return compressor->getRelease_msec(); };
		float getScaleFactor_dBSPL_at_dBFS(void) { return compressor->getMaxdB(); };
		float getExpansionCompRatio(void) { return compressor->getExpansionCompRatio(); };
		float getKneeExpansion_dBSPL(void) { return compressor->getKneeExpansion_dBSPL(); };
		float getLinearGain_dB(void) { return compressor->getGain_dB(); };
		float getCompRatio(void) { return compressor->getCompRatio(); };
		float getKneeCompressor_dBSPL(void) { return compressor->getKneeCompressor_dBSPL(); };
		float getKneeLimiter_dBSPL(void) { return compressor->getKneeLimiter_dBSPL(); };

		//These methods are not used to directly maintain the state of the AudioEffectCompWDRC.
		//They are supporting methods
		void setCompressor(WDRCinterface *c) { compressor = c; }  //get pointer


	protected:
		WDRCinterface *compressor;  //will be an array of pointers to our compressors
};

class AudioEffectCompWDRC_F32 : public AudioStream_F32, WDRCinterface
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName: CompressWDRC
  public:
    AudioEffectCompWDRC_F32(void): AudioStream_F32(1,inputQueueArray), WDRCinterface() { //need to modify this for user to set sample rate
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);  //use the default sample rate from the Teensy Audio library
      setDefaultValues();
    }

    AudioEffectCompWDRC_F32(AudioSettings_F32 settings): AudioStream_F32(1,inputQueueArray), WDRCinterface() { //need to modify this for user to set sample rate
      setSampleRate_Hz(settings.sample_rate_Hz);
      setDefaultValues();
    }

	//initialize with the default values
	void setDefaultValues(void);

	
	// ////////////////////////// These are the methods where the audio processing work gets done

    //here is the method that is called automatically by the audio library
    void update(void);

	//here is a standard method for executing the guts of the algorithm without having to call update()
	//This is the access point used by the compressor bank class, for example, since the compressor bank
	//handles the audio_block manipulation normally done by update()
	//
	//This method uses audio_block_f32_t as its inputs and outputs, to be consistent with all the other
	//"processAudioBlock()" methods that are used in many other of my audio-processing classes
	int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block);

    //Here is the function that actually does all the work
	//This method uses simply float arrays as the inptus and outputs, so that this is maximally compatible
	//with other ways of using this class.
     void compress(float *x, float *y, int n);

	// ///////////////////////// These are the methods used to configure or otherwise interact with this class

    float setSampleRate_Hz(const float _fs_Hz) {return calcEnvelope.setSampleRate_Hz(_fs_Hz); }
	float getSampleRate_Hz(void) { return calcEnvelope.getSampleRate_Hz(); }

    //set all of the parameters for the compressor using the CHA_WDRC "GHA" structure
	void configureFromGHA(float fs_Hz, const BTNRH_WDRC::CHA_WDRC &gha) { setSampleRate_Hz(fs_Hz);  setParams_from_CHA_WDRC(&gha); }
	void configureFromGHA(const BTNRH_WDRC::CHA_WDRC &gha) { setParams_from_CHA_WDRC(&gha); }  //assumes that the sample rate has already been set!!!
    void setParams_from_CHA_WDRC(const BTNRH_WDRC::CHA_WDRC *gha);
	
    //set all of the user parameters for the compressor...assuming no expansion regime
    //assumes that the sample rate has already been set!!!
	void setParams(float attack_ms, float release_ms, float maxdB, float tkgain, float comp_ratio, float tk, float bolt);

    //set all of the user parameters for the compressor...assumes that there is an expansion regime
    //assumes that the sample rate has already been set!!!
    void setParams(float attack_ms, float release_ms, float maxdB, float exp_cr, float exp_end_knee, float tkgain, float comp_ratio, float tk, float bolt);


    //set, increment, or get the linear gain of the system
    float setGain_dB(float linear_gain_dB) { return calcGain.setGain_dB(linear_gain_dB); }
    float incrementGain_dB(float increment_dB) { return calcGain.incrementGain_dB(increment_dB); }    
    float getGain_dB(void) { return calcGain.getGain_dB(); }
	float getCurrentGain_dB(void) { return calcGain.getCurrentGain_dB(); }
    float getCurrentLevel_dB(void) { return AudioCalcGainWDRC_F32::db2(calcEnvelope.getCurrentLevel()); }  //this is 20*log10(abs(signal)) after the envelope smoothing
	
	//set or get the other parameters
	void setAttackRelease_msec(float32_t attack_ms, float32_t release_ms) {
		calcEnvelope.setAttackRelease_msec(attack_ms, release_ms);
	}
	float setAttack_msec(float attack_ms) { return calcEnvelope.setAttack_msec(attack_ms); }
	float getAttack_msec(void) { return calcEnvelope.getAttack_msec(); }
	float setRelease_msec(float release_ms) { return calcEnvelope.setRelease_msec(release_ms); }
	float getRelease_msec(void) { return calcEnvelope.getRelease_msec(); }
	float setMaxdB(float32_t foo) { return calcGain.setMaxdB(foo); }
	float getMaxdB(void) { return calcGain.getMaxdB(); }
	float setKneeExpansion_dBSPL(float32_t _knee) { return calcGain.setKneeExpansion_dBSPL(_knee); }
	float getKneeExpansion_dBSPL(void) { return calcGain.getKneeExpansion_dBSPL(); }
	float setExpansionCompRatio(float32_t _cr) { return calcGain.setExpansionCompRatio(_cr); }
	float getExpansionCompRatio(void) { return calcGain.getExpansionCompRatio(); }
	float setKneeCompressor_dBSPL(float32_t foo) { return calcGain.setKneeCompressor_dBSPL(foo); }
	float getKneeCompressor_dBSPL(void) { return calcGain.getKneeCompressor_dBSPL(); }
	float setCompRatio(float32_t foo) { return calcGain.setCompRatio(foo); }
	float getCompRatio(void) { return calcGain.getCompRatio(); }
	float setKneeLimiter_dBSPL(float32_t foo) { return calcGain.setKneeLimiter_dBSPL(foo); }
	float getKneeLimiter_dBSPL(void) { return calcGain.getKneeLimiter_dBSPL(); }
	
	
	// /////////////////////////////////////////////////  Here are the public data members
    AudioCalcEnvelope_F32 calcEnvelope;
    AudioCalcGainWDRC_F32 calcGain;
	AudioCompWDRCState state;

  private:
    audio_block_f32_t *inputQueueArray[1];
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


//This is the base class to be inherited by the FIR and Biquad versions
class AudioEffectCompWDRC_F32_UI : public AudioEffectCompWDRC_F32, SerialManager_UI {
	public:
		//AudioFilterbank_UI(void) : SerialManager_UI() {};
		AudioEffectCompWDRC_F32_UI(void) : 	AudioEffectCompWDRC_F32(), SerialManager_UI() {	};
		
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void) {};
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false)  {}; 
	
	/*
		//create the button sets for the TympanRemote's GUI
		TR_Card *addCard_crossoverFreqs(TR_Page *page_h);
		TR_Page *addPage_crossoverFreqs(TympanRemoteFormatter *gui);
		TR_Page *addPage_default(TympanRemoteFormatter *gui) {return addPage_crossoverFreqs(gui); };
		
*/
		
	protected:

		//GUI names and whatnot
		//String freq_id_str = String("cfreq");
		
};


#endif
    

