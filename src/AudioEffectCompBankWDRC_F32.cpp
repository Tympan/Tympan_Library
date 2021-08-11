

/*
 * AudioEffectCompBank_F32.cpp
 *
 * Chip Audette, OpenAudio, Aug 2021
 *
 * MIT License,  Use at your own risk.
 *
*/

#include <AudioEffectCompBankWDRC_F32.h>

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Compressor Bank State Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//Tell this state-tracking class which compressors to reference when asking for the different
//state parameter values (so as we don't actually need to keep a local copy of each and every
//parameter of the compressors)
void AudioEffectCompBankWDRCState::setCompressors(std::vector<AudioEffectCompWDRC_F32> &c) {
		
	//make big enough
	compressors.resize(c.size());
	compressors.shrink_to_fit();
	
	//add new compressors
	for (int i=0; i < (int)c.size(); i++) compressors[i] = &(c[i]); //get pointer
	
	//shrink n_chan, if it overshoots past the end of our new array
	if (get_n_chan() > get_max_n_chan()) set_n_chan(get_max_n_chan());
}

// This method sets the value of the number of compressors being employed, as being tracked by this state-tracking class.
// This method doesn't change the algorithm's actual audio processing.
// To change the actual audio processing, do AudioEffectCompBankWDRC_F32::set_n_chan()
int AudioEffectCompBankWDRCState::set_n_chan(int n) {
	if (compressors.size() == 0) return 0;
	if ((n < 0) || (n > get_max_n_chan())) return -1; //this is an error
	n_chan = n;
	return n_chan; 
}


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Compressor Bank Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AudioEffectCompBankWDRC_F32::set_max_n_chan(int n_max_chan) {

	compressors.resize(n_max_chan);
	compressors.shrink_to_fit();
	int new_max_n_size = (int)compressors.size();
	
	state.setCompressors(compressors);
	if (new_max_n_size < get_n_chan()) set_n_chan(new_max_n_size);
	//if (get_n_chan() == 0) set_n_chan(new_max_n_size);
	
	return (int)compressors.size();
}


int AudioEffectCompBankWDRC_F32::configureFromDSLandGHA(float fs_Hz, const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha) {
	int n_chan_to_load = this_dsl.nchannel;
	Serial.println("AudioEffectCompBankWDRC_F32: configureFromDSLandGHA: this_dsl.nchannel = " + String(this_dsl.nchannel) + ", MAX = " + String(state.get_max_n_chan()));
	
	//sorta check the validity of inputs
	if ((n_chan_to_load < 1) || (n_chan_to_load >= __MAX_NUM_COMP)) {
		Serial.println(F("AudioEffectCompBankWDRC_F32: configureFromDSLandGHA: *** ERROR ***"));
		Serial.println(F("    : dsl.nchannel = ") + String(n_chan_to_load) + " must be 1 to " + String(__MAX_NUM_COMP));
		Serial.println(F("    : No change to WDRC configurations.  Returning."));
		return -1;  //-1 is error
	}

	//ensure that we have space
	if (n_chan_to_load > state.get_max_n_chan()) set_max_n_chan(n_chan_to_load);
	int max_n_chan = state.get_max_n_chan();
	if (n_chan_to_load > max_n_chan) {
		Serial.println(F("AudioEffectCompBankWDRC_F32: configureFromDSLandGHA: *** ERROR ***"));
		Serial.println(F("    : cannot allocate enough memory to fit all ") + String(n_chan_to_load) + F(" requested channels."));
		Serial.println(F("    : Continuing using only ") + String(max_n_chan) + F(" channels..."));
		n_chan_to_load = max_n_chan;
	}
	
	//parse the inputs that apply to all channels [logic and values are extracted from from CHAPRO repo agc_prepare.c]
	float atk = (float)this_dsl.attack;   //milliseconds!
	float rel = (float)this_dsl.release;  //milliseconds!
	float fs = (float)fs_Hz; // override based on the global setting...was float fs = gha->fs;
	float maxdB = (float) this_dsl.maxdB;
	
	//now, loop over each channel
	for (int i=0; i < n_chan_to_load; i++) {

		//parse the per-channel inputs
		float exp_cr = (float)this_dsl.exp_cr[i];
		float exp_end_knee = (float)this_dsl.exp_end_knee[i];
		float tk = (float) this_dsl.tk[i];
		float comp_ratio = (float) this_dsl.cr[i];
		float tkgain = (float) this_dsl.tkgain[i];
		float bolt = (float) this_dsl.bolt[i];

		// adjust BOLT
		float cltk = (float)this_gha.tk;
		if (bolt > cltk) bolt = cltk;
		if (tkgain < 0) bolt = bolt + tkgain;

		//set the compressor's parameters
		compressors[i].setSampleRate_Hz(fs);
		compressors[i].setParams(atk,rel,maxdB,exp_cr,exp_end_knee,tkgain,comp_ratio,tk,bolt);
	}
	
	//Serial.println(F("AudioEffectCompBankWDRC_F32: configureFromDSLandGHA: n_chan_to_load = ") + String(n_chan_to_load));
	set_n_chan(n_chan_to_load);
	is_enabled = true;
	
	return n_chan_to_load;  //returns number of channels loaded from the DSL
	
}

