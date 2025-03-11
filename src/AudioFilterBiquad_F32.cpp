
/*
 * AudioFilterBiquad_F32.cpp
 *
 * Chip Audette, OpenAudio, Apr 2017
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioFilterBiquad_F32.h"

void AudioFilterBiquad_F32::update(void)
{
  audio_block_f32_t *block;

  block = AudioStream_F32::receiveWritable_f32();
  if (!block) return;

  // If there's no coefficient table, give up.  
  if (coeff_p == NULL) {
    AudioStream_F32::release(block);
    return;
  }

  // do passthru
  if ((coeff_p == IIR_F32_PASSTHRU) || (is_bypassed==true)) {
    // Just passthrough
    AudioStream_F32::transmit(block);
    AudioStream_F32::release(block);
    return;
  }

  // do IIR
  //arm_biquad_cascade_df1_f32(&iir_inst, block->data, block->data, block->length);
  processAudioBlock(block, block);
  
  
  //transmit the data
  AudioStream_F32::transmit(block); // send the IIR output
  AudioStream_F32::release(block);
}

int AudioFilterBiquad_F32::processAudioBlock(const audio_block_f32_t *block, audio_block_f32_t *block_new)  {
	if (!is_enabled || !block || !block_new) return -1;
	
	if (is_bypassed) {
		for (int i=0; i<block->length; i++) block_new->data[i] = block->data[i]; //copy input to output
	} else {
		// do IIR
		arm_biquad_cascade_df1_f32(&iir_inst, block->data, block_new->data, block->length);
	}

	//copy info about the block
	block_new->length = block->length;
	block_new->id = block->id;
	
	return 0;
}


//for all of these filters, a butterworth filter has q = 0.7017  (ie, 1/sqrt(2))
void AudioFilterBiquad_F32::calcLowpass(float32_t freq_Hz, float32_t _q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = _q;
	cur_type_ind = LOWPASS;
	
	//int coeff[5];
	double w0 = freq_Hz * (2.0 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	//double scale = 1073741824.0 / (1.0 + alpha);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = ((1.0 - cosW0) / 2.0) * scale;
	/* b1 */ coeff[1] = (1.0 - cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;
	
}
void AudioFilterBiquad_F32::calcHighpass(float32_t freq_Hz, float32_t _q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = _q;
	cur_type_ind = HIGHPASS;
	
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = ((1.0 + cosW0) / 2.0) * scale;
	/* b1 */ coeff[1] = -(1.0 + cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;

}
void AudioFilterBiquad_F32::calcBandpass(float32_t freq_Hz, float32_t _q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = _q;
	cur_type_ind = BANDPASS;
	
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = alpha * scale;
	/* b1 */ coeff[1] = 0;
	/* b2 */ coeff[2] = (-alpha) * scale;
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;

}
void AudioFilterBiquad_F32::calcNotch(float32_t freq_Hz, float32_t _q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = _q;
	cur_type_ind = NOTCH;
	
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = scale;
	/* b1 */ coeff[1] = (-2.0 * cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;
}
	
void AudioFilterBiquad_F32::calcLowShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = -1;
	cur_type_ind = LOWSHELF;
	cur_gain_for_shelf = gain;
			
	//int coeff[5];
	double a = pow(10.0, gain/40.0);
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
	double cosW0 = cos(w0);
	//generate three helper-values (intermediate results):
	double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/slope-1.0)+2.0*a );
	double aMinus = (a-1.0)*cosW0;
	double aPlus = (a+1.0)*cosW0;
	double scale = 1.0 / ( (a+1.0) + aMinus + sinsq);
	/* b0 */ coeff[0] =		a *	( (a+1.0) - aMinus + sinsq	) * scale;
	/* b1 */ coeff[1] =  2.0*a * ( (a-1.0) - aPlus  			) * scale;
	/* b2 */ coeff[2] =		a * ( (a+1.0) - aMinus - sinsq 	) * scale;
	/* a1 */ coeff[3] = -2.0*	( (a-1.0) + aPlus			) * scale;
	/* a2 */ coeff[4] =  		( (a+1.0) + aMinus - sinsq	) * scale;

}

