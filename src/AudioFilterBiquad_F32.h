/*
 * AudioFilterBiquad_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 *
 * License: MIT License.  Use at your own risk.
 * 
 */

#ifndef _filter_iir_f32
#define _filter_iir_f32

#include <Arduino.h>
#include <arm_math.h>
#include "AudioStream_F32.h"
#include "SerialManager_UI.h"


#include <SdFat.h>     //this was added in Teensyduino 1.54beta3
#include <SDWriter.h>  //to get macro definition of SD_CONFIG   //in Tympan_Library
#include <AccessConfigDataOnSD.h>   // in Tympan_Library
#include <BTNRH_WDRC_Types.h>  //in Tympan_Library


// One way to indicate that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define IIR_F32_PASSTHRU ((const float32_t *) 1)

#define IIR_MAX_STAGES 4  //meaningless right now

class AudioFilterBiquad_F32_settings {
	public:
		int cur_type_ind = 0;   //see BiquadFiltType enum
		int is_bypassed = 1;	//set to 1 to bypass this filter
		float cutoff_Hz = 4000.0;
		float q = 1.0;
		
		void printAllValues(void) { printAllValues(&Serial); }	
		void printAllValues(Stream *s) {
			s->println("AudioFilterBiquad_F32_settings:");
			s->print("    : Filter type = "); s->println(cur_type_ind);
			s->print("    : Is bypassed = "); s->println(is_bypassed);
			s->print("    : Cutoff (Hz) = "); s->println(cutoff_Hz);
			s->print("    : Filter Q = "); s->println(q);
		};		
	protected:	
};


class AudioFilterBase_F32 : public AudioStream_F32 {
	public:
		AudioFilterBase_F32(void) : AudioStream_F32(1,inputQueueArray) {};
		AudioFilterBase_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) {};

		virtual int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new) = 0;
		
		virtual bool enable(bool enable = true) { 
			if (enable == true) {
				//if (is_armed) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				//}
			}
			is_enabled = false;
			return get_is_enabled();
		}	
		virtual bool bypass(bool _bypass = true) {return is_bypassed = _bypass; }
		virtual bool get_is_enabled(void) { return is_enabled; }
		virtual bool get_is_bypassed(void) { return is_bypassed; }
	protected:
	    audio_block_f32_t *inputQueueArray[1];
		bool is_enabled = false;  //this turns off the algorithm and has it output nothing 
		bool is_bypassed = false;   //this turns off the aglorithm but has it passthrough the input data to the output

};

