
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


// This method only sets the values of crossover frequencies as being tracked by this state-tracking class.
// This method doesn't change the algorithm's actual crossover frequencies employed in the audio processing.
// To change the actual signal processing, do AudioFilterbankFIR_F32::designFilters or AudioFilterbankBiquad_F32::designFilters
int AudioFilterbankState::set_crossover_freq_Hz(float *freq_Hz, int n_crossover) {  //n_crossover is n_filters-1
	//make sure that we have valid input
	if (n_crossover < 0) return -1;  //-1 is error
	
	//make sure that we have space
	int assumed_n_filters = n_crossover + 1; //n_crossover is always 1 less than number of filters
	if (get_max_n_filters() < assumed_n_filters) {
		int ret_val = set_max_n_filters(assumed_n_filters);
		if (ret_val < 0) {
			Serial.println(F("AudioFilterbankState: set_crossover_freq_Hz: *** ERROR ***"));
			Serial.println(F("    : could not allocate memory.  Returning early."));
			return ret_val;
		}
	}
	
	//if the number of filters is greater than zero, copy the freuqencies.
	if (n_crossover==0) return 0;  //this is OK
	for (int Ichan=0; Ichan < n_crossover; Ichan++) crossover_freq_Hz[Ichan] = freq_Hz[Ichan];
	return AudioFilterbankState::set_n_filters(assumed_n_filters); //returning zero means "OK".  Also, n_crossover = n_filter - 1;	
}

// Report the requested crossover frequency value stored in this state-tracking class
float AudioFilterbankState::get_crossover_freq_Hz(int Ichan) { 
	int n_crossover = n_filters-1;  //there will always be one less crossover frequency than filters
	if (Ichan < n_crossover) {  
		return crossover_freq_Hz[Ichan]; 
	} 
	return 0.0f;
}

// Copy all the cross-over frequency values into a float array provided by the user
int AudioFilterbankState::get_crossover_freq_Hz(float *freq_Hz, int n_requested_crossover) {
	int n_to_copy = min(n_requested_crossover, get_max_n_filters()-1);
	for (int i=0; i < n_to_copy; i++) {
		freq_Hz[i] = crossover_freq_Hz[i];
	}
	if (n_requested_crossover > n_to_copy) {
		Serial.println("AudioFilterbankState: get_crossover_freq_Hz: asked for " + String(n_requested_crossover) + " but only sent " + String(n_to_copy));
		return -1;  //you asked for too many filters, so return an error
	}
	return 0;  //return OK!
}

// This method sets the value of the number of filters being employed, as being tracked by this state-tracking class.
// This method doesn't change the algorithm's actual audio processing.
// To change the actual audio processing, do AudioFilterbankFIR_F32::designFilters or AudioFilterbankBiquad_F32::designFilters
int AudioFilterbankState::set_n_filters(int n) {
	if ((n < 0) || (n > get_max_n_filters())) return -1; //this is an error
	n_filters = n;
	return n_filters; 
}

//This method is purely internal to this state-tracking class.  It sets the size of the array used to hold
//the tracking of the crossover frequencies.  It must be equal to or larger than the actual number of filters
//tracked.
int AudioFilterbankState::set_max_n_filters(int n) { 
	
	//check inputs
	if ((n < 1) || (n > 64)) return -1; //-1 is an error
	
	//compare to current setting
	crossover_freq_Hz.resize(n);
	
	//check state of allocation
	if ((int)crossover_freq_Hz.size() != n) return -1;  //didn't allocate enough space!
	
	return 0;  //zero is OK;
}






// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Generic Filterbank Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Ensure that the frequency values are sorted low-to-high and that they are spaced away from each other
// by the minimum allowed amount.  "Direction" of +1 will move frequencies higher if they're too close.
// "Direction" of -1 will move frequencies lower if they're too close
//
// Sorts and adjusts the frequency values in-place, so be sure to use a writable frequency location!
int AudioFilterbankBase_F32::enforce_minimum_spacing_of_crossover_freqs(float *freqs_Hz, int n_crossover, float min_seperation_fac, int direction) {
	int n_freq_changed = 0;
	if (n_crossover <= 0) return n_freq_changed;
	
	
	float ratio;
	if (direction >= 0) {  
		//we are requesting unaccetpable values be moved upwards.  so, if any are too close, move them upward to make them big enough
		for (int i=0; i < n_crossover-1; i++) { //loop upward...start at the first one and end before you get to the last one
			ratio = freqs_Hz[i+1]/freqs_Hz[i];  //compare this one to the next one (which is why we're ending one early)
			if (ratio < min_seperation_fac) {
				//too small!  raise the upper frequency
				freqs_Hz[i+1]  = freqs_Hz[i] * min_seperation_fac;  //note that this could be pushed above nyquist.  danger!
				n_freq_changed++;
			}
		}
	} else {
		//we are requesting unaccetpable values be moved upwards.  so, if any are too close, move them upward to make them big enough
		for (int i=n_crossover-1; i > 0; i--) { //loop downward...start at the last one and end before you get to the first one
			ratio = freqs_Hz[i]/freqs_Hz[i-1];  //compare this one to the next one (which is why we're ending one early)
			if (ratio < min_seperation_fac) {
				//too small!  lower the lower frequency
				freqs_Hz[i-1]  = freqs_Hz[i] / min_seperation_fac; 
				n_freq_changed++;
			}
		}
	}
	
	return n_freq_changed;
}

// Increment the cross-over frequency (adjusting as necessary to keep them minimally seperated )
int AudioFilterbankBase_F32::increment_crossover_freq(int Ichan, float freq_increment_fac) {
	int n_freq_changed = 0;
	
	//check validity of inputs
	const int n_filters = get_n_filters();
	const int n_crossover = n_filters - 1;     //n_crossover is always n_filters-1
	if ((Ichan < 0) || (Ichan >= n_crossover) || (n_filters <= 0)) return -1;  // -1 is "error"

	//get a copy of the crossover frequencies
	float freqs_Hz[n_filters];  						 //allocate an array that is big
	if (freqs_Hz == NULL) return -1;  					 //did it allocate the memory?
	state.get_crossover_freq_Hz(freqs_Hz, n_crossover);  //copies cross-over frequencies into freqs_Hz	
		
	//increment the target frequency
	freqs_Hz[Ichan] = freqs_Hz[Ichan] *  freq_increment_fac;
	n_freq_changed++;
	
	//Ensure proper frequency spacing (don't let them be too close together)
	int direction = 1;  //baseline assumption is that we're reqeusting an upward move 
	if (freq_increment_fac < 1.0f) direction = -1;  //oh!  we're actually requesting a downward move.  OK.
	n_freq_changed += enforce_minimum_spacing_of_crossover_freqs(freqs_Hz, n_crossover, min_freq_seperation_fac, direction);
	
	//redesign the filters using the new crossover frequencies
	designFilters(n_filters, state.filter_order, state.sample_rate_Hz, state.audio_block_len, freqs_Hz);
	
	return n_freq_changed;
}



