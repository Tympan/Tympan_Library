

#include <AudioEffectCompWDRC_F32.h>


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// AudioCompWDRCState Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//get parameter values from the compressors
float AudioCompWDRCState::getSampleRate_Hz(void) { return compressor->getSampleRate_Hz(); }
float AudioCompWDRCState::getAttack_msec(void) { return compressor->getAttack_msec(); };
float AudioCompWDRCState::getRelease_msec(void) { return compressor->getRelease_msec(); };
float AudioCompWDRCState::getScaleFactor_dBSPL_at_dBFS(void) { return compressor->getMaxdB(); };
float AudioCompWDRCState::getExpansionCompRatio(void) { return compressor->getExpansionCompRatio(); };
float AudioCompWDRCState::getKneeExpansion_dBSPL(void) { return compressor->getKneeExpansion_dBSPL(); };
float AudioCompWDRCState::getLinearGain_dB(void) { return compressor->getGain_dB(); };
float AudioCompWDRCState::getCompRatio(void) { return compressor->getCompRatio(); };
float AudioCompWDRCState::getKneeCompressor_dBSPL(void) { return compressor->getKneeCompressor_dBSPL(); };
float AudioCompWDRCState::getKneeLimiter_dBSPL(void) { return compressor->getKneeLimiter_dBSPL(); };

//These methods are not used to directly maintain the state of the AudioEffectCompWDRC.
//They are supporting methods
void AudioCompWDRCState::setCompressor(AudioEffectCompWDRC_F32 *c) { compressor = c; }  //get pointer
void AudioCompWDRCState::printWDRCParameters(void) {
	Serial.println("WDRC Parameters: ");
	Serial.println("  Sample rate (Hz) = " + String(getSampleRate_Hz(),0));
	Serial.println("  Attack (msec) = " + String(getAttack_msec(),0));
	Serial.println("  Release (msec) = " + String(getRelease_msec(),0));
	Serial.println("  Scale Factor (dBSPL at dB FS) = " + String(getScaleFactor_dBSPL_at_dBFS(),0));
	Serial.println("  Expansion Knee (dB SPL) = " + String(getKneeExpansion_dBSPL(),0));
	Serial.println("  Expansion CR = " + String(getExpansionCompRatio(),2));
	Serial.println("  Linear Gain (dB) = " + String(getLinearGain_dB(),0));
	Serial.println("  Compression Knee (dB SPL) = " + String(getKneeCompressor_dBSPL(),0));
	Serial.println("  Compression Ratio = " + String(getCompRatio(),2));
	Serial.println("  Limiter Knee (dB SPL) = " + String(getKneeLimiter_dBSPL(),0));
}
 
 
 
 
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// AudioEffectCompWDRC_F32 Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void AudioEffectCompWDRC_F32::setDefaultValues(void) {
	state.setCompressor(this);
	
	//set default values...taken from CHAPRO, GHA_Demo.c  from "amplify()"...ignores given sample rate
	//assumes that the sample rate has already been set!!!!
	BTNRH_WDRC::CHA_WDRC gha = {1.0f, // attack time (ms)
		50.0f,     // release time (ms)
		24000.0f,  // fs, sampling rate (Hz), THIS IS IGNORED!
		119.0f,    // maxdB, maximum signal (dB SPL)
		1.0f,      // compression ratio for lowest-SPL region (ie, the expansion region)
		0.0f,      // expansion ending kneepoint (set small to defeat the expansion)
		0.0f,      // tkgain, compression-start gain
		105.0f,    // tk, compression-start kneepoint
		10.0f,     // cr, compression ratio
		105.0f     // bolt, broadband output limiting threshold
	};
	setParams_from_CHA_WDRC(&gha);
}

void AudioEffectCompWDRC_F32::update(void) {
	//receive the input audio data
	audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
	if (block == NULL) return;

	//allocate memory for the output of our algorithm
	audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
	if (out_block == NULL) { AudioStream_F32::release(block); return; }

	//do the algorithm
	int is_error = processAudioBlock(block,out_block); //anything other than a zero is an error
	
	// transmit the block and release memory
	if (!is_error) AudioStream_F32::transmit(out_block); // send the output
	AudioStream_F32::release(out_block);
	AudioStream_F32::release(block);
}

int AudioEffectCompWDRC_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block) {
	if ((block == NULL) || (out_block == NULL)) return -1;  //-1 is error
	
	compress(block->data, out_block->data, block->length);
	
	//copy the audio_block info
	out_block->id = block->id;
	out_block->length = block->length;
	
	return 0;  //0 is OK
}