class AudioFilterBiquad_F32 : public AudioFilterBase_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName:IIR
  public:
    AudioFilterBiquad_F32(void): AudioFilterBase_F32(), coeff_p(IIR_F32_PASSTHRU) { 
		setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		clearCoeffArray();		
	}
	AudioFilterBiquad_F32(const AudioSettings_F32 &settings): AudioFilterBase_F32(settings), coeff_p(IIR_F32_PASSTHRU) {
			setSampleRate_Hz(settings.sample_rate_Hz); 
	}

    virtual void begin(const float32_t *cp, int n_stages = 1) {
      coeff_p = cp;
      // Initialize Biquad instance (ARM DSP Math Library)
      if (coeff_p && (coeff_p != IIR_F32_PASSTHRU) && n_stages <= IIR_MAX_STAGES) {
        //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html
        arm_biquad_cascade_df1_init_f32(&iir_inst, n_stages, (float32_t *)coeff_p,  &StateF32[0]);
		is_armed = true;enable(true);
      }
    }
    virtual void end(void) {
      coeff_p = NULL;
	  enable(false);
    }
	
	virtual void clearCoeffArray(void) {
		for (int i=0; i<IIR_MAX_STAGES*5;i++) coeff[i]=0.0;
		coeff[0]=1.0f;  //makes this be a simple pass-thru
	}
	
	virtual float getSampleRate_Hz(void) { return sampleRate_Hz; }
	virtual float setSampleRate_Hz(float32_t _fs_Hz) { return sampleRate_Hz = _fs_Hz; }
    
    virtual void setBlockDC(void) {
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff for HP at 40Hz: [b,a]=butter(2,40/(44100/2),'high'); %assumes fs_Hz = 44100
      float32_t b[] = {8.173653471988667e-01,  -1.634730694397733e+00,   8.173653471988667e-01};  //from Matlab
      float32_t a[] = { 1.000000000000000e+00,   -1.601092394183619e+00,  6.683689946118476e-01};  //from Matlab
      setFilterCoeff_Matlab(b, a);
    }
    
    virtual void setFilterCoeff_Matlab(float32_t b[], float32_t a[]) { //one stage of N=2 IIR
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff, such as: [b,a]=butter(2,20/(44100/2),'high'); %assumes fs_Hz = 44100
      coeff[0] = b[0];   coeff[1] = b[1];  coeff[2] = b[2]; //here are the matlab "b" coefficients
      coeff[3] = -a[1];  coeff[4] = -a[2];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
      begin(coeff,1);
    }

    virtual void setFilterCoeff_Matlab_sos(float32_t sos[], int n_sos) { //n_sos second-order sections of IIR
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff, such as: 
	  //   fs_Hz = 44100;  %sample rate of the signal to be processed
	  //   N_IIR = 3;      %order of the IIR filter 
	  //   bp_Hz = [1000 2000];    %define the desired cutoff frequencies [low, high] of the bandpass filter in Hz
	  //   [b,a]=butter(N_IIR,bp_Hz/(fs_Hz/2)));  %creates bandpass filter, but not yet a second-order sections
	  //   [sos]=tf2sos(b,a);  %convert to second order section  (no gain term...try to add that into this code later)
	  int start_ind;
	  for (int i=0; i < min(n_sos,IIR_MAX_STAGES); i++) {
		  start_ind = i*5;
		  coeff[start_ind+0] = sos[i*6+0];   
		  coeff[start_ind+1] = sos[i*6+1];  
		  coeff[start_ind+2] = sos[i*6+2];  
		  //sos[i][3];  //the DSP data structure skips over this because it should because should be 1.0 (ie, it is a[0])
		  coeff[start_ind+3] = -sos[i*6+4];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
		  coeff[start_ind+4] = -sos[i*6+5];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
	  }
      begin(coeff,n_sos);
    }
	
	
	// //////////////////////// From Audio EQ Cookbook
	
	//This setCoefficients method sets the coefficients given the equations below from the AudioEQ Cookbook
	//note: stage is currently ignored
	virtual void setCoefficients(int stage, float32_t c[]) {
		if (stage > 0) {
			if (Serial) {
				Serial.println(F("AudioFilterBiquad_F32: setCoefficients: *** ERROR ***"));
				Serial.print(F("    : This module only accepts one stage."));
				Serial.print(F("    : You are attempting to set stage "));Serial.print(stage);
				Serial.print(F("    : Ignoring this filter."));
			}
			return;
		}
		coeff[0] = c[0];
		coeff[1] = c[1];
		coeff[2] = c[2];
		coeff[3] = -c[3];  //notice the sign flip!  from Matlab convention to ARM convention
		coeff[4] = -c[4]; //notice the sign flip!  from Matlab convention to ARM convention
		begin(coeff);
		cur_filt_stage = stage;
	}
	
	// Compute common filter functions...all second order filters...all with Matlab convention on a1 and a2 coefficients
	// http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	void calcLowpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcHighpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcBandpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcNotch(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcLowShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *c);
	void calcHighShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *c);
	
	//set the filter coefficients without the caller having to explicitly handle the coefficients
	void setLowpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcLowpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setHighpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcHighpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setBandpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcBandpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setNotch(uint32_t stage, float32_t freq_Hz, float32_t q = 10.0) {
		calcNotch(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setLowShelf(uint32_t stage, float32_t freq_Hz, float32_t gain, float32_t slope = 1.0f) {
		calcLowShelf(freq_Hz, gain, slope, coeff);
		setCoefficients(stage,coeff);
	}
	void setHighShelf(uint32_t stage, float32_t freq_Hz, float32_t gain, float32_t slope = 1.0f) {
		calcHighShelf(freq_Hz, gain, slope, coeff);
		setCoefficients(stage,coeff);
	}
	
	float increment_crossover_freq(float incr_fac);
	float increment_filter_q(float incr_fac);
    
    virtual void update(void);
	virtual int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new);
	virtual float getCutoffFrequency_Hz(void) { return cutoff_Hz; }
	virtual float getQ(void) { return q; }
	virtual float getBW_Hz(void);
	
	bool enable(bool enable = true) { 
		if (enable == true) {
			if (is_armed) {  //don't allow it to enable if it can't actually run the filters
				is_enabled = enable;
				return get_is_enabled();
			}
		}
		is_enabled = false;  //see parent class
		return get_is_enabled();
	}
	//bool get_is_enabled(void) { return is_enabled; }

	virtual void setupFromSettings(AudioFilterBiquad_F32_settings &state); //loads values from "state"
	virtual void getSettings(AudioFilterBiquad_F32_settings *settings);       //puts result into "state"

	enum BiquadFiltType {NONE=0, LOWPASS, BANDPASS, HIGHPASS, NOTCH, LOWSHELF, HIGHSHELF};
	String getCurFilterTypeString(void);
   
  protected:
	bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
    float32_t coeff[5 * IIR_MAX_STAGES]; //no filtering. actual filter coeff set later
	float32_t sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT; //default.  from AudioStream.h??
	float32_t cutoff_Hz = -999;
	float32_t q = -1;
	int cur_type_ind = -1;
	int cur_filt_stage = 0;
	float cur_gain_for_shelf = 1.0;
  
	//functions with no bounds checking
	virtual int redesignGivenCutoffAndQ(float new_freq_Hz, float new_Q);
	virtual int redesignGivenCutoffAndQ(int filt_type, float new_freq_Hz, float new_Q);
  
    // pointer to current coefficients or NULL or FIR_PASSTHRU
    const float32_t *coeff_p;
  
    // ARM DSP Math library filter instance
    arm_biquad_casd_df1_inst_f32 iir_inst;
    float32_t StateF32[4*IIR_MAX_STAGES];
};