void AudioFilterBiquad_F32::calcHighShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	q = -1;
	cur_type_ind = HIGHSHELF;
	cur_gain_for_shelf = gain;
			
	//int coeff[5];
	double a = pow(10.0, gain/40.0);
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
	double cosW0 = cos(w0);
	//generate three helper-values (intermediate results):
	double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/slope-1.0)+2.0*a );
	double aMinus = (a-1.0)*cosW0;
	double aPlus = (a+1.0)*cosW0;
	double scale = 1.0 / ( (a+1.0) - aMinus + sinsq);
	/* b0 */ coeff[0] =		a *	( (a+1.0) + aMinus + sinsq	) * scale;
	/* b1 */ coeff[1] = -2.0*a * ( (a-1.0) + aPlus  			) * scale;
	/* b2 */ coeff[2] =		a * ( (a+1.0) + aMinus - sinsq 	) * scale;
	/* a1 */ coeff[3] =  2.0*	( (a-1.0) - aPlus			) * scale;
	/* a2 */ coeff[4] =  		( (a+1.0) - aMinus - sinsq	) * scale;
}

float AudioFilterBiquad_F32::getBW_Hz(void) {
	//per https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html, Eq 4
	double omega = getCutoffFrequency_Hz() * (2.0*M_PI)/getSampleRate_Hz();
	double alpha = sin(omega) / (2.0*getQ());
	double foo = alpha / sin(omega);
	foo = asinh(foo);
	double BW = foo / ((log(2.0)/2.0) * (omega/sin(omega)));
	double BW_Hz = BW * getCutoffFrequency_Hz();
	return BW_Hz;
}

float AudioFilterBiquad_F32::increment_crossover_freq(float incr_fac) {
	float new_freq = getCutoffFrequency_Hz() * incr_fac;
	if (new_freq <= 0.0) {
		Serial.println(F("AudioFilterBiquad_F32: requesting cutoff of ") + String(new_freq) + F(" Hz, which is too low."));
	} else if (new_freq >= getSampleRate_Hz()/2.0) {
		Serial.println(F("AudioFilterBiquad_F32: requesting cutoff of ") + String(new_freq) + F(" Hz, which is too high for a sample rate of ") + String(getSampleRate_Hz()/2.0) + " Hz");
	} else {
		redesignGivenCutoffAndQ(new_freq, getQ());
	}
	return getCutoffFrequency_Hz();
}

float AudioFilterBiquad_F32::increment_filter_q(float incr_fac) {
	float new_Q = getQ() * incr_fac;
	if (new_Q <= 0.01) {
		Serial.println(F("AudioFilterBiquad_F32: requesting filter Q of ") + String(new_Q) + " Hz, which is too low.");
//	} else if (new_Q >= getSampleRate_Hz()/2.0) {
//		Serial.println(F("AudioFilterBiquad_F32: requesting filter Q of ") + String(new_Q) + " Hz, which is too high.");
	} else {
		redesignGivenCutoffAndQ(getCutoffFrequency_Hz(), new_Q);
	}
	return getQ();
}
	
int AudioFilterBiquad_F32::redesignGivenCutoffAndQ(float new_freq_Hz, float new_Q) {
	return redesignGivenCutoffAndQ(cur_type_ind, new_freq_Hz, new_Q);
}