void AudioFilterbankBase_F32::sortFrequencies(float *freq_Hz, int n_freqs) {
	if (n_freqs <= 1) return;
	
	//brute force sort
	float min_val;
	int min_ind;
	for (int i=0; i < n_freqs-1; i++) {
		min_ind = i;
		min_val = freq_Hz[min_ind];
		for (int j= i+1; j < n_freqs; j++) {
			if (freq_Hz[j] < min_val) {
				//swap their locations to put the smaller one in the spot for the minimum
				freq_Hz[min_ind] = freq_Hz[j]; //put the new value into the spot for the minimum  
				freq_Hz[j] = min_val; //put the old minimum into the current spot
				min_val = freq_Hz[min_ind];  //hold onto the new minimum
			}
		}
	}	
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


int AudioFilterbankFIR_F32::set_max_n_filters(int n_max_chan) {
	if (n_max_chan < 0) return filters.size();
	filters.resize(n_max_chan);
	filters.shrink_to_fit();
	int new_max_n_size = (int)filters.size();
	
	state.set_max_n_filters(new_max_n_size);
	if (new_max_n_size < get_n_filters()) set_n_filters(new_max_n_size);
	return (int)filters.size();
}

void AudioFilterbankFIR_F32::update(void) {

	//return if not enabled
	if (!is_enabled) return;

	//get the input audio
	audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

	//Serial.println("AudioFilterbankFIR_F32: update: entering...");

	//loop over each filter
	int n_filters = state.get_n_filters();
	int any_error;
	audio_block_f32_t *block_new;
	for (int Ichan = 0; Ichan < n_filters; Ichan++) {
		block_new = AudioStream_F32::allocate_f32();
		if (!block_new) {			
			if (filters[Ichan].get_is_enabled()) {
				any_error = filters[Ichan].processAudioBlock(block,block_new);
				if (!any_error) {
					AudioStream_F32::transmit(block_new,Ichan);
				} else {
					//Serial.print(F("AudioFilterBankFIR_F32: update: error in processAudioBlock for filter "));
					//Serial.println(Ichan);
				}
			} else {
				//Serial.print(F("AudioFilterBankFIR_F32: update: filter is not enabled: Ichan = "));	Serial.println(Ichan);
			}
		} else {
			//Serial.println(F("AudioFilterBankFIR_F32: update: Could not audio memory (block_new)"));
		}
		AudioStream_F32::release(block_new);
	}
	
	//release the original audio block
	AudioStream_F32::release(block);
}

int AudioFilterbankFIR_F32::set_n_filters(int val) {
	int new_n_filters = min(val, (int)filters.size()); 
	int n_filters = state.set_n_filters(new_n_filters);
	for (int Ichan = 0; Ichan < new_n_filters; Ichan++) {
		if (Ichan < n_filters) {
			filters[Ichan].enable(true);  //enable the individual filter
		} else {
			filters[Ichan].enable(false); //disable the individual filter
		}
	}		
	return n_filters;
}


int AudioFilterbankFIR_F32::designFilters(int n_chan, int n_fir, float sample_rate_Hz, int block_len, float *crossover_freq) {

	//Serial.println("AudioFilterbankFIR_F32: designFilters: n_chan " + String(n_chan) 
	//	+ ", n_fir " + String(n_fir) 
	//	+ ", fs_Hz " + String(sample_rate_Hz) 
	//	+ ", block len " + String(block_len));
	//Serial.print("AudioFilterbankFIR_F32: designFilters: freqs (Hz): ");
	//for (int i=0;i< (n_chan-1); i++) { Serial.print(crossover_freq[i]); Serial.print(", "); } Serial.println();
	
	//validate inputs
	n_chan = set_n_filters(n_chan); //will automatically limit to the max allowed number of filters
	if (n_chan <= 0) { enable(false); return -1; }  //invalid inputs

	//Serial.println("AudioFilterbankFIR_F32: designFilters: updated n_chan " + String(n_chan));
	
	//sort and enforce minimum seperation of the crossover frequencies
	float freqs_Hz[n_chan];  //we really only need n_chan-1 for the n_crossover, but let's leave it as n_chan)
	if (freqs_Hz == NULL) { enable(false); return -1; }  //failed to allocate memory
	int n_crossover = n_chan - 1;
	for (int i=0; i<n_crossover;i++) { freqs_Hz[i] = crossover_freq[i]; } //copy to known-writable memory
	sortFrequencies(freqs_Hz, n_crossover);	  //sort the frequencies from smallest to highest
	enforce_minimum_spacing_of_crossover_freqs(freqs_Hz, n_crossover, min_freq_seperation_fac);  //nudge the frequencies if they are too close (does this protect against going over nyquist?)

	//Serial.print("AudioFilterbankFIR_F32: designFilters: sorted/nudged: freqs (Hz): ");
	//for (int i=0; i < n_crossover; i++) { Serial.print(crossover_freq[i]); Serial.print(", "); } Serial.println();

	//allocate memory (temporarily) for the filter coefficients
	//float filter_coeff[n_chan][n_fir];
	//if (filter_coeff == NULL) { enable(false); return -1; }  //failed to allocate memory	
	int n_coeff_needed = n_chan * n_fir;
	if (n_coeff_needed > n_coeff_allocated) {
		if (filter_coeff != NULL) delete filter_coeff;
		filter_coeff = new float[n_coeff_needed];
		if (filter_coeff == NULL) { enable(false); return -1; }  //failed to allocate memory
		n_coeff_allocated = n_coeff_needed;
	}
	
	//call the designer...only for N_FIR up to 1024...but will it really work if it is that big??  64, 96, 128 are more normal
	//Serial.println("AudioFilterbankFIR_F32: designFilters: creating coefficients...");
	int ret_val = filterbankDesigner.createFilterCoeff(n_chan, n_fir, sample_rate_Hz, freqs_Hz, (float *)filter_coeff);
	if (ret_val < 0) { 
		Serial.println("AudioFilterbankFIR_F32: designFilters: createFilterCoeff failed with code " + String(ret_val));
		enable(false); 
		return -1; //failed to compute coefficients
	} 

	
	//copy the coefficients over to the individual filters
	//Serial.println("AudioFilterbankFIR_F32: designFilters: setting coefficients for each filter...");
	for (int i=0; i<n_chan; i++) filters[i].begin(&(filter_coeff[i*n_fir]), n_fir, block_len);
			
	//copy the crossover frequencies to the state
	state.set_crossover_freq_Hz(freqs_Hz, n_crossover); //n_crossover is n_chan-1
	state.filter_order = n_fir;
	state.sample_rate_Hz = sample_rate_Hz;
	state.audio_block_len = block_len;
	
	//normal return
	enable(true);
	//Serial.println("AudioFilterbankFIR_F32: designFilters: returning normally at end.");
	return 0;
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// IIR Filterbank Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AudioFilterbankBiquad_F32::set_max_n_filters(int n_max_chan) {
	if (n_max_chan < 0) return (int)filters.size();
	filters.resize(n_max_chan);
	filters.shrink_to_fit();
	int new_max_n_size = (int)filters.size();
	
	state.set_max_n_filters(new_max_n_size);
	if (new_max_n_size < get_n_filters()) set_n_filters(new_max_n_size);
	return (int)filters.size();
}

void AudioFilterbankBiquad_F32::update(void) {

	//return if not enabled
	if (!is_enabled) return;

	//get the input audio
	audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;

	//loop over each filter
	int n_filters = state.get_n_filters();
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
		}
		AudioStream_F32::release(block_new);
	}
	
	//release the original audio block
	AudioStream_F32::release(block);
}

