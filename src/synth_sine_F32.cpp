/* 
 * AudioSynthWaveformSine_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Modeled on: AudioSynthWaveformSine from Teensy Audio Library
 * 
 * Purpose: Create sine wave of given amplitude and frequency
 *
 * License: MIT License. Use at your own risk.
 *
 */

#include "synth_sine_F32.h"
#include "utility/dspinst.h"
#include "utility/data_waveforms.c"


// ///////////////////////////////////////////////////////////////////////////////////
// //////////////////////////// Define the methods for the AudioSynthWaveformSineState
// ///////////////////////////////////////////////////////////////////////////////////

float AudioSynthWaveformSineState::getSampleRate_Hz(void) { if (synthWaveform) { return synthWaveform->getSampleRate_Hz(); } else { return -1.0; } };
float AudioSynthWaveformSineState::getFrequency_Hz(void) { if (synthWaveform) { return synthWaveform->getFrequency_Hz(); } else { return -1.0; } };
float AudioSynthWaveformSineState::getAmplitude(void) { if (synthWaveform) { return synthWaveform->getAmplitude(); } else { return -1.0; } };
float AudioSynthWaveformSineState::getAmplitude_dBFS(void) { if (synthWaveform) { return synthWaveform->getAmplitude_dBFS(); } else { return -1.0; } };
void AudioSynthWaveformSineState::setSynthWaveform(AudioSynthWaveformSine_F32 *c) { synthWaveform = c; }
void AudioSynthWaveformSineState::printWaveformParameters(void) { 
	Serial.println("Waveform Parameters:");
	Serial.println("  Sample rate (Hz) = " + String(getSampleRate_Hz(),0));
	Serial.println("  Frequency (Hz) = " + String(getFrequency_Hz(),1));
	Serial.println("  Amplitude (dB Peak re: FS) = " + String(getAmplitude_dBFS(),1));
}

// ///////////////////////////////////////////////////////////////////////////////////
// //////////////////////////// Define the methods for the AudioSynthWaveformSine_F32
// ///////////////////////////////////////////////////////////////////////////////////

// data_waveforms.c
//extern "C" {
//  extern const int16_t AudioWaveformSine[257];
//}

void AudioSynthWaveformSine_F32::setDefaultValues(void) {
	state.setSynthWaveform(this);
	setFrequency_Hz(1000.0f);
	setAmplitude_dBFS(-12.0f);
}

void AudioSynthWaveformSine_F32::update(void)
{
	audio_block_f32_t *block;
	uint32_t i, ph, inc, index, scale;
	int32_t val1, val2;
	static uint32_t block_length = 0;
	
	if (enabled) {
		if (magnitude) {
			block = allocate_f32();
			if (block) {
				block_length = (uint32_t)block->length;
				ph = phase_accumulator;
				inc = phase_increment;
				for (i=0; i < block_length; i++) {
					index = ph >> 24;
					val1 = AudioWaveformSine_tympan[index];
					val2 = AudioWaveformSine_tympan[index+1];
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
#if defined(__ARM_ARCH_7EM__)
					block->data[i] = multiply_32x32_rshift32(val1 + val2, magnitude);
#elif defined(KINETISL)
					block->data[i] = (((val1 + val2) >> 16) * magnitude) >> 16;
#endif
					ph += inc;
					
					block->data[i] = block->data[i] / 32767.0f; // scale to float
				}
				phase_accumulator = ph;
				
				block_counter++;
				block->id = block_counter;
				
				AudioStream_F32::transmit(block);
				AudioStream_F32::release(block);
				return;
			}
		}
		phase_accumulator += phase_increment * block_length;
	}
}


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// AudioSynthWaveformSine_F32 UI Methods
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioSynthWaveformSine_F32_UI::printHelp(void) {
	String prefix = getPrefix();  //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(" " + name_for_UI + ": Prefix = " + prefix);
	Serial.println(F("   e,E: Enable/disable tone (currently ") + String(getEnabled() ? "Enabled" : "Disabled") + ")");
	Serial.println(F("   f,F: Incr/decrease frequency (") + String(getFrequency_Hz(),1) + " Hz)");
	Serial.println(F("   a,A: Incr/decrease amplitude (") + String(getAmplitude_dBFS(),1) + " dBFS)"); 

}; 