int AudioFilterBiquad_F32::redesignGivenCutoffAndQ(int filtType, float new_freq_Hz, float new_Q) {	

	new_freq_Hz = max(0.0,min(getSampleRate_Hz()/2.0, new_freq_Hz));
	new_Q = max(0.0, new_Q);
	
	int ret_val = 0;  //assume OK
	
	switch (filtType) {
//		case NONE:
//			break;
		case LOWPASS:
			setLowpass( cur_filt_stage,new_freq_Hz,new_Q);
			break;
		case BANDPASS:
			setBandpass(cur_filt_stage,new_freq_Hz,new_Q);
			break;
		case HIGHPASS:
			setHighpass(cur_filt_stage,new_freq_Hz,new_Q);
			break;
		case NOTCH:
			setNotch(   cur_filt_stage,new_freq_Hz,new_Q);
			break;
//		case LOWSHELF:
//			setLowShelf(cur_filt_stage,cur_gain_for_shelf,new_freq_Hz,new_Q);
//			break;
//		case HIGHSHELF:
//			setHighShelf(cur_filt_stage,cur_gain_for_shelf,new_freq_Hz,new_Q);
//			break;
		default:
			ret_val = -1;
	}
	
	return ret_val;
	
} 

String AudioFilterBiquad_F32::getCurFilterTypeString(void) {
	switch (cur_type_ind) {
		//case NONE:
		//	return String(F("Not Specified"));
		case LOWPASS:
			return String(F("Lowpass"));
		case BANDPASS:
			return String(F("Bandpass"));
		case HIGHPASS:
			return String(F("Highpass"));
		case NOTCH:
			return String(F("Notch"));
		case LOWSHELF:
			return String(F("Low Shelf"));
		case HIGHSHELF:
			return String(F("High Shelf"));				
	}
	return String("Not Specified");
}

void AudioFilterBiquad_F32::setupFromSettings(AudioFilterBiquad_F32_settings &state) {
	redesignGivenCutoffAndQ(state.cur_type_ind, 
							state.cutoff_Hz,
							state.q);
	bypass(state.is_bypassed);
}
void AudioFilterBiquad_F32::getSettings(AudioFilterBiquad_F32_settings *state) {
	state->cur_type_ind = cur_type_ind;
	state->is_bypassed = get_is_bypassed();
	state->cutoff_Hz = getCutoffFrequency_Hz();
	state->q = getQ();
}

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SD methods for settings
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AudioFilterBiquad_F32_settings_SD::readFromSDFile(SdFile *file, const String &var_name) {
	const int buff_len = 300;
	char line[buff_len];
	
	//find start of data structure
	char targ_str[] = "AudioFilterBiquad_F32_settings";
	//int lines_read = readRowsUntilTargStr(file,line,buff_len,targ_str); //file is incremented so that the next line should be the first part of the DSL data
	int lines_read = readRowsUntilBothTargStrs(file,line,buff_len,targ_str,var_name.c_str()); //file is incremented so that the next line should be the first part of the DSL data
	if (lines_read <= 0) {
		Serial.println("AudioFilterBiquad_F32_settings_SD: readFromSDFile: *** could not find start of " + String(targ_str) + String(" data in file."));
		return -1;
	}

	// read the overall settings
	if (readAndParseLine(file, line, buff_len, &cur_type_ind, 1) < 0) return -1;
	if (readAndParseLine(file, line, buff_len, &is_bypassed, 1) < 0) return -1;
	if (readAndParseLine(file, line, buff_len, &cutoff_Hz, 1) < 0) return -1;
	if (readAndParseLine(file, line, buff_len, &q, 1) < 0) return -1;
	
	//write to serial for debugging
	//printAllValues();

	return 0;
}

int AudioFilterBiquad_F32_settings_SD::readFromSD(SdFat &sd, String &filename_str, const String &var_name) {
	int ret_val = 0;
	char filename[100]; filename_str.toCharArray(filename,99);
	
	SdFile file;
	
	//open SD
	ret_val = beginSD_wRetry(&sd,4); //number of retries
	if (ret_val != 0) { Serial.println("AudioFilterBiquad_F32_settings_SD: readFromSD: *** ERROR ***: could not sd.begin()."); return -1; }
	
	//open file
	if (!(file.open(filename,O_READ))) {   //open for reading
		Serial.print("AudioFilterBiquad_F32_settings_SD: readFromSD: readFromSD: cannot open file ");
		Serial.println(filename);
		return -1;
	}
	
	//read data
	ret_val = readFromSDFile(&file, var_name);
	
	//close file
	file.close();
	
	//return
	return ret_val;
}