void AudioEffectCompWDRC_F32::compress(float *x, float *y, int n)    
//x, input, audio waveform data
//y, output, audio waveform data after compression
//n, input, number of samples in this audio block
{        
	// find smoothed envelope
	audio_block_f32_t *envelope_block = AudioStream_F32::allocate_f32();
	if (envelope_block == NULL) return;  //failed to allocate
	calcEnvelope.smooth_env(x, envelope_block->data, n);
	//float *xpk = envelope_block->data; //get pointer to the array of (empty) data values

	//calculate gain
	audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
	if (gain_block == NULL) return;  //failed to allocate
	calcGain.calcGainFromEnvelope(envelope_block->data, gain_block->data, n);
	
	//apply gain
	arm_mult_f32(x, gain_block->data, y, n);

	// release memory
	AudioStream_F32::release(envelope_block);
	AudioStream_F32::release(gain_block);
}


//set all of the parameters for the compressor using the CHA_WDRC "GHA" structure
void AudioEffectCompWDRC_F32::setParams_from_CHA_WDRC(const BTNRH_WDRC::CHA_WDRC *gha) {  //assumes that the sample rate has already been set!!!
	//configure the envelope calculator...assumes that the sample rate has already been set!
	calcEnvelope.setAttackRelease_msec(gha->attack,gha->release); //these are in milliseconds

	//configure the compressor
	calcGain.setParams_from_CHA_WDRC(gha);
}

