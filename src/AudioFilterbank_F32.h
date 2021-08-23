/*
 * AudioFilterbank_F32
 * 
 * Created: Chip Audette (OpenAudio) Aug 2021
 * 
 * Purpose: Ceate filberbank classes that joins the filter design and filter implementation to
 *     make it easier to manage having multiple filters so that you don't have to handle each 
 *     individual filter yourself
 * 
 * MIT License.  Use at your own risk.
 * 
 */
 
#ifndef _AudioFilterbank_f32_h
#define _AudioFilterbank_f32_h

#include <Arduino.h>
#include <AudioStream_F32.h>
#include <AudioSettings_F32.h>
#include <AudioFilterFIR_F32.h> 		  //from Tympan_Library
#include <AudioConfigFIRFilterBank_F32.h> //from Tympan_Library
#include <AudioFilterBiquad_F32.h> 		  //from Tympan_Library
#include <AudioConfigIIRFilterBank_F32.h> //from Tympan_Library
#include <SerialManager_UI.h>			  //from Tympan_Library
#include <TympanRemoteFormatter.h> 		  //from Tympan_Library
#include <vector>

//define AudioFilterbank_MAX_NUM_FILTERS 8      //maximum number of filters to allow
#define AudioFilterbankBiquad_MAX_IIR_FILT_ORDER 6    //oveall desired filter order (note: in Matlab, an "N=3" bandpass is actually a 6th-order filter to be broken up into biquads
#define AudioFilterbankBiquad_COEFF_PER_BIQUAD  6     //3 "b" coefficients and 3 "a" coefficients per biquad

//This class helps manage some of the configuration and state information of the AudioFilterbank classes.
//By having this class, it tries to put everything in one place 
//It is also helpful for managing the GUI on the TympanRemote mobile App.
class AudioFilterbankState {
	public:
		AudioFilterbankState(void) { };
		~AudioFilterbankState(void) { };
		
		int filter_order = 6; 			//order of each filter being designed.  Should be overwritten!
		float sample_rate_Hz = 44100.f;	//sample rate used during filter design.  Should be overwritten!
		int audio_block_len = 128;		//audio block length sometimes used in initializing the filter states.  Should be overwritten!
		
		int set_crossover_freq_Hz(float *freq_Hz, int n_crossover);  //n_crossover is n_filters-1
		float get_crossover_freq_Hz(int Ichan);
		int get_crossover_freq_Hz(float *freq_Hz, int n_crossover); //n_crossover is n_filters-1

		int set_n_filters(int n);
		int get_n_filters(void) { return n_filters; }
		
				
		
		/// These functions are only for internal work by this class; these functions do not hold any 
		/// state or configuration information about the AudioFilterbank class.
		
		//keep track of the maximum number of filters...the user shouldn't have to worry about this
		int set_max_n_filters(int n);
		int get_max_n_filters(void) { return crossover_freq_Hz.size(); } //crossover_freq_Hz should be 1 larger than number of crossover freq
		

	protected:
		//int max_n_filters = 0;  //should correspond to the length of crossover_freq_Hz
		int n_filters = 0;      //should correspond to however many of the filters are actually being employed by the AuioFilterbank
		std::vector<float> crossover_freq_Hz;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
	
};

//This is a parent class for the FIR filterbank and IIR (biquad) filterbank.  This class defines an interfaces
//for other classes to interact with the filterbank (regardless of FIR and IIR) and this class holds some
//common methods that should only be defined in one place.
//
//Since this has some purely virtual methods, you cannot instantiate this class directly.
class AudioFilterbankBase_F32 : public AudioStream_F32 {
	public:
		AudioFilterbankBase_F32(void): AudioStream_F32(1,inputQueueArray) { } 
		AudioFilterbankBase_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) { }
		~AudioFilterbankBase_F32(void) { delete filter_coeff; }
		
		virtual void enable(bool _enable = true) { is_enabled = _enable; }
		
		//virtual void update(void) = 0;  //already required by AudioStream_F32 to be implemented in a child class
		virtual int set_n_filters(int val) = 0;  //must implement this in a child class
		virtual int set_max_n_filters(int n_chan) = 0; //must implement this in a child class
		virtual int designFilters(int n_chan, int n_order, float sample_rate_Hz, int block_len, float *crossover_freq) = 0;  //must implement this in a child class
		
		int increment_crossover_freq(int Ichan, float freq_increment_fac); //nudge of the frequencies, which might nudge others if they're too close...and update the filter design
		virtual int get_n_filters(void) { return state.get_n_filters(); }
		
		AudioFilterbankState state;
		
	protected: 
		audio_block_f32_t *inputQueueArray[1];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
		//int n_filters = AudioFilterbank_MAX_NUM_FILTERS; //how many filters are actually being used.  Must be less than AudioFilterbank_MAX_NUM_FILTERS
		float min_freq_seperation_fac = powf(2.0f,1.0f/6.0f);  //minimum seperation of the filter crossover frequencies
		
		//helper functions
		static int enforce_minimum_spacing_of_crossover_freqs(float *freqs_Hz, int n_crossover, float min_seperation_fac,  int direction = 1); //direction = 1 to move unacceptable freqs higher, -1 to move them lower
		static void sortFrequencies(float *freq_Hz, int n_filts);
		
		float *filter_coeff;
		float n_coeff_allocated = 0;
};


