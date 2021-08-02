
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
// Filterbank State Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int AudioFilterbankState::set_crossover_freq_Hz(float *freq_Hz, int n_filts) {
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

float AudioFilterbankState::get_crossover_freq_Hz(int Ichan) { 
	if (Ichan < n_filters-1) {  //there will always be one less crossover frequency than filters
		return crossover_freq_Hz[Ichan]; 
	} 
	return 0.0f;
}

int AudioFilterbankState::get_crossover_freq_Hz(float *freq_Hz, int n_requested_filters) {
	for (int i=0; i < max(n_requested_filters, get_max_n_filters()); i++) {
		freq_Hz[i] = crossover_freq_Hz[i];
	}
	if (n_requested_filters > get_max_n_filters()) {
		return -1;  //you asked for too many filters, so return an error
	}
	return 0;  //return OK!
}

int AudioFilterbankState::set_n_filters(int n) {
	if ((n < 0) && (n > max_n_filters)) return -1; //this is an error
	n_filters = n;
	return n_filters; 
}

int AudioFilterbankState::set_max_n_filters(int n) { 
	if ((n < 0) || (n > 64)) return -1; //-1 is an error
	if (crossover_freq_Hz != NULL) delete crossover_freq_Hz; 
	max_n_filters = 0; n_filters = 0;
	if (n==0) return 0;  //zero is OK
	crossover_freq_Hz = new float[n];
	if (crossover_freq_Hz == NULL) return -1; //-1 is an error
	max_n_filters = n;
	return 0;  //zero is OK;
}

int AudioFilterbankBase_F32::increment_crossover_freq_Hz(int Ichan, float freq_increment_fac) {
	
	//TBD!!!
	
	return 0;
}


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// FIR Filterbank Methods
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
// IIR Filterbank Methods
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


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Filterbank UI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void AudioFilterbank_UI::printChanUpMsg(void) { 
	int N_CHAN = this_filterbank->state.get_n_filters();	//N_CHAN is the number of filters, so there are N_CHAN-1 cross-over frequencies
	char fooChar[] = "12345678"; 
	int nchar = 8;
	Serial.print(" ");
	for (int i=0; i < (N_CHAN-1);i++) {
		if (i < nchar) {
			Serial.print(fooChar[i]);   //use numbers 1-8 for first 8 channels
		} else {
			Serial.print('a' + (i - nchar));  //use lower-case a-z (and beyond) for more than 8 channels
		}
	}
	Serial.print(": Raise crossover frequency for channel (1-");
	Serial.print(N_CHAN);
	Serial.print(") by ");
}
void AudioFilterbank_UI::printChanDownMsg(void) { 
	int N_CHAN = this_filterbank->state.get_n_filters();	//N_CHAN is the number of filters, so there are N_CHAN-1 cross-over frequencies
	char fooChar[] = "!@#$%^&*"; 
	int nchar = 8; 
	Serial.print(" ");
	for (int i=0; i < (N_CHAN-1);i++) {
		if (i < nchar) {
			Serial.print(fooChar[i]);    //use the symbol above each number key 1-8 for first 8 channels
		} else {
			Serial.print('A' + (i - nchar));  //use upper-case A-Z (and beyond) for more than 8 channels
		}
	}
	Serial.print(": Lower crossover frequency for channel (1-");
	Serial.print(N_CHAN);
	Serial.print(") by ");
}

void AudioFilterbank_UI::printHelp(void) {
	Serial.println(F(" Filterbank: Prefix = ") + String(quadchar_start_char) + String(ID_char) + String("x"));
	printChanUpMsg(); Serial.println(String(freq_increment_fac,2) + "x");
	printChanDownMsg(); Serial.println(String(freq_increment_fac,2) + "x");
}

bool AudioFilterbank_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;

	//check to see if we have filters configured.  If not, return;
	int n_crossover = (this_filterbank->state.get_n_filters()) - 1;
	if (n_crossover < 1) return false;

	//we ignore the chan_char and only work with the data_char
	char c = data_char;
	
	//prepare
	bool was_action_taken = false;
	int Ichan;
	int n_freqs_changed = 0; //how many frequencies incrementCrossover() change? Ideally, just one is changed, but it might need to nudge the others, too
	
	//check to see if they are asking to *increase* the frequency.   If so, which channel is being requested?
	Ichan = findChan(c,+1); //the +1 searches for characters commanding to *raise* the frequency
	if (Ichan >= 0) { 
		n_freqs_changed = this_filterbank->increment_crossover_freq_Hz(Ichan,freq_increment_fac); 
		if (n_freqs_changed > 0) was_action_taken = true;
	}
	
	//check to see if they are asking to *decrease* the frequency.  If so, which channel is being requested?
	if (!was_action_taken) {
		Ichan = findChan(c,-1);  //the -1 searches for characters commanding to *lower* the frequency
		if (Ichan >= 0) {
			n_freqs_changed = this_filterbank->increment_crossover_freq_Hz(Ichan,1.0f/freq_increment_fac); 
			if (n_freqs_changed > 0) was_action_taken = true;
		}
	}
	
	//update the GUI, if any changes were made
	if (was_action_taken) {
		if (n_freqs_changed > 1) {
			sendAllFreqs();
			printCrossoverFreqs();
		} else {
			sendOneFreq(Ichan);
			Serial.println("AudioFilterbank_UI: Chan " + String(Ichan) + " crossover (Hz): " + String(this_filterbank->state.get_crossover_freq_Hz(Ichan),0));
		}
	}
	
	return was_action_taken;
}
	
//Given a character, see if it corresponds to any of the characters associated with a request to
//raise (or lower) the cross-over frequency.  If so, return which channel is being commanded to
//change.  Return -1 if no match is found.
int AudioFilterbank_UI::findChan(char c, int direction) {  //direction is +1 (raise) or -1 (lower)
	//check inputs
	if (!((direction == -1) || (direction == +1))) return -1;  //-1 means "not found"
	
	//define the targets to search across
	char charUp[] = "12345678"; char startCharUp = 'a';
	char charDown[] = "!@#$%^&*"; char startCharDown = 'A';
	int nchar = 8;
	char *charTest = charUp; char startChar = startCharUp;
	if (direction == -1) {
		charTest = charDown; startChar = startCharDown;
	}
	
	//begin the search
	int n_crossover = (this_filterbank->state.get_n_filters()) - 1;
	int Ichan = 0;
	while (Ichan < n_crossover) {
		if (Ichan < nchar) {
			if (c == charTest[Ichan]) return Ichan;
		} else  {
			if (c == (startChar + (Ichan-1))) return Ichan;
		}
		Ichan++;
	}
	return -1; //-1 means "not found"
}

void AudioFilterbank_UI::printCrossoverFreqs(void) {
	int n_crossover = (this_filterbank->state.get_n_filters()) - 1;
	
	Serial.print("AudioFilterbank_UI: crossover freqs (Hz): ");
	for (int i=0; i < n_crossover; i++) { 
		Serial.print(this_filterbank->state.get_crossover_freq_Hz(i),0); 
		if (i < (n_crossover-1)) Serial.print(", "); 
	} 
	Serial.println();
}

void AudioFilterbank_UI::sendAllFreqs(void) {
	
}

void AudioFilterbank_UI::sendOneFreq(int Ichan) {
	
}

void AudioFilterbank_UI::setFullGUIState(bool activeButtonsOnly) {
	
}