//set all of the user parameters for the compressor...assuming no expansion regime
//assumes that the sample rate has already been set!!!
void AudioEffectCompWDRC_F32::setParams(float attack_ms, float release_ms, float maxdB, float tkgain, float comp_ratio, float tk, float bolt) {
	float exp_cr = 1.0, exp_end_knee = -200.0;  //this should disable the exansion regime
	setParams(attack_ms, release_ms, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
}

//set all of the user parameters for the compressor...assumes that there is an expansion regime
//assumes that the sample rate has already been set!!!
void AudioEffectCompWDRC_F32::setParams(float attack_ms, float release_ms, float maxdB, float exp_cr, float exp_end_knee, float tkgain, float comp_ratio, float tk, float bolt) {
  
	//configure the envelope calculator...assumes that the sample rate has already been set!
	calcEnvelope.setAttackRelease_msec(attack_ms,release_ms);

	//configure the WDRC gains
	calcGain.setParams(maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
}



// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// AudioEffectCompWDRC UI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void AudioEffectCompWDRC_F32_UI::printHelp(void) {
	String prefix = getPrefix();  //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(" " + name_for_UI + ": Prefix = " + prefix);
	Serial.println(F("   a,A: Incr/decrease attack time (") + String(getAttack_msec(),0) + "msec)"); 
	Serial.println(F("   r,R: Incr/decrease release time (") + String(getRelease_msec(),0) + "msec)");
	Serial.println(F("   m,M: Incr/decrease scale factor (") + String(getMaxdB(),0) + "dBSPL at 0 dBFS)");
	Serial.println(F("   x,X: Incr/decrease expansion comp ratio (") + String(getExpansionCompRatio(),2) + ")");
	Serial.println(F("   z,Z: Incr/decrease expansion knee (") + String(getKneeExpansion_dBSPL(),0) + "dB SPL)");
	Serial.println(F("   g,G: Incr/decrease linear gain (") + String(getGain_dB(),0) + "dB)");
	Serial.println(F("   c,C: Incr/decrease compression ratio (") + String(getCompRatio(),2) + ")");
	Serial.println(F("   k,K: Incr/decrease compression knee (") + String(getKneeCompressor_dBSPL(),0) + "dB SPL)");;
	Serial.println(F("   l,L: Incr/decrease limiter knee (") + String(getKneeLimiter_dBSPL(),0) + "dB SPL)");;
	
}; 

bool AudioEffectCompWDRC_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

	//we ignore the chan_char and only work with the data_char
	bool return_val = true;  //assume that we will find this character
	switch (data_char) {    
		case 'a':
			incrementAttack(time_incr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed attack to ") + String(getAttack_msec(),0) + " msec"); 
			updateCard_attack();  //send updated value to the GUI
			break;
		case 'A':
			incrementAttack(1.0f/time_incr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed attack to ") + String(getAttack_msec(),0) + " msec"); 
			updateCard_attack();  //send updated value to the GUI
			break;
		case 'r':
			incrementRelease(time_incr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed release to ") + String(getRelease_msec(),0) + " msec");
			updateCard_release();  //send updated value to the GUI
			break;
		case 'R':
			incrementRelease(1.0f/time_incr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed release to ") + String(getRelease_msec(),0) + " msec");
			updateCard_release();  //send updated value to the GUI
			break;
		case 'm':
			incrementMaxdB(1.0);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed scale factor to ") + String(getMaxdB(),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac();  //send updated value to the GUI
			break;
		case 'M':
			incrementMaxdB(-1.0);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed scale factor to ") + String(getMaxdB(),0) + " dBSPL at 0 dBFS");
			updateCard_scaleFac();  //send updated value to the GUI
			break;
		case 'x':
			incrementExpCR(cr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed expansion comp ratio to ") + String(getExpansionCompRatio(),2));
			updateCard_expComp();  //send updated value to the GUI
			break;
		case 'X':
			incrementExpCR(-cr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed expansion comp ratio to ") + String(getExpansionCompRatio(),2));
			updateCard_expComp();  //send updated value to the GUI
			break;
		case 'z':
			incrementExpKnee(knee_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed expansion knee to ") + String(getKneeExpansion_dBSPL(),0) + " dB SPL");
			updateCard_expKnee();  //send updated value to the GUI
			break;
		case 'Z':
			incrementExpKnee(-knee_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed expansion knee to ") + String(getKneeExpansion_dBSPL(),0) + " dB SPL");
			updateCard_expKnee();  //send updated value to the GUI
			break;
		case 'g':
			incrementGain_dB(gain_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed linear gain to ") + String(getGain_dB(),0) + " dB");
			updateCard_linGain();  //send updated value to the GUI
			break;
		case 'G':
			incrementGain_dB(-gain_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed linear gain to ") + String(getGain_dB(),0) + " dB");
			updateCard_linGain();  //send updated value to the GUI
			break;
		case 'c':
			incrementCompRatio(cr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed compression ratio to ") + String(getCompRatio(),2));
			updateCard_compRat();  //send updated value to the GUI
			break;
		case 'C':
			incrementCompRatio(-cr_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed compression ratio to ") + String(getCompRatio(),2));
			updateCard_compRat();  //send updated value to the GUI
			break;
		case 'k':
			incrementKnee(knee_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed compression knee to ") + String(getKneeCompressor_dBSPL(),0) + " dB SPL");
			updateCard_compKnee();  //send updated value to the GUI
			break;
		case 'K':
			incrementKnee(-knee_fac);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed compression knee to ") + String(getKneeCompressor_dBSPL(),0) + " dB SPL");
			updateCard_compKnee();  //send updated value to the GUI
			break;
		case 'l':
			incrementLimiter(1.0);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed limiter knee to ") + String(getKneeLimiter_dBSPL(),0) + " dB SPL");
			updateCard_limKnee();   //send updated value to the GUI
			break;
		case 'L':
			incrementLimiter(-1.0);
			Serial.println(F("AudioEffectCompWDRC_F32_UI: changed limiter knee to ") + String(getKneeLimiter_dBSPL(),0) + " dB SPL");
			updateCard_limKnee();   //send updated value to the GUI
			break;
		default:
			return_val = false;  //we did not process this character
	}

	return return_val;	
}




// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// TympanRemoteLayout GUI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



TR_Card* AudioEffectCompWDRC_F32_UI::addCard_attackRelease(TR_Page *page_h) {
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard("Attack/Release (msec)");
	if (card_h == NULL) return NULL;

	String prefix = getPrefix();   //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	String field_name1 = ID_char_fn + String("att");
	String field_name2 = ID_char_fn + String("rel");

	card_h->addButton("Att", "",          "",          3);  //label, command, id, width
	card_h->addButton("-",   prefix+"A",  "",          3);  //label, command, id, width
	card_h->addButton("",    "",          field_name1, 3);  //label, command, id, width
	card_h->addButton("+",   prefix+"a",  "",          3);  //label, command, id, width

	card_h->addButton("Rel", "",          "",          3);  //label, command, id, width
	card_h->addButton("-",   prefix+"R",  "",          3);  //label, command, id, width
	card_h->addButton("",    "",          field_name2, 3);  //label, command, id, width
	card_h->addButton("+",   prefix+"r",  "",          3);  //label, command, id, width

	return card_h;   	
};

//Make a bunch of cards (button groups) to adjust the different algorithm parameters.
//These are all simple cards that allow you to adjust the parameter up and down based on a character command.
//Note that the addCardPreset_UpDown is in SerialManager_UI.h...and it automatically prepends the prefix() to the cmd chars
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_attack(  TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Attack Time (msec)",      "att",     "A", "a");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_release( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Release Time (msec)",     "rel",     "R", "r");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_scaleFac(TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Scale (dBSPL at dBFS)","maxdB", "M", "m");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_expComp( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Expansion CR (x:1)",     "expCR",   "X", "x");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_expKnee( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Expansion Knee (dB SPL)","expKnee", "Z", "z");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_linGain( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Linear Gain (dB)",   "linGain", "G", "g");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_compRat( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Compression Ratio (x:1)",   "compRat", "C", "c");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_compKnee(TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Compressor Knee (dB SPL)", "compKnee","K", "k");};
TR_Card* AudioEffectCompWDRC_F32_UI::addCard_limKnee( TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Limiter Knee (dB SPL)","limKnee", "L", "l");};


	
//This GUI page does ALL parameters
TR_Card* AudioEffectCompWDRC_F32_UI::addCards_allParams(TR_Page *page_h) {
	if (page_h == NULL) return NULL;
	
	addCard_attack(  page_h);
	addCard_release( page_h);
	addCard_scaleFac(page_h);
	addCard_expComp( page_h);
	addCard_expKnee( page_h);
	addCard_linGain( page_h);
	addCard_compKnee(page_h);
	addCard_compRat( page_h);
	TR_Card *card_h = addCard_limKnee( page_h);

	return card_h;	
}
	
TR_Page* AudioEffectCompWDRC_F32_UI::addPage_allParams(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("WDRC Parameters");
	if (page_h == NULL) return NULL;
	
	//add all the cards
	addCards_allParams(page_h);

	return page_h;	
}

//This GUI page leaves off the attack, release, and maxDB, which all tend to be global.
//This page only does the WDRC parameters that are generally tailored per compressor
TR_Page* AudioEffectCompWDRC_F32_UI::addPage_compParams(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage("WDRC Parameters");
	if (page_h == NULL) return NULL;
	
	addCard_expComp( page_h);
	addCard_expKnee( page_h);
	addCard_linGain( page_h);
	addCard_compKnee(page_h);
	addCard_compRat( page_h);
	addCard_limKnee( page_h);
	
	return page_h;
}; 



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
void AudioEffectCompWDRC_F32_UI::updateCard_attack(void)  { 
	float val = getAttack_msec(); String str_val = String(val,1);
	if (val >= 10.0f) str_val = String(val,0); //less resolution of < 10 msec
	updateCardPreset_UpDown("att",str_val); //this method is in SerialManager_UI.h
}
void AudioEffectCompWDRC_F32_UI::updateCard_release(void) { updateCardPreset_UpDown("rel",     String(getRelease_msec(),0)); }
void AudioEffectCompWDRC_F32_UI::updateCard_scaleFac(void){ updateCardPreset_UpDown("maxdB",   String(getMaxdB(),0)); }
void AudioEffectCompWDRC_F32_UI::updateCard_expComp(void) { updateCardPreset_UpDown("expCR",   String(getExpansionCompRatio(),2)); }
void AudioEffectCompWDRC_F32_UI::updateCard_expKnee(void) { updateCardPreset_UpDown("expKnee", String(getKneeExpansion_dBSPL(),0)); }
void AudioEffectCompWDRC_F32_UI::updateCard_linGain(void) { updateCardPreset_UpDown("linGain", String(getGain_dB(),0)); }
void AudioEffectCompWDRC_F32_UI::updateCard_compRat(void) { updateCardPreset_UpDown("compRat", String(getCompRatio(),2)); }
void AudioEffectCompWDRC_F32_UI::updateCard_compKnee(void){ updateCardPreset_UpDown("compKnee",String(getKneeCompressor_dBSPL(),0)); }
void AudioEffectCompWDRC_F32_UI::updateCard_limKnee(void) { updateCardPreset_UpDown("limKnee", String(getKneeLimiter_dBSPL(),0)); }

//Update all the fields
 void AudioEffectCompWDRC_F32_UI::setFullGUIState(bool activeButtonsOnly) {
	updateCard_attack();
	updateCard_release(); 
	updateCard_scaleFac(); 
	updateCard_expComp();
	updateCard_expKnee();
	updateCard_linGain(); 
	updateCard_compRat();
	updateCard_compKnee();
	updateCard_limKnee();	 
 }
 