//This is the FIR based filterbank.
//You should instantiate this class if you want FIR filters.
class AudioFilterbankFIR_F32 : public AudioFilterbankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_FIR
	public:
		AudioFilterbankFIR_F32(void) : AudioFilterbankBase_F32() { }
		AudioFilterbankFIR_F32(const AudioSettings_F32 &settings) : AudioFilterbankBase_F32(settings) { }
		AudioFilterbankFIR_F32(const AudioSettings_F32 &settings, int n_chan) : AudioFilterbankBase_F32(settings) {
			set_max_n_filters(n_chan);
		}


		virtual void update(void);
		virtual int set_n_filters(int val);
		int set_max_n_filters(int val);
		virtual int designFilters(int n_chan, int n_fir, float sample_rate_Hz, int block_len, float *crossover_freq);

		//core classes for designing and implementing the filters
		AudioConfigFIRFilterBank_F32 filterbankDesigner;
		//AudioFilterFIR_F32 filters[AudioFilterbank_MAX_NUM_FILTERS]; //every filter instance consumes memory to hold its states, which are numerous for an FIR filter
		std::vector<AudioFilterFIR_F32> filters;
		
	private:

};

//This is the IIR based (specifically, IIR biquad [aka. second-order sections]) filterbank
//You should instantiate this class if you want Biquad filters.
class AudioFilterbankBiquad_F32 : public AudioFilterbankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_Biquad
	public:
		AudioFilterbankBiquad_F32(void) : AudioFilterbankBase_F32() { }
		AudioFilterbankBiquad_F32(const AudioSettings_F32 &settings) : AudioFilterbankBase_F32(settings) { }
		AudioFilterbankBiquad_F32(const AudioSettings_F32 &settings, int n_chan) : AudioFilterbankBase_F32(settings) { 
			set_max_n_filters(n_chan);
		}

		virtual void update(void);
		virtual int set_n_filters(int val);
		int set_max_n_filters(int val);
		virtual int designFilters(int n_chan, int n_iir, float sample_rate_Hz, int block_len, float *crossover_freq);

		//core classes for designing and implementing the filters
		AudioConfigIIRFilterBank_F32 filterbankDesigner;
		//AudioFilterBiquad_F32 filters[AudioFilterbank_MAX_NUM_FILTERS]; //every filter instance consumes memory to hold its states, which are numerous for an FIR filter
		std::vector<AudioFilterBiquad_F32> filters;
		
	private:

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
class AudioFilterbank_UI : public SerialManager_UI {
	public:
		//AudioFilterbank_UI(void) : SerialManager_UI() {};
		AudioFilterbank_UI(AudioFilterbankBase_F32 *_this_filterbank) : 
			SerialManager_UI(), this_filterbank(_this_filterbank) {
				freq_id_str = String(getIDchar()) + freq_id_str; //prepend with a unique character to this instance
			};
		
		
		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false); 

		float freq_increment_fac = powf(2.0,1.0/12.0);  //how much to multiply the crossover frequency by when shifting up or down
		void printCrossoverFreqs(void);

		//create the button sets for the TympanRemote's GUI
		TR_Card *addCard_crossoverFreqs(TR_Page *page_h);
		TR_Page *addPage_crossoverFreqs(TympanRemoteFormatter *gui);
		TR_Page *addPage_default(TympanRemoteFormatter *gui) {return addPage_crossoverFreqs(gui); };
		
		//buttons for updating the App's GUI
		void sendAllFreqs(void);
		void sendOneFreq(int Ichan);
		
		String name_for_UI = "Filterbank";  //used for printHelp()
				
	protected:
		AudioFilterbankBase_F32 *this_filterbank = NULL;
		void printChanMsg(int direction);   //used for building the help menu.  direction is +1 for raising and -1 for lowering
		int findChan(char c, int direction); //used for interpreting in-coming commands

		//characters that map to channel number
		#define N_CHARMAP (8+26+1)
		const int n_charMap = N_CHARMAP;
		char charMapUp[N_CHARMAP]   = "12345678abcdefghijklmnopqrstuvwxyz"; //characters for raising the frequencies
		char charMapDown[N_CHARMAP] = "!@#$%^&*ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //characters for lowering the frequencies

		//GUI names and whatnot
		String freq_id_str = String("cfreq");

};

//FIR filterbank with built-in UI support
class AudioFilterbankFIR_F32_UI : public AudioFilterbankFIR_F32, public AudioFilterbank_UI {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_FIR_UI	
	public:
		AudioFilterbankFIR_F32_UI(void) : AudioFilterbankFIR_F32(), AudioFilterbank_UI(this) {}
		AudioFilterbankFIR_F32_UI(const AudioSettings_F32 &settings) : AudioFilterbankFIR_F32(settings), AudioFilterbank_UI(this) {};
		

};

//IIR (Biquad) filterbank with built-in UI support
class AudioFilterbankBiquad_F32_UI : public AudioFilterbankBiquad_F32, public AudioFilterbank_UI {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_Biquad_UI
	public:
		AudioFilterbankBiquad_F32_UI(void) : AudioFilterbankBiquad_F32(), AudioFilterbank_UI(this) {}
		AudioFilterbankBiquad_F32_UI(const AudioSettings_F32 &settings) : AudioFilterbankBiquad_F32(settings), AudioFilterbank_UI(this) {};
			
};


#endif