int AudioFilterbankBiquad_F32::set_n_filters(int val) {
	int new_n_filters = min(val, (int)filters.size()); 
	int n_filters = state.set_n_filters(new_n_filters);
	for (int Ichan = 0; Ichan < new_n_filters; Ichan++) {
		if (Ichan < n_filters) {
			filters[Ichan].enable(true);  //enable the individual filter
		} else {
			filters[Ichan].enable(false); //disable the individual filter
		}
	}		
	return n_filters;
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
	
	///more validation of inputs
	n_chan = set_n_filters(n_chan); //will automatically limit to the max allowed number of filters
	if (n_chan <= 0) { enable(false); return -1; }  //invalid inputs
	
	//sort and enforce minimum seperation of the crossover frequencies
	float freqs_Hz[n_chan];
	if (freqs_Hz == NULL) { enable(false); return -1; }  //failed to allocate memory
	int n_crossover = n_chan - 1;
	for (int i=0; i<n_crossover;i++) { freqs_Hz[i] = crossover_freq[i]; } //copy to known-writable memory
	sortFrequencies(freqs_Hz, n_crossover);	  //sort the frequencies from smallest to highest
	enforce_minimum_spacing_of_crossover_freqs(freqs_Hz, n_crossover,min_freq_seperation_fac);  //nudge the frequencies if they are too close (does this protect against going over nyquist?)
	
	
	//allocate memory (temporarily) for the filter coefficients
	int N_BIQUAD_PER_FILT = n_iir / 2;
	if ((n_iir % 2) == 1) N_BIQUAD_PER_FILT++;  //if odd, increase by one so that there is enough space allcoated
	int ncol = N_BIQUAD_PER_FILT * AudioFilterbankBiquad_COEFF_PER_BIQUAD;
	//float filter_sos[n_chan][ncol];
	int n_coeff_needed = n_chan *ncol;
	if (n_coeff_needed > n_coeff_allocated) {
		if (filter_coeff != NULL) delete filter_coeff;
		filter_coeff = new float[n_coeff_needed];
		if (filter_coeff == NULL) { enable(false); return -1; }  //failed to allocate memory
		n_coeff_allocated = n_coeff_needed;
	}
	float *filter_sos = filter_coeff;
	int filter_delay[n_chan]; //samples
	if ((filter_sos == NULL) || (filter_delay == NULL)) { enable(false); return -1; }  //failed to allocate memory
	
	
	//call the designer
	float td_msec = 0.000;  //assumed max delay (?) for the time-alignment process?
	int ret_val = filterbankDesigner.createFilterCoeff_SOS(n_chan, n_iir, sample_rate_Hz, td_msec, freqs_Hz,filter_sos, filter_delay);
	
	if (ret_val < 0) { enable(false); return -1; } //failed to compute coefficients
	
	//copy the coefficients over to the individual filters...is this right?!? 
	for (int i=0; i< n_chan; i++) filters[i].setFilterCoeff_Matlab_sos(&(filter_sos[i*ncol]), N_BIQUAD_PER_FILT);  //sets multiple biquads.  Also calls begin().
		
	//copy the crossover frequencies to the state
	state.set_crossover_freq_Hz(freqs_Hz, n_crossover);	 // n_crossover is n_chan-1
	state.filter_order = n_iir;
	state.sample_rate_Hz = sample_rate_Hz;
	state.audio_block_len = block_len;	
	
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



void AudioFilterbank_UI::printChanMsg(int direction) {  //direction +1 is up, -1 is down
	char *charMap = charMapUp;
	if (direction < 0) charMap = charMapDown;

	int n_crossover = this_filterbank->state.get_n_filters() - 1;	// n_crossover is always n_filters - 1
	Serial.print("   ");
	for (int i=0; i < min(n_crossover,n_charMap);i++) Serial.print(charMap[i]);	
	if (direction >= 0) {
		Serial.print(F(": Change crossover for channels (1-"));
		Serial.print(n_crossover);
		Serial.println(") by " + String(freq_increment_fac,3) + "x");
	} else {
		Serial.print(F(": Change crossover for channels (1-"));
		Serial.print(n_crossover);
		Serial.println(") by " + String(1.0/freq_increment_fac,3) + "x");
	}
}


void AudioFilterbank_UI::printHelp(void) {
	Serial.println(F(" Filterbank: Prefix = ") + getPrefix()); //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	printChanMsg(1);  //upward changes
	printChanMsg(-1); //downward changes
}

bool AudioFilterbank_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

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
		Serial.println("AudioFilterbank_UI: increasing crossover frequency[" + String(Ichan) + "]...");
		n_freqs_changed = this_filterbank->increment_crossover_freq(Ichan,freq_increment_fac);		
		if (n_freqs_changed > 0) was_action_taken = true;
	}
	
	//check to see if they are asking to *decrease* the frequency.  If so, which channel is being requested?
	if (!was_action_taken) {
		Ichan = findChan(c,-1);  //the -1 searches for characters commanding to *lower* the frequency
		if (Ichan >= 0) {
			Serial.println("AudioFilterbank_UI: decreasing crossover frequency[" + String(Ichan) + "]...");
			n_freqs_changed = this_filterbank->increment_crossover_freq(Ichan,1.0f/freq_increment_fac); 
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
	char *charTest = charMapUp; 
	if (direction < 0) charTest = charMapDown; 
	
	//begin the search
	int n_crossover = (this_filterbank->state.get_n_filters()) - 1;
	int Ichan = 0;
	while (Ichan < min(n_crossover,n_charMap)) {
		if (c == charTest[Ichan]) return Ichan;
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
	int n_crossover = this_filterbank->state.get_n_filters() - 1;
	if (n_crossover <= 0) return;
	for (int i=0; i < n_crossover; i++) sendOneFreq(i);
}

void AudioFilterbank_UI::sendOneFreq(int Ichan) {  //Ichan counts from zero
	if (Ichan < 0) return;
	if (Ichan > (this_filterbank->state.get_n_filters()-1)) return;
	setButtonText(freq_id_str + String(Ichan),String(this_filterbank->state.get_crossover_freq_Hz(Ichan),0));
}

void AudioFilterbank_UI::setFullGUIState(bool activeButtonsOnly) {
	sendAllFreqs();	
}

TR_Card* AudioFilterbank_UI::addCard_crossoverFreqs(TR_Page *page_h) {
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard("Crossover Freqs (Hz)");
	if (card_h == NULL) return NULL;
	String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
	int n_crossover = this_filterbank->get_n_filters()-1; //n_crossover is always n_filter - 1
	
	for (int i=0; i < min(n_crossover,n_charMap); i++) {
		String label = String(i+1) + String("-") + String(i+2);
		card_h->addButton(label, "", 				    "",                    3);  //label, command, id, width
		card_h->addButton("-",   prefix+charMapDown[i], "",                    3);  //label, command, id, width
		card_h->addButton("",    "",                    freq_id_str+String(i), 3);  //label, command, id, width
		card_h->addButton("+",   prefix+charMapUp[i],   "",                    3);  //label, command, id, width
	}

	return card_h;   	
};

TR_Page* AudioFilterbank_UI::addPage_crossoverFreqs(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Filterbank");
	if (page_h == NULL) return NULL;
	addCard_crossoverFreqs(page_h);
	return page_h;
}; 