void AudioFilterBiquad_F32_settings_SD::printToSDFile(SdFile *file, const String &var_name) {
	char header_str[] = "AudioFilterBiquad_F32_settings";
	writeHeader(file, header_str, var_name.c_str());
	
	writeValuesOnLine(file, &cur_type_ind, 1, true,  "FilterType: 1=LOWPASS, 2=BANDPASS, 3=HIGHPASS, 4=NOTCH", 1);
	writeValuesOnLine(file, &is_bypassed,  1, true,  "isBypassed: 1 = bypass the filter; 0 = filter is active", 1);
	writeValuesOnLine(file, &cutoff_Hz,    1, true,  "for LOWPASS or HIGPASS: Cutoff Frequency (Hz);  for BANDPASS or NOTCH: Center Frequency (Hz)", 1);
	writeValuesOnLine(file, &q,            1, false, "Filter Q (width / center)", 1);   //no trailing comma on the last one 
	
	writeFooter(file); 
}

int AudioFilterBiquad_F32_settings_SD::printToSD(SdFat &sd, String &filename_str, const String &var_name, bool deleteExisting) {
	SdFile file;
	char filename[100]; filename_str.toCharArray(filename,99);
	
	//open SD
	int ret_val = beginSD_wRetry(&sd,5); //number of retries
	if (ret_val != 0) { Serial.println("AudioFilterBiquad_F32_settings_SD: printToSD: *** ERROR ***: could not sd.begin()."); return -1; }
	
	//delete existing file
	if (deleteExisting) {
		if (sd.exists(filename)) sd.remove(filename);
	}
	
	//open file
	file.open(filename,O_RDWR  | O_CREAT | O_APPEND); //open for writing
	if (!file.isOpen()) {  
		Serial.print("AudioFilterBiquad_F32_settings_SD: printToSD: cannot open file ");
		Serial.println(filename);
		return -1;
	}
	
	//write data
	printToSDFile(&file, var_name);
	
	//close file
	file.close();
	
	//return
	return 0;
}


// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// UI Methods
//
// 
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////


String AudioFilterBiquad_F32_UI::getCurFilterFrequencyNameString(void) {
	switch (cur_type_ind) {
		case NONE:
			return String(F("Not Specified"));
		case LOWPASS:
			return String(F("Cutoff"));
		case BANDPASS:
			return String(F("Center"));
		case HIGHPASS:
			return String(F("Cutoff"));
		case NOTCH:
			return String(F("Center"));
		case LOWSHELF:
			return String(F("Transition"));
		case HIGHSHELF:
			return String(F("Transition"));				
		default:
			return String(F("Frequency"));
	}	
}

void AudioFilterBiquad_F32_UI::printHelp(void) {
	Serial.println(" " + name_for_UI + ": Prefix = " + getPrefix()); //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(F("   INFO: Filter type = ") + getCurFilterTypeString());
	Serial.println(F("   1/!: Incr/Decrease ") + getCurFilterFrequencyNameString() + F(" frequency (currently ") + String(getCutoffFrequency_Hz()) + " Hz)");
	Serial.println(F("   q/Q: Incr/Decrease filter Q (currently ") + String(getQ()) + ", making BW = " + String(getBW_Hz()) + " Hz)");
}

bool AudioFilterBiquad_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h
	
	//ignoring the chan_char (it's probably always 'x')

	Serial.println("AudioFilterBiquad_F32_UI: received " + String(data_char));
	
	bool ret_val = true;
	switch (data_char) {
		case '1':
			increment_crossover_freq(freq_increment_fac);
			updateGUI_cutoff();
			break;
		case '!':
			increment_crossover_freq(1.0/freq_increment_fac);
			updateGUI_cutoff();
			break;
		case 'q':
			increment_filter_q(q_increment_fac);
			updateGUI_Q();
			break;
		case 'Q':
			increment_filter_q(1./q_increment_fac);
			updateGUI_Q();
			break;
		case 'b':
			bypass(true);
			updateGUI_bypass();
			break;
		case 'B':
			bypass(false);
			updateGUI_bypass();
			break;
		default:
			ret_val = false;
	}
	
	return ret_val;
}