void AudioEffectCompBankWDRC_F32::update(void) {
	//return if not enabled
	if (!is_enabled) return;
	
	//loop over each channel...but only those up to the active channel limit
	int n_chan = state.get_n_chan();
	for (int Ichan=0; Ichan < n_chan; Ichan++) {
		
		 //request the in-coming data block
		audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32(Ichan);
		
		if (block != NULL) { //did we get a block of data?
		
			//request a data block to hold th processed data
			audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
			
			if (out_block != NULL) { //did we get a valid memory handle?
				//do the algorithm
				int is_error = compressors[Ichan].processAudioBlock(block,out_block); //anything other than a zero is an error
				
				//if we had no error, transmit the processed data
				if (!is_error) AudioStream_F32::transmit(out_block, Ichan);

			} else {
				//Serial.println(F("AudioEffectCompBankWDRC_F32: update: could not allocate out_block ") + String(Ichan));
			}
			AudioStream_F32::release(out_block);  //release the memory block that we requested 
		} 
		AudioStream_F32::release(block); //release the memory block that we requested
	} 
}

int AudioEffectCompBankWDRC_F32::set_n_chan(int val) {
	val = min(val, state.get_max_n_chan()); 
	int n_chan = state.set_n_chan(val);
	for (int Ichan = 0; Ichan < (int)compressors.size(); Ichan++) {
		if (Ichan < n_chan) {
			//compressors[Ichan].enable(true);  //enable the individual compressor [there is no such method?]
		} else {
			//compressors[Ichan].enable(false); //disable the individual compressor [there is no such method?]
		}
	}		
	
	if (n_chan>0) {
		is_enabled = true;
	} else {
		is_enabled = false;
	}
	
	return n_chan;
}



// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Compbank UI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioEffectCompBankWDRC_F32_UI::printHelp(void) {
	Serial.println(F(" WDRC Compressor Bank: Prefix = ") + getPrefix()); //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(F("   a/A: incr/decrease global attack time (") + String(state.getAttack_msec(),1) + " msec)");
	Serial.println(F("   r/R: incr/decrease global release time (") + String(state.getRelease_msec(),0) + " msec)");
	Serial.println(F("   m,M: Incr/decrease global scale factor (") + String(getMaxdB(),0) + " dBSPL at 0 dBFS)");
}


bool AudioEffectCompBankWDRC_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

	//Serial.println("AudioEffectCompBankWDRC_F32_UI: processCharacterTriple: " + String(mode_char) + String(chan_char) + String(data_char));

	//we ignore the chan_char and only work with the data_char
	bool return_val = true;  //assume that we will find this character
	if (chan_char == global_char) {
		return_val = processCharacter_global(data_char);
	} else {
		int chan = findChan(chan_char);
		return_val = processCharacter_perChannel(data_char, chan);
	}
	return return_val;
}