// ///////////////////////////////////////////////////////////////////////////////////////////////
//
// State management to help with handling presets
//
// ///////////////////////////////////////////////////////////////////////////////////////////////

class AudioFilterBiquad_F32_settings_SD : public AudioFilterBiquad_F32_settings, public Preset_SD_Base {  //look in Preset_SD_Base and in AccessConfigDataOnSD.h for more info on the methods and functions used here
	public: 
		AudioFilterBiquad_F32_settings_SD() : AudioFilterBiquad_F32_settings(), Preset_SD_Base() {};
		AudioFilterBiquad_F32_settings_SD(const AudioFilterBiquad_F32_settings &state) : AudioFilterBiquad_F32_settings(state), Preset_SD_Base() {};
		using Preset_SD_Base::readFromSDFile; //I don't really understand why these are necessary
		using Preset_SD_Base::readFromSD;
		using Preset_SD_Base::printToSD;
		
		virtual int readFromSDFile(SdFile *file, const String &var_name);
		virtual int readFromSD(SdFat &sd, String &filename_str, const String &var_name);
		void printToSDFile(SdFile *file, const String &var_name);
		int printToSD(SdFat &sd, String &filename_str, const String &var_name, bool deleteExisting);
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

class AudioFilterBiquad_F32_UI : public AudioFilterBiquad_F32, public SerialManager_UI {
	public:
		//AudioFilterbank_UI(void) : SerialManager_UI() {};
		AudioFilterBiquad_F32_UI(void) : AudioFilterBiquad_F32(), SerialManager_UI() {};
		AudioFilterBiquad_F32_UI(const AudioSettings_F32 &settings) : AudioFilterBiquad_F32(settings), SerialManager_UI() {}
		
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false); 


		float freq_increment_fac = powf(2.0,1.0/12.0);  //how much to multiply the crossover frequency by when shifting up or down
		float q_increment_fac = powf(2.0,1.0/3.0);
		//void printCutoffFreq(void);

		//create the button sets for the TympanRemote's GUI
		TR_Card *addCard_cutoffFreq(TR_Page *page_h);
		TR_Card *addCard_filterQ(TR_Page *page_h);
		TR_Card *addCard_filterBW(TR_Page *page_h);
		TR_Card *addCard_filterBypass(TR_Page *page_h);
		TR_Page *addPage_CutoffAndQ(TympanRemoteFormatter *gui);
		TR_Page *addPage_CutoffAndQAndBW(TympanRemoteFormatter *gui);;

		TR_Page *addPage_default(TympanRemoteFormatter *gui) {return addPage_CutoffAndQ(gui); };

		void updateGUI_cutoff(bool activeButtonsOnly=false);
		void updateGUI_Q(bool activeButtonsOnly=false);
		void updateGUI_bypass(bool activeButtonsOnly=false);

				
		String name_for_UI = "Biquad Filter";     //used for App and printHelp()
		String getCurFilterFrequencyNameString(void);  //get proper name for frequency ("cutoff", "center", etc) for the current filter type
				
	protected:
		
		//characters for controlling cutoff
		const int n_charMap = 1;  //only 1 character to map (unlike in AudioFilterbank, which has many)
		char charMapUp[1+1]   = "1"; //characters for raising the frequencies (the extra +1 is for the terminating NULL)
		char charMapDown[1+1] = "!"; //characters for lowering the frequencies (the extra +1 is for the terminating NULL)

		//names for buttons and fields in the GUI...this is not the text shown on the button or fields but is the
		//behind-the-scenes names for the items so that we can tell the GUI which specific item we want to update
		String freq_id_str = String("cfreq");
		String q_id_str = String("q");
		String BW_id_str = String("bw");
		String passthru_id_str = String("byp");
		String normal_id_str = String("norm");
};



#endif


