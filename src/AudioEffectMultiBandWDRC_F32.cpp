
#include "AudioEffectMultiBandWDRC_F32.h"

float AudioEffectMultiBandWDRC_Base_F32_UI::setSampleRate_Hz(float rate_Hz) {
	sample_rate_Hz = rate_Hz;

	//set the sample rate and block size for the individual classes held by this class
	
	// WARNING!  This does not change the sample rate of already-created filters.  
	// You'll have to call the filter redesign method yourself
	//
	//filterbank:  //uses samplerate and blocksize when designing filters, so we'll use these values at that time
	
	//set the other classes
	compbank.setSampleRate_Hz(sample_rate_Hz);
	broadbandGain.setSampleRate_Hz(sample_rate_Hz); //this algorithm doesn't care, but it does have this method, so we'll call it just in case
	compBroadband.setSampleRate_Hz(sample_rate_Hz);
	
	return sample_rate_Hz;
}

void AudioEffectMultiBandWDRC_Base_F32_UI::update(void) {
     
  //get the input audio
  audio_block_f32_t *block_in = AudioStream_F32::receiveReadOnly_f32();
  if (block_in == NULL) return;
  
  //get the output audio
  audio_block_f32_t *block_out=allocate_f32();
  if (block_out == NULL) { AudioStream_F32::release(block_in); return;}  //there was no memory available
  
  // process audio
  int any_error = processAudioBlock(block_in, block_out);

  //if good, transmit the processed block of audio
  if (!any_error) AudioStream_F32::transmit(block_out); 
  
  //release the memory blocks
  AudioStream_F32::release(block_out);
  AudioStream_F32::release(block_in);
  
} // close update()


int AudioEffectMultiBandWDRC_Base_F32_UI::processAudioBlock(audio_block_f32_t *block_in, audio_block_f32_t *block_out) {
  //check validity of inputs
  int ret_val = -1;
  if ((block_in == NULL) || (block_out == NULL)) return ret_val;  //-1 is error

  //return if not enabled
  if (!is_enabled) return -1;

  //get a temporary working block for audio
  audio_block_f32_t * block_tmp = AudioStream_F32::allocate_f32();
  if (block_tmp == NULL) return ret_val;  //there was no memory available 

  //  /////////////////////////////////////////  loop over all the channels to do the per-band processing
  bool were_any_blocks_processed = false;
  AudioFilterbankBase_F32 *fb = getFilterbank();
  int n_filters = fb->get_n_filters();
  bool firstChannelProcessed = true;
  for (int Ichan = 0; Ichan < n_filters; Ichan++) {
	AudioFilterBase_F32 *filter = fb->getFilter(Ichan);
	
	if (filter->get_is_enabled()) {
	  //apply the filter
	  int any_error = filter->processAudioBlock(block_in,block_tmp);
	  
	  if (!any_error) {
		//apply the compressor
		any_error = compbank.compressors[Ichan].processAudioBlock(block_tmp,block_tmp);
		
		if (!any_error) {              
		  //success!
		  were_any_blocks_processed = true;  //if any one channel processes OK, this whole method will return OK

		  //now we mix the processed signal with the other bands that have been processed
		  if (firstChannelProcessed) {   
						 
			// First channel. Just copy it into the output
			for (int i=0; i < block_in->length; i++) block_out->data[i] = block_tmp->data[i];
			block_out->id = block_in->id;   block_out->length = block_in->length;
			firstChannelProcessed=false; //next time, we can't use this branch of code and we'll use the branch below
			
		  } else {  
						  
			// Later channels.  Must sum this channel with the previous channels
			arm_add_f32(block_out->data, block_tmp->data, block_out->data, block_out->length);  
		
		  }
		} else { // if(!any_error) for compbank
		  //Serial.println(F("AudioEffectMultiBandWDRC_F32_UI: update: error in compbank chan ") + String(Ichan));
		} 
	  } else { // if(!any_error) for filterbank
		//Serial.println(F("AudioEffectMultiBandWDRC_F32_UI: update: error in filterbank chan  ") + String(Ichan));
	  }
	} else {  //if filter is enabled
	  //Serial.print(F("AudioEffectMultiBandWDRC_F32_UI: update: filter is not enabled: Ichan = ")); Serial.println(Ichan);
	}
	
  } //close the loop over channels
	

  //release the temporary memory block
  AudioStream_F32::release(block_tmp);

  //  ////////////////////////////////////////////////////  now do broadband processing
  if (were_any_blocks_processed) {
	
	//apply some broadband gain (could probably be included in the compressor below
	broadbandGain.processAudioBlock(block_out, block_out);

	//apply final broadband compression
	int any_error = compBroadband.processAudioBlock(block_out, block_out);

	//if it got this far without an error, it means that the data processed OK!!
	if (!any_error) ret_val = 0;
  }

  //return
  return ret_val;

} // close processAudioBlock()	