bool AudioEffectCompBankWDRC_F32_UI::processCharacter_global(char data_char) {
	bool return_val = true;
	String self_id = "AudioEffectCompBankWDRC_F32"; //used when printing messages to Serial Monitor
		
	//set global parameters
	switch (data_char) {    
		case 'a':
			incrementAttack_all(time_incr_fac);
			Serial.println(self_id + String(": changed global attack to ") + String(getAttack_msec(),0) + " msec"); 
			updateCard_attack_global();  //send updated value to the GUI
			break;
		case 'A':
			incrementAttack_all(1.0f/time_incr_fac);
			Serial.println(self_id + String(": changed global attack to ") + String(getAttack_msec(),0) + " msec"); 
			updateCard_attack_global();  //send updated value to the GUI
			break;
		case 'r':
			incrementRelease_all(time_incr_fac);
			Serial.println(self_id + String(": changed global release to ") + String(getRelease_msec(),0) + " msec");
			updateCard_release_global();  //send updated value to the GUI
			break;
		case 'R':
			incrementRelease_all(1.0f/time_incr_fac);
			Serial.println(self_id + String(": changed global release to ") + String(getRelease_msec(),0) + " msec");
			updateCard_release_global();  //send updated value to the GUI
			break;
		case 'm':
			incrementMaxdB_all(1.0);
			Serial.println(self_id + String(": changed global scale factor to ") + String(getMaxdB(),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac_global();  //send updated value to the GUI
			break;
		case 'M':
			incrementMaxdB_all(-1.0);
			Serial.println(self_id + String(": changed global scale factor to ") + String(getMaxdB(),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac_global();  //send updated value to the GUI
			break;
		default:
			return_val = false;  //we did not process this character
	}
	
	return return_val;
}

bool AudioEffectCompBankWDRC_F32_UI::processCharacter_perChannel(char data_char, int chan) {
	bool return_val = true;
	String self_id = "AudioEffectCompBankWDRC_F32"; //used when printing messages to Serial Monitor
	
	//Serial.println(self_id + ": processCharacter_perChannel: " + String(data_char) + String(chan));
	
	switch (data_char) {    
		case 'a':
			incrementAttack(time_incr_fac, chan);
			Serial.println(self_id + String(": changed attack ") + String(chan) + " to " + String(getAttack_msec(chan),0) + " msec"); 
			updateCard_attack(chan);  //send updated value to the GUI
			break;
		case 'A':
			incrementAttack(1.0f/time_incr_fac, chan);
			Serial.println(self_id + String(": changed attack ") + String(chan) + " to " + String(getAttack_msec(chan),0) + " msec"); 
			updateCard_attack(chan);  //send updated value to the GUI
			break;
		case 'r':
			incrementRelease(time_incr_fac, chan);
			Serial.println(self_id + String(": changed release ") + String(chan) + " to " + String(getRelease_msec(chan),0) + " msec");
			updateCard_release(chan);  //send updated value to the GUI
			break;
		case 'R':
			incrementRelease(1.0f/time_incr_fac, chan);
			Serial.println(self_id + String(": changed release ") + String(chan) + " to " + String(getRelease_msec(chan),0) + " msec");
			updateCard_release(chan);  //send updated value to the GUI
			break;
		case 'm':
			incrementMaxdB(1.0, chan);
			Serial.println(self_id + String(": changed scale factor ") + String(chan) + " to " + String(getMaxdB(chan),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac(chan);  //send updated value to the GUI
			break;
		case 'M':
			incrementMaxdB(-1.0, chan);
			Serial.println(self_id + String(": changed scale factor ") + String(chan) + " to " + String(getMaxdB(chan),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac(chan);  //send updated value to the GUI
			break;
		case 'x':
			incrementExpCR(cr_fac, chan);
			Serial.println(self_id + String(": changed expansion comp ratio ") + String(chan) + " to " + String(getExpansionCompRatio(chan),2));
			updateCard_expCR(chan);  //send updated value to the GUI
			break;
		case 'X':
			incrementExpCR(-cr_fac, chan);
			Serial.println(self_id + String(": changed expansion comp ratio ") + String(chan) + " to " + String(getExpansionCompRatio(chan),2));
			updateCard_expCR(chan);  //send updated value to the GUI
			break;
		case 'z':
			incrementExpKnee(knee_fac, chan);
			Serial.println(self_id + String(": changed expansion knee ") + String(chan) + " to " + String(getKneeExpansion_dBSPL(chan),0) + " dB SPL");
			updateCard_expKnee(chan);  //send updated value to the GUI
			break;
		case 'Z':
			incrementExpKnee(-knee_fac, chan);
			Serial.println(self_id + String(": changed expansion knee ") + String(chan) + " to " + String(getKneeExpansion_dBSPL(chan),0) + " dB SPL");
			updateCard_expKnee(chan);  //send updated value to the GUI
			break;
		case 'g':
			incrementGain_dB(gain_fac, chan);
			Serial.println(self_id + String(": changed linear gain ") + String(chan) + " to " + String(getGain_dB(chan),0) + " dB");
			updateCard_linGain(chan);  //send updated value to the GUI
			break;
		case 'G':
			incrementGain_dB(-gain_fac, chan);
			Serial.println(self_id + String(": changed linear gain ") + String(chan) + " to " + String(getGain_dB(chan),0) + " dB");
			updateCard_linGain(chan);  //send updated value to the GUI
			break;
		case 'c':
			incrementCompRatio(cr_fac, chan);
			Serial.println(self_id + String(": changed compression ratio ") + String(chan) + " to " + String(getCompRatio(chan),2));
			updateCard_compRat(chan);  //send updated value to the GUI
			break;
		case 'C':
			incrementCompRatio(-cr_fac, chan);
			Serial.println(self_id + String(": changed compression ratio ") + String(chan) + " to " + String(getCompRatio(chan),2));
			updateCard_compRat(chan);  //send updated value to the GUI
			break;
		case 'k':
			incrementKnee(knee_fac, chan);
			Serial.println(self_id + String(": changed compression knee ") + String(chan) + " to " + String(getKneeCompressor_dBSPL(chan),0) + " dB SPL");
			updateCard_compKnee(chan);  //send updated value to the GUI
			break;
		case 'K':
			incrementKnee(-knee_fac, chan);
			Serial.println(self_id + String(": changed compression knee ") + String(chan) + " to " + String(getKneeCompressor_dBSPL(chan),0) + " dB SPL");
			updateCard_compKnee(chan);  //send updated value to the GUI
			break;
		case 'l':
			incrementLimiter(1.0, chan);
			Serial.println(self_id + String(": changed limiter knee ") + String(chan) + " to " + String(getKneeLimiter_dBSPL(chan),0) + " dB SPL");
			updateCard_limKnee(chan);   //send updated value to the GUI
			break;
		case 'L':
			incrementLimiter(-1.0, chan);
			Serial.println(self_id + String(": changed limiter knee ") + String(chan) + " to " + String(getKneeLimiter_dBSPL(chan),0) + " dB SPL");
			updateCard_limKnee(chan);   //send updated value to the GUI
			break; 
		default:
			return_val = false;  //we did not process this character
	}

	return return_val;	
}

//Given a character, see if it corresponds to any of the characters associated with a request to
//raise (or lower) the cross-over frequency.  If so, return which channel is being commanded to
//change.  Return -1 if no match is found.
int AudioEffectCompBankWDRC_F32_UI::findChan(char c, int direction) {  //direction is +1 (raise) or -1 (lower)

	//check inputs
	if (!((direction == -1) || (direction == +1))) return -1;  //-1 means "not found"
	
	//define the targets to search across
	char *charTest = charMapUp; 
	if (direction < 0) charTest = charMapDown; 
	
	//begin the search
	int Ichan = 0;
	while (Ichan < n_charMap) {
		if (c == charTest[Ichan]) return Ichan;
		Ichan++;
	}
	return -1; //-1 means "not found"
}



// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// TympanRemoteLayout GUI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_globals(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage(String("Compressor Bank, Global Parameters"));
	if (page_h == NULL) return NULL;
	
	addCard_attack_global(page_h);
	addCard_release_global(page_h);
	addCard_scaleFac_global(page_h);

	return page_h;
}

TR_Card* AudioEffectCompBankWDRC_F32_UI::addCard_attack_global(  TR_Page *page_h) { 
	flag_send_global_attack=true;
	return addCardPreset_UpDown(page_h, "Attack Time (msec)",  "att",    "A", "a");
};
TR_Card* AudioEffectCompBankWDRC_F32_UI::addCard_release_global( TR_Page *page_h) { 
	flag_send_global_release=true;
	return addCardPreset_UpDown(page_h, "Release Time (msec)", "rel",    "R", "r");
};
TR_Card* AudioEffectCompBankWDRC_F32_UI::addCard_scaleFac_global(TR_Page *page_h) { 
	flag_send_global_scaleFac=true;
	return addCardPreset_UpDown(page_h, "Scale (dBSPL at dBFS)","maxdB", "M", "m");
};

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_attack(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Attack Time (msec)", "att", "A", "a", get_n_chan());
	flag_send_perBand_attack = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_release(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Release Time (msec)", "rel", "R", "r", get_n_chan());
	flag_send_perBand_release = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_scaleFac(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Scale (dBSPL at dBFS)", "maxdB", "M", "m", get_n_chan());
	flag_send_perBand_scaleFac = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_expCompRatio(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Expansion CR (x:1)", "expCR", "X", "x", get_n_chan());
	flag_send_perBand_expCR = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_expKnee(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Expansion Knee (dB SPL)", "expKnee", "Z", "z", get_n_chan());
	flag_send_perBand_expKnee = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_linearGain(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Linear Gain (dB)", "linGain", "G", "g", get_n_chan());
	flag_send_perBand_linGain = true;    //tells updateAll to send the values associated with these buttons
	return page_h;
}

TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_compRatio(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Compression Ratio (x:1)", "compRat", "C", "c", get_n_chan());
	flag_send_perBand_compRat = true;   //tells updateAll to send the values associated with these buttons
	return page_h;
}
TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_compKnee(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Comp. Knee (dB SPL)", "compKnee", "K", "k", get_n_chan());
	flag_send_perBand_compKnee = true;   //tells updateAll to send the values associated with these buttons
	return page_h;
}
TR_Page* AudioEffectCompBankWDRC_F32_UI::addPage_limKnee(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("Compressor Bank");
	if (page_h == NULL) return NULL;
	
	addCardPreset_UpDown_multiChan(page_h, "Limiter Knee (dB SPL)", "limKnee", "L", "l", get_n_chan());
	flag_send_perBand_limKnee = true;   //tells updateAll to send the values associated with these buttons
	return page_h;
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Methods to update the fields in the GUI
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Make a bunch of methods to update the value of the parameter value shown in the app.
//Because these were all created using "addCardPreset_UpDown()", let's use the related
//method  "updateCardPreset_UpDown()", which also lives in SerialManager_UI.h
void AudioEffectCompBankWDRC_F32_UI::updateCard_attack(int i)  { 
	float val = getAttack_msec(i); String str_val = String(val,1);
	if (val >= 10.0f) str_val = String(val,0); //less resolution of < 10 msec
	updateCardPreset_UpDown("att",str_val,i); //this method is in SerialManager_UI.h
}
void AudioEffectCompBankWDRC_F32_UI::updateCard_release(int i) { updateCardPreset_UpDown("rel",     String(getRelease_msec(i),0),        i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_scaleFac(int i){ updateCardPreset_UpDown("maxdB",   String(getMaxdB(i),0),               i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_expCR(int i)   { updateCardPreset_UpDown("expCR",   String(getExpansionCompRatio(i),2),  i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_expKnee(int i) { updateCardPreset_UpDown("expKnee", String(getKneeExpansion_dBSPL(i),0), i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_linGain(int i) { updateCardPreset_UpDown("linGain", String(getGain_dB(i),0),             i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_compRat(int i) { updateCardPreset_UpDown("compRat", String(getCompRatio(i),2),           i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_compKnee(int i){ updateCardPreset_UpDown("compKnee",String(getKneeCompressor_dBSPL(i),0),i); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_limKnee(int i) { updateCardPreset_UpDown("limKnee", String(getKneeLimiter_dBSPL(i),0),   i); }

//Make global versions
void AudioEffectCompBankWDRC_F32_UI::updateCard_attack_global(void)  { 
	float val = getAttack_msec(); String str_val = String(val,1);
	if (val >= 10.0f) str_val = String(val,0); //less resolution of < 10 msec
	updateCardPreset_UpDown("att",str_val); //this method is in SerialManager_UI.h
}
void AudioEffectCompBankWDRC_F32_UI::updateCard_release_global(void) { updateCardPreset_UpDown("rel",  String(getRelease_msec(),0)); }
void AudioEffectCompBankWDRC_F32_UI::updateCard_scaleFac_global(void){ updateCardPreset_UpDown("maxdB",String(getMaxdB(),0)); }


//Update all the fields
 void AudioEffectCompBankWDRC_F32_UI::setFullGUIState(bool activeButtonsOnly) {
	//update the parameters that are assumed to be globally the same
	if (flag_send_global_attack) updateCard_attack_global();
	if (flag_send_global_release) updateCard_release_global(); 
	if (flag_send_global_scaleFac) updateCard_scaleFac_global();
	
	//update the parameters that are assumed to be per-band
	for (int i=0; i< get_n_chan(); i++) {
		if (flag_send_perBand_attack) updateCard_attack(i);
		if (flag_send_perBand_release) updateCard_release(i); 
		if (flag_send_perBand_scaleFac) updateCard_scaleFac(i); 
		if (flag_send_perBand_expCR) updateCard_expCR(i);
		if (flag_send_perBand_expKnee) updateCard_expKnee(i);
		if (flag_send_perBand_linGain) updateCard_linGain(i); 
		if (flag_send_perBand_compRat) updateCard_compRat(i);
		if (flag_send_perBand_compKnee) updateCard_compKnee(i);
		if (flag_send_perBand_limKnee) updateCard_limKnee(i);	 
	}
 }
