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

class AudioFilterBankBase_F32 : public AudioStream_F32 {
	public:
		AudioFilterBankBase_F32(void): AudioStream_F32(1,inputQueueArray) { } 
		AudioFilterBankBase_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1,inputQueueArray) { }
		
		virtual void enable(bool _enable = true) { is_enabled = _enable; }
		
		//virtual void begin(void) = 0;  //must implement this in a child class
		virtual void update(void) = 0;  //must implement this in a child class;
		virtual int set_n_filters(int val) = 0;  //must implement this in a child class
		virtual int designFilters(int n_chan, int n_order, float sample_rate_Hz, int block_len, float *crossover_freq) = 0;  //must implement this in a child class
		
		virtual int get_n_filters(void) { return n_filters; }
		
	protected: 
		audio_block_f32_t *inputQueueArray[1];  //required as part of AudioStream_F32.  One input.
		bool is_enabled = false;
		int n_filters = AudioFilterbankFIR_MAX_NUM_FILTERS; //how many filters are actually being used.  Must be less than AudioFilterbankFIR_MAX_NUM_FILTERS

};

class AudioFilterbankFIR_F32 : public AudioFilterBankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_FIR
	public:
		AudioFilterbankFIR_F32(void) : AudioFilterBankBase_F32() { }
		AudioFilterbankFIR_F32(const AudioSettings_F32 &settings) : AudioFilterBankBase_F32(settings) { }

		//virtual void begin(void);
		virtual void update(void);
		virtual int set_n_filters(int val);
		virtual int designFilters(int n_chan, int n_fir, float sample_rate_Hz, int block_len, float *crossover_freq);

		//core classes for designing and implementing the filters
		AudioConfigFIRFilterBank_F32 filterbankDesigner;
		AudioFilterFIR_F32 filters[AudioFilterbankFIR_MAX_NUM_FILTERS]; //every filter instance consumes memory to hold its states, which are numerous for an FIR filter
		
	private:

};

class AudioFilterbankBiquad_F32 : public AudioFilterBankBase_F32 {
//GUI: inputs:1, outputs:8  //this line used for automatic generation of GUI node  
//GUI: shortName:filterbank_Biquad
	public:
		AudioFilterbankBiquad_F32(void) : AudioFilterBankBase_F32() { }
		AudioFilterbankBiquad_F32(const AudioSettings_F32 &settings) : AudioFilterBankBase_F32(settings) { }

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