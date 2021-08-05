

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
	if (!block) return;

	//allocate memory for the output of our algorithm
	audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
	if (!out_block) { AudioStream_F32::release(block); return; }

	//do the algorithm
	int is_error = processAudioBlock(block,out_block); //anything other than a zero is an error
	
	// transmit the block and release memory
	if (!is_error) AudioStream_F32::transmit(out_block); // send the output
	AudioStream_F32::release(out_block);
	AudioStream_F32::release(block);
}

int AudioEffectCompWDRC_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block) {
	if ((!block) || (!out_block)) return -1;  //-1 is error
	
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
	if (!envelope_block) return;
	calcEnvelope.smooth_env(x, envelope_block->data, n);
	//float *xpk = envelope_block->data; //get pointer to the array of (empty) data values

	//calculate gain
	audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
	if (!gain_block) return;
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
	Serial.println(F(" AudioEffectCompWDRC: Prefix = ") + prefix);
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
	if (mode_char != ID_char) return false;

	//we ignore the chan_char and only work with the data_char
	bool return_val = true;  //assume that we will find this character
	switch (data_char) {    
		case 'a':
			incrementAttack(time_incr_fac);
			break;
		case 'A':
			incrementAttack(1.0f/time_incr_fac);
			break;
		case 'r':
			incrementRelease(time_incr_fac);
			break;
		case 'R':
			incrementRelease(1.0f/time_incr_fac);
			break;
		case 'm':
			incrementMaxdB(1.0);
			break;
		case 'M':
			incrementMaxdB(-1.0);
			break;
		case 'x':
			incrementExpCR(cr_fac);
			break;
		case 'X':
			incrementExpCR(-cr_fac);
			break;
		case 'z':
			incrementExpKnee(knee_fac);
			break;
		case 'Z':
			incrementExpKnee(-knee_fac);
			break;
		case 'g':
			incrementGain_dB(gain_fac);
			break;
		case 'G':
			incrementGain_dB(-gain_fac);
			break;
		case 'c':
			incrementCompRatio(cr_fac);
			break;
		case 'C':
			incrementCompRatio(1./cr_fac);
			break;
		case 'k':
			incrementKnee(knee_fac);
			break;
		case 'K':
			incrementKnee(-knee_fac);
			break;
		case 'l':
			incrementLimiter(1.0);
			break;
		case 'L':
			incrementLimiter(-1.0);
			break;
		default:
			return_val = false;  //we did not process this character
	}

	
	return return_val;	

}