void AudioFilterBiquad_F32_UI::setFullGUIState(bool activeButtonsOnly) {
	updateGUI_cutoff();
	updateGUI_Q();
	updateGUI_bypass();
}

void AudioFilterBiquad_F32_UI::updateGUI_cutoff(bool activeButtonsOnly) {
	//send the cutoff
	String  button_name = ID_char_fn + freq_id_str;
	setButtonText(button_name,String(round(getCutoffFrequency_Hz())));
}

void AudioFilterBiquad_F32_UI::updateGUI_Q(bool activeButtonsOnly) {
	//send the Q
	String button_name = ID_char_fn + q_id_str;
	setButtonText(button_name,String(round(getQ())));

	//send the BW
	button_name = ID_char_fn + BW_id_str;
	if (cur_type_ind == NOTCH) {
		setButtonText(button_name,String(round(getBW_Hz())));
	} else {
		setButtonText(button_name,String(""));
	}
}

void AudioFilterBiquad_F32_UI::updateGUI_bypass(bool activeButtonsOnly) {
	if ((is_bypassed) || (!activeButtonsOnly)) {
		String  button_name = ID_char_fn + passthru_id_str;
		setButtonState(button_name,is_bypassed);
	}
	if ((!is_bypassed) || (!activeButtonsOnly)) {
		String  button_name = ID_char_fn + normal_id_str;
		setButtonState(button_name,!is_bypassed);
	}
}

TR_Card* AudioFilterBiquad_F32_UI::addCard_cutoffFreq(TR_Page *page_h) {
	String card_title = getCurFilterFrequencyNameString() + String(" Freq (Hz)");
	return addCardPreset_UpDown(page_h, card_title, freq_id_str,charMapDown[0],charMapUp[0]);
}
TR_Card* AudioFilterBiquad_F32_UI::addCard_filterQ(TR_Page *page_h) {
	String card_title = String("Filter Q (Sharpness)");
	return addCardPreset_UpDown(page_h, card_title, q_id_str,'Q','q');
}
TR_Card* AudioFilterBiquad_F32_UI::addCard_filterBW(TR_Page *page_h) {
	String card_title = String("Filter Width (Hz)");
	
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard(card_title);
	if (card_h == NULL) return NULL;
	
	String button_id = ID_char_fn + BW_id_str;
	card_h->addButton("","",button_id,12); //label, command, id, width...display for the bandwidth
	return card_h;
}
TR_Card* AudioFilterBiquad_F32_UI::addCard_filterBypass(TR_Page *page_h){
	String card_title = String("Filter Operation");
	
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard(card_title);
	if (card_h == NULL) return NULL;
	
	String prefix = getPrefix();   //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	
	String button_id = ID_char_fn + normal_id_str;
	card_h->addButton("Normal",prefix + "B",button_id,6); //label, command, id, width
	
	button_id = ID_char_fn + passthru_id_str;
	card_h->addButton("Bypass",prefix + "b",button_id,6); //label, command, id, width

	return card_h;
}

TR_Page* AudioFilterBiquad_F32_UI::addPage_CutoffAndQ(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage(name_for_UI);
	if (page_h == NULL) return NULL;
	addCard_cutoffFreq(page_h);
	addCard_filterQ(page_h);
	return page_h;
};

TR_Page* AudioFilterBiquad_F32_UI::addPage_CutoffAndQAndBW(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage(name_for_UI);
	if (page_h == NULL) return NULL;
	addCard_cutoffFreq(page_h);
	addCard_filterQ(page_h);
	addCard_filterBW(page_h);
	return page_h;
};