bool AudioSynthWaveformSine_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h

	//we ignore the chan_char and only work with the data_char
	bool return_val = true;  //assume that we will find this character
	switch (data_char) {
		case 'e':
			setEnabled(true);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: eanble tone = ") + String(getEnabled()));
			updateCard_enable();
			break;
		case 'E':
			setEnabled(false);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: eanble tone = ") + String(getEnabled()));
			updateCard_enable();
			break;
		case 'a':
			incrementAmplitude_dBFS(amp_fac_dB);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: changed amplitude to ") + String(getAmplitude_dBFS(),1) + " dBFS"); 
			updateCard_amp_dBFS();  //send updated value to the GUI
			break;
		case 'A':
			incrementAmplitude_dBFS(-amp_fac_dB);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: changed amplitude to ") + String(getAmplitude_dBFS(),1) + " dBFS"); 
			updateCard_amp_dBFS();  //send updated value to the GUI
			break;
		case 'f':
			incrementFrequency(freq_fac);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: changed frequency to ") + String(getFrequency_Hz(),1) + " Hz");
			updateCard_freq();  //send updated value to the GUI
			break;
		case 'F':
			incrementFrequency(1.0/freq_fac);
			Serial.println(F("AudioSynthWaveformSine_F32_UI: changed frequency to ") + String(getFrequency_Hz(),1) + " Hz");
			updateCard_freq();  //send updated value to the GUI
			break;
		default:
			return_val = false;  //we did not process this character
	}

	return return_val;	
}


//Make a bunch of cards (button groups) to adjust the different algorithm parameters.
//These are all simple cards that allow you to adjust the parameter up and down based on a character command.
//Note that the addCardPreset_UpDown is in SerialManager_UI.h...and it automatically prepends the prefix() to the cmd chars
TR_Card* AudioSynthWaveformSine_F32_UI::addCard_freq( TR_Page *page_h)      { return addCardPreset_UpDown(page_h, "Frequency (Hz)",    "freq",     "F", "f");};
TR_Card* AudioSynthWaveformSine_F32_UI::addCard_amp_dBFS(  TR_Page *page_h) { return addCardPreset_UpDown(page_h, "Amplitude (dBFS)",  "amp_dB",   "A", "a");};

//add an on/off card for the enable
TR_Card* AudioSynthWaveformSine_F32_UI::addCard_enable(TR_Page *page_h) { 
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard("Play Tone");
	if (card_h == NULL) return NULL;

	String prefix = getPrefix();   //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	String field_name1 = ID_char_fn + String("on");
	String field_name2 = ID_char_fn + String("off");
	card_h->addButton("On",   prefix+"e",  field_name1, 6);  //label, command, id, width
	card_h->addButton("Off",  prefix+"E",  field_name2, 6);  //label, command, id, width
	
	return card_h;
}


//Add a default for single-page control of this sinewave generator
TR_Page* AudioSynthWaveformSine_F32_UI::addPage_default(TympanRemoteFormatter *gui) {
	if (gui == NULL) return NULL;
	TR_Page *page_h = gui->addPage(name_for_UI);
	if (page_h == NULL) return NULL;
	
	//add the default cards
	addCard_enable(page_h);
	addCard_freq(page_h);
	addCard_amp_dBFS(page_h);

	return page_h;	
}		
		
//Make a bunch of methods to update the value of the parameter value shown in the app.
//Because these were all created using "addCardPreset_UpDown()", let's use the related
//method  "updateCardPreset_UpDown()", which also lives in SerialManager_UI.h
void AudioSynthWaveformSine_F32_UI::updateCard_freq(void) {
	float value = getFrequency_Hz();
	String val_str = String(value,1);
	if (value >= 9999.999999f) val_str = String(value,0);
	updateCardPreset_UpDown("freq", val_str); 
}
void AudioSynthWaveformSine_F32_UI::updateCard_amp_dBFS(void){ updateCardPreset_UpDown("amp_dB",   String(getAmplitude_dBFS(),1)); }

//update the on/off field
void AudioSynthWaveformSine_F32_UI::updateCard_enable(void) { 
	bool enabled = getEnabled();
	String field_name1 = ID_char_fn + String("on");
	String field_name2 = ID_char_fn + String("off");
	setButtonState(field_name1,enabled);
	setButtonState(field_name2,!enabled);
}

//Update all the fields
 void AudioSynthWaveformSine_F32_UI::setFullGUIState(bool activeButtonsOnly) {
  updateCard_enable();
	updateCard_freq();
	updateCard_amp_dBFS(); 
 }		
		



