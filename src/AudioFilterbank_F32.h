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
#include <AudioFilterFIR_F32.h> 		  //from Tympan_Library
#include <AudioConfigFIRFilterBank_F32.h> //from Tympan_Library
#include <AudioFilterBiquad_F32.h> 		  //from Tympan_Library
#include <AudioConfigIIRFilterBank_F32.h> //from Tympan_Library

#define AudioFilterbankFIR_MAX_NUM_FILTERS 8       //maximum number of filters to allow
#define AudioFilterbankBiquad_MAX_NUM_FILTERS 12      //maximum number of filters to allow
#define AudioFilterbankBiquad_MAX_IIR_FILT_ORDER 6    //oveall desired filter order (note: in Matlab, an "N=3" bandpass is actually a 6th-order filter to be broken up into biquads
#define AudioFilterbankBiquad_COEFF_PER_BIQUAD  6     //3 "b" coefficients and 3 "a" coefficients per biquad

class AudioFilterbankState {
	public:
		AudioFilterbankState(void) {};
		~AudioFilterbankState(void) { delete crossover_freq_Hz; }
		
		int filter_order = 0;
		
		int set_crossover_freq_Hz(float *freq_Hz, int n_filts) {
			//make sure that we have valid input
			if (n_filts < 0) return -1;  //-1 is error
			
			//make sure that we have space
			if (n_filts > max_n_filters) {   //allocate more space
				int ret_val = set_max_n_filters(n_filts);
				if (ret_val < 0) return ret_val; //return if it returned an error
			}
			
			//if the number of filters is greater than zero, copy the freuqencies.
			if (n_filts==0) return 0;  //this is OK
			for (int Ichan=0;Ichan < (n_filts-1); Ichan++) crossover_freq_Hz[Ichan] = freq_Hz[Ichan];  //n-1 because there will always be one less crossover frequency than filter
			return set_n_filters(n_filts); //zeros is OK	
		}
		float get_crossover_freq_Hz(int Ichan) { 
			if (Ichan < n_filters-1) {  //there will always be one less crossover frequency than filters
				return crossover_freq_Hz[Ichan]; 
			} 
			return 0.0f;
		}

		int set_n_filters(int n) {
			if ((n < 0) && (n > max_n_filters)) return -1; //this is an error
			n_filters = n;
			return n_filters; 
		}
		int get_n_filters(void) { return n_filters; }

	private:
		int max_n_filters = 0;  //should correspond to the length of crossover_freq_Hz
		int n_filters = 0;      //should correspond to however many of the filters are actually being employed by the AuioFilterbank
		float *crossover_freq_Hz;  //this really only needs to be [nfilters-1] in length, but we'll generally allocate [nfilters] just to avoid mistaken overruns
		
		//keep track of the maximum number of filters
		int set_max_n_filters(int n) { 
			if ((n < 0) || (n > 64)) return -1; //-1 is an error
			if (crossover_freq_Hz != NULL) delete crossover_freq_Hz; 
			max_n_filters = 0; n_filters = 0;
			if (n==0) return 0;  //zero is OK
			crossover_freq_Hz = new float[n];
			if (crossover_freq_Hz == NULL) return -1; //-1 is an error
			max_n_filters = n;
			return 0;  //zero is OK;
		}
		int get_max_n_filters(void) { return max_n_filters; }
		
};


class AudioFilterbankBase_F32 : public AudioStream_F32 {
	public:
		AudioFilterbankBase_F32(void): AudioStream_F32(1,inputQueueArray) { } 
		AudioFilterbankBase_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) { }
		
		virtual void enable(bool _enable = true) { is_enabled = _enable; }
		
		//virtual void begin(void) = 0;  //must implement this in a child class
		virtual void update(void) = 0;  //must implement this in a child class;
		virtual int set_n_filters(int val) = 0;  //must implement this in a child class
		virtual int designFilters(int n_chan, int n_order, float sample_rate_Hz, int block_len, float *crossover_freq) = 0;  //must implement this in a child class
		
		virtual int get_n_filters(void) { return n_filters; }
		
		AudioFilterbankState state;
		
	protected: 
		audio_block_f32_t *inputQueueArray[1];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
		int n_filters = AudioFilterbankFIR_MAX_NUM_FILTERS; //how many filters are actually being used.  Must be less than AudioFilterbankFIR_MAX_NUM_FILTERS

};

class AudioFilterbankFIR_F32 : public AudioFilterbankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_FIR
	public:
		AudioFilterbankFIR_F32(void) : AudioFilterbankBase_F32() { }
		AudioFilterbankFIR_F32(const AudioSettings_F32 &settings) : AudioFilterbankBase_F32(settings) { }

		//virtual void begin(void);
		virtual void update(void);
		virtual int set_n_filters(int val);
		virtual int designFilters(int n_chan, int n_fir, float sample_rate_Hz, int block_len, float *crossover_freq);

		//core classes for designing and implementing the filters
		AudioConfigFIRFilterBank_F32 filterbankDesigner;
		AudioFilterFIR_F32 filters[AudioFilterbankFIR_MAX_NUM_FILTERS]; //every filter instance consumes memory to hold its states, which are numerous for an FIR filter
		
	private:

};

class AudioFilterbankBiquad_F32 : public AudioFilterbankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_Biquad
	public:
		AudioFilterbankBiquad_F32(void) : AudioFilterbankBase_F32() { }
		AudioFilterbankBiquad_F32(const AudioSettings_F32 &settings) : AudioFilterbankBase_F32(settings) { }

		//virtual void begin(void);
		virtual void update(void);
		virtual int set_n_filters(int val);
		virtual int designFilters(int n_chan, int n_iir, float sample_rate_Hz, int block_len, float *crossover_freq);

		//core classes for designing and implementing the filters
		AudioConfigIIRFilterBank_F32 filterbankDesigner;
		AudioFilterBiquad_F32 filters[AudioFilterbankBiquad_MAX_NUM_FILTERS]; //every filter instance consumes memory to hold its states, which are numerous for an FIR filter
		
	private:

};

#endif