void AudioEffectMultiBandWDRC_Base_F32_UI::getDSL(BTNRH_WDRC::CHA_DSL *new_dsl) {
	//get settings from the filterbank
	int n_filters = getFilterbank()->get_n_filters();
	int n_cross_freq = n_filters-1;  //there is always one fewer crossover frequency than there are filters (because it's the crossover *between* filters)
	getFilterbank()->state.get_crossover_freq_Hz(new_dsl->cross_freq,n_cross_freq); //copy into new_dsl
	
	//get the settings from the compressor bank
	compbank.collectParams_into_CHA_DSL(new_dsl);
}

void AudioEffectMultiBandWDRC_Base_F32_UI::getWDRC(BTNRH_WDRC::CHA_WDRC *new_bb) {
	compBroadband.collectParams_into_CHA_WDRC(new_bb);
}

// ///////////////////////////////////////////////////////////////////////////////////
//
// Here are the methods specific to FIR or Biquad filterbanks
//
// ////////////////////////////////////////////////////////////////////////////////////

void AudioEffectMultiBandWDRC_F32_UI::setupFromBTNRH(BTNRH_WDRC::CHA_DSL &this_dsl,BTNRH_WDRC::CHA_WDRC &this_bb, int n_filt_order) {
  //set the per-channel filter coefficients (using our filterbank class)
  Serial.println("AudioEffectMultiBandWDRC_F32_UI: setupFromBTNRH: channels = " + String(this_dsl.nchannel) + ", filt order = " + String(n_filt_order));
  filterbank.designFilters(this_dsl.nchannel, n_filt_order, sample_rate_Hz, audio_block_samples, (float *)this_dsl.cross_freq);

  //setup all of the per-channel compressors (using our compressor bank class)
  compbank.configureFromDSLandGHA(sample_rate_Hz, this_dsl, this_bb);

  //setup the broad band compressor (typically used as a limiter)
  //configureBroadbandWDRCs(settings.sample_rate_Hz, this_bb, compBroadband);
  compBroadband.configureFromGHA(sample_rate_Hz, this_bb);
}

void AudioEffectMultiBandWDRC_IIR_F32_UI::setupFromBTNRH(BTNRH_WDRC::CHA_DSL &this_dsl,BTNRH_WDRC::CHA_WDRC &this_bb, int n_filt_order) {
  //set the per-channel filter coefficients (using our filterbank class)
  filterbank.designFilters(this_dsl.nchannel, n_filt_order, sample_rate_Hz, audio_block_samples, (float *)this_dsl.cross_freq);

  //setup all of the per-channel compressors (using our compressor bank class)
  compbank.configureFromDSLandGHA(sample_rate_Hz, this_dsl, this_bb);

  //setup the broad band compressor (typically used as a limiter)
  //configureBroadbandWDRCs(settings.sample_rate_Hz, this_bb, compBroadband);
  compBroadband.configureFromGHA(sample_rate_Hz, this_bb);
}

// //////////////////////////////////////////////////////////////////////////////////////
//
// Here are the TympanRemote App GUI related pages FOR STEREO OPERATION
//
// ///////////////////////////////////////////////////////////////////////////////////////


TR_Page* StereoContainerWDRC_UI::addPage_filterbank(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Filterbank");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->getFilterbankUI())->addCard_crossoverFreqs(page_h);
    (rightWDRC->getFilterbankUI())->addCard_crossoverFreqs(page_foo_h); //filterbank might track which GUI elements have been invoked.  This triggers that automatic behavior
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressorbank_globals(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Compressor Bank, Global Parameters");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compbank).addCard_attack_global(page_h);
    (leftWDRC->compbank).addCard_release_global(page_h);
    (leftWDRC->compbank).addCard_scaleFac_global(page_h);

    (rightWDRC->compbank).addCard_attack_global(page_foo_h);   //compbank might track which GUI elements have been invoked.  This triggers that automatic behavior
    (rightWDRC->compbank).addCard_release_global(page_foo_h);
    (rightWDRC->compbank).addCard_scaleFac_global(page_foo_h);
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressorbank_perBand(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Compressor Bank");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compbank).addCard_chooseMode(page_h);
    (leftWDRC->compbank).addCard_persist_perChan(page_h);

    (rightWDRC->compbank).addCard_chooseMode(page_foo_h);   //compbank might track which GUI elements have been invoked.  This triggers that automatic behavior
    (rightWDRC->compbank).addCard_persist_perChan(page_foo_h);
  }

  return page_h;
}

TR_Page* StereoContainerWDRC_UI::addPage_compressor_broadband(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Broadband Compressor");
  TR_Page page_foo; TR_Page *page_foo_h = &page_foo;
  if (page_h == NULL) return NULL;
  
  addCard_chooseChan(page_h); //see StereoContainer_UI.h
  if ((leftWDRC != NULL) && (rightWDRC != NULL)) {
    (leftWDRC->compBroadband).addCards_allParams(page_h);
    (rightWDRC->compBroadband).addCards_allParams(page_foo_h);  //compBroadband might track which GUI elements have been invoked.  This triggers that automatic behavior
  }

  return page_h;
}

