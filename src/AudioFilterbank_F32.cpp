
/*
 * AudioFilterbank_F32.cpp
 *
 * Chip Audette, OpenAudio, Aug 2021
 *
 * MIT License,  Use at your own risk.
 *
*/


#include <AudioFilterbank_F32.h>


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// FIR Filterbank
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//void AudioFilterbankFIR_F32::begin(void) {
//	//initialize all filters
//	for (Ichan=0; Ichan < n_filters; Ichan++) filters[Ichan].begin(); //sets all filters to pass-through
//	enable(true); //enable this overall filterbank
//}


void AudioFilterbankFIR_F32::update(void) {

	//return if not enabled
	if (!is_enabled) return;

	//get the input audio
	audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

	//loop over each output
	int any_error;
	audio_block_f32_t *block_new;
	for (int Ichan = 0; Ichan < n_filters; Ichan++) {
		block_new = AudioStream_F32::allocate_f32();
		if (!block_new) {			
			if (filters[Ichan].get_is_enabled()) {
				any_error = filters[Ichan].processAudioBlock(block,block_new);
				if (!any_error) {
					AudioStream_F32::transmit(block_new,Ichan);
				}
			}
			AudioStream_F32::release(block_new);
		}
	}
	
	//release the original audio block
	AudioStream_F32::release(block);
}

int AudioFilterbankFIR_F32::set_n_filters(int val) {
	val = min(val, AudioFilterbankFIR_MAX_NUM_FILTERS); 
	n_filters = val;
	for (int Ichan = 0; Ichan < AudioFilterbankFIR_MAX_NUM_FILTERS; Ichan++) {
		if (Ichan < n_filters) {
			filters[Ichan].enable(true);  //enable the individual filter
		} else {
			filters[Ichan].enable(false); //disable the individual filter
		}
	}		
	state.set_n_filters(n_filters);
	return get_n_filters();
}


int AudioFilterbankFIR_F32::designFilters(int n_chan, int n_fir, float sample_rate_Hz, int block_len, float *crossover_freq) {
	n_chan = set_n_filters(n_chan); //will automatically limit to the max allowed number of filters
	
	//allocate memory (temporarily) for the filter coefficients
	float filter_coeff[n_chan][n_fir];
	if (filter_coeff == NULL) { enable(false); return -1; }  //failed to allocate memory
	
	//call the designer...only for N_FIR up to 1024...but will it really work if it is that big??  64, 96, 128 are more normal
	int ret_val = filterbankDesigner.createFilterCoeff(n_chan, n_fir, sample_rate_Hz, crossover_freq, (float *)filter_coeff);
	if (ret_val < 0) { enable(false); return -1; } //failed to compute coefficients
	
	//copy the coefficients over to the individual filters
	for (int i=0; i< n_chan; i++) filters[i].begin(filter_coeff[i], n_fir, block_len);
			
	//copy the crossover frequencies to the state
	state.set_crossover_freq_Hz(crossover_freq, n_chan);
	state.filter_order = n_fir;
	
	//normal return
	enable(true);
	return 0;
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// IIR Filterbank
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void AudioFilterbankBiquad_F32::update(void) {

	//return if not enabled
	if (!is_enabled) return;

	//get the input audio
	audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

	//loop over each output
	int any_error;
	audio_block_f32_t *block_new;
	for (int Ichan = 0; Ichan < n_filters; Ichan++) {
		block_new = AudioStream_F32::allocate_f32();
		if (!block_new) {			
			if (filters[Ichan].get_is_enabled()) {
				any_error = filters[Ichan].processAudioBlock(block,block_new);
				if (!any_error) {
					AudioStream_F32::transmit(block_new,Ichan);
				}
			}
			AudioStream_F32::release(block_new);
		}
	}
	
	//release the original audio block
	AudioStream_F32::release(block);
}

int AudioFilterbankBiquad_F32::set_n_filters(int val) {
	val = min(val, AudioFilterbankBiquad_MAX_NUM_FILTERS); 
	n_filters = val;
	for (int Ichan = 0; Ichan < AudioFilterbankBiquad_MAX_NUM_FILTERS; Ichan++) {
		if (Ichan < n_filters) {
			filters[Ichan].enable(true);  //enable the individual filter
		} else {
			filters[Ichan].enable(false); //disable the individual filter
		}
	}		
	state.set_n_filters(n_filters);
	return get_n_filters();
}



int AudioFilterbankBiquad_F32::designFilters(int n_chan, int n_iir, float sample_rate_Hz, int block_len, float *crossover_freq) {
	//check the input values
	if ((n_iir % 2) == 1) {  //is it an odd number?
		Serial.println("AudioFilterBankIIR_F32: designFilters: *** WARNING ***");
		Serial.println("  : requested n_iir = " + String(n_iir) + " is odd.  This class only supports even.");
		if (n_iir >= 3) {
			n_iir = n_iir - 1;
		}
		Serial.println("  : using n_iir = " + String(n_iir) + " instead.  Continuing...");
	}
	if (n_iir != AudioFilterbankBiquad_MAX_IIR_FILT_ORDER) {
		Serial.println("AudioFilterBankIIR_F32: designFilters: *** WARNING ***");
		Serial.println("  : requested n_iir = " + String(n_iir) + " but currently this only supports n_iir = " + String(AudioFilterbankBiquad_MAX_IIR_FILT_ORDER));
		n_iir = AudioFilterbankBiquad_MAX_IIR_FILT_ORDER;
		Serial.println("  : using n_iir = " + String(n_iir) + " instead.  Continuing...");
	}
	
	n_chan = set_n_filters(n_chan); //will automatically limit to the max allowed number of filters
	
	//allocate memory (temporarily) for the filter coefficients
	int N_BIQUAD_PER_FILT = n_iir / 2;
	if ((n_iir % 2) == 1) N_BIQUAD_PER_FILT++;  //if odd, increase by one so that there is enough space allcoated
	float filter_sos[n_chan][N_BIQUAD_PER_FILT * AudioFilterbankBiquad_COEFF_PER_BIQUAD];
	int filter_delay[n_chan]; //samples
	if ((filter_sos == NULL) || (filter_delay == NULL)) { enable(false); return -1; }  //failed to allocate memory
	
	//call the designer
	float td_msec = 0.000;  //assumed max delay (?) for the time-alignment process?
	int ret_val = filterbankDesigner.createFilterCoeff_SOS(n_chan, n_iir, sample_rate_Hz, td_msec, crossover_freq,(float *)filter_sos, filter_delay);
	
	if (ret_val < 0) { enable(false); return -1; } //failed to compute coefficients
	
	//copy the coefficients over to the individual filters
	for (int i=0; i< n_chan; i++) filters[i].setFilterCoeff_Matlab_sos(&(filter_sos[i][0]), N_BIQUAD_PER_FILT);  //sets multiple biquads.  Also calls begin().
		
	//copy the crossover frequencies to the state
	state.set_crossover_freq_Hz(crossover_freq, n_chan);	
	state.filter_order = n_iir;
	
	//normal return
	enable(true);
	return 0;
}



