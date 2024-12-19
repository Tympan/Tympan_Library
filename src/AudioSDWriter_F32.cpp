

#include "AudioSDWriter_F32.h"	


int AudioSDWriter_F32::setWriteDataType(WriteDataType type) {
	Print *serial_ptr = &Serial1;
	int write_nbytes = DEFAULT_SDWRITE_BYTES;

	//get info from previous objects
	if (buffSDWriter) {
		serial_ptr = buffSDWriter->getSerial();
		write_nbytes = buffSDWriter->getWriteSizeBytes();
	}

	//make the full method call
	return setWriteDataType(type, serial_ptr, write_nbytes);
}

int AudioSDWriter_F32::setWriteDataType(WriteDataType type, Print* serial_ptr, const int writeSizeBytes, const int bufferLength_samps) {
	stopRecording();
	writeDataType = type;
	if (!buffSDWriter) {
		if (!sd) {
			sd = new SdFs();
		}
		
		//Serial.println("AudioSDWriter_F32: setWriteDataType: creating buffSDWriter...");
		buffSDWriter = new BufferedSDWriter(sd, serial_ptr, writeSizeBytes);
		if (buffSDWriter) {
			buffSDWriter->setNChanWAV(numWriteChannels);
			if (bufferLength_samps >= 0) {
				allocateBuffer(bufferLength_samps); //leave empty for default buffer size
			} else {
				//if we don't allocateBuffer() here, it simply lets BufferedSDWrite create it last-minute
			}
		} else {
			Serial.print("AudioSDWriter_F32: setWriteDataType: *** ERROR *** Could not create buffered SD writer.");
		}
	}
	if (buffSDWriter == NULL) { return -1; } else { return 0; };
}

void AudioSDWriter_F32::prepareSDforRecording(void) {
	if (current_SD_state == STATE::UNPREPARED) {
		if (buffSDWriter) {
			buffSDWriter->init(); //part of SDWriter, which is the base for BufferedSDWriter_I16
			if (PRINT_FULL_SD_TIMING) buffSDWriter->setPrintElapsedWriteTime(true); //for debugging.  
		}
		current_SD_state = STATE::STOPPED;
	}
}

void AudioSDWriter_F32::end(void) {
	stopRecording();
	sd->end();
	current_SD_state = STATE::UNPREPARED;
}

int AudioSDWriter_F32::deleteAllRecordings(void) {
	//loop through all file names and erase if existing
	int return_val = 0;

	//check to see if the SD has been initialized
	if (current_SD_state == STATE::UNPREPARED) prepareSDforRecording();

	//check to see if SD is ready
	if (current_SD_state == STATE::STOPPED) {
		bool done = false;
		
		recording_count = 0; //restart at first recording
		while ((!done) && (recording_count < 998)) {
			//increment file counter
			recording_count++;
	
			//make file name
			char fname[] = "AUDIOxxx.WAV";
			int hundreds = recording_count / 100;
			fname[5] = hundreds + '0';  //stupid way to convert the number to a character
			int tens = (recording_count - (hundreds*100)) / 10;  //truncates
			fname[6] = tens + '0';  //stupid way to convert the number to a character
			int ones = recording_count - (tens * 10) - (hundreds*100);
			fname[7] = ones + '0';  //stupid way to convert the number to a character

			//does the file exist?
			if (buffSDWriter->exists(fname)) {
				//remove the file
				if (serial_ptr) serial_ptr->print("AudioSDWriter: removing ");
				if (serial_ptr) serial_ptr->println(fname);
				buffSDWriter->remove(fname);
			} else {
				//do nothing
				//done = true; //stop stepping through the files
			}
		}
			
		//set recording counter to start from zero
		recording_count = 0; 
		
	} else {
		//SD subsystem is in the wrong state to start recording
		if (serial_ptr) serial_ptr->println(F("AudioSDWriter: clear: not in correct state to start."));
		return_val = -1;
	}
	

	return return_val;
		
}

//int AudioSDWriter_F32::startRecording_noOverwrite(void) {
int AudioSDWriter_F32::startRecording(void) {	  //make this the default "startRecording"
	int return_val = 0;

	//check to see if the SD has been initialized
	if (current_SD_state == STATE::UNPREPARED) prepareSDforRecording();

	//check to see if SD is ready
	if (current_SD_state == STATE::STOPPED) {
		bool done = false;
		while ((!done) && (recording_count < 998)) {
			
			recording_count++;
			if (recording_count < 1000) {
				//make file name
				char fname[] = "AUDIOxxx.WAV";
				int hundreds = recording_count / 100;
				fname[5] = hundreds + '0';  //stupid way to convert the number to a character
				int tens = (recording_count - (hundreds*100)) / 10;  //truncates
				fname[6] = tens + '0';  //stupid way to convert the number to a character
				int ones = recording_count - (tens * 10) - (hundreds*100);
				fname[7] = ones + '0';  //stupid way to convert the number to a character

				//does the file exist?
				if (buffSDWriter->exists(fname)) {
					//loop around again
					done = false;
				} else {
					//open the file
					return_val = startRecording(fname);
					done = true;
				}
			} else {
				if (serial_ptr) serial_ptr->println(F("AudioSDWriter: start: Cannot do more than 999 files."));
				done = true; //don't loop again
			} //close if (recording_count)
				
		} //close the while loop

	} else {
		//SD subsystem is in the wrong state to start recording
		if (serial_ptr) serial_ptr->println(F("AudioSDWriter: start: not in correct state to start."));
		return_val = -1;
	}
	
	return return_val;
}

int AudioSDWriter_F32::startRecording(const char* fname) {
  int return_val = 0;
  
  //check to see if the SD has been initialized
  if (current_SD_state == STATE::UNPREPARED) prepareSDforRecording();
  
  if (current_SD_state == STATE::STOPPED) {
	//try to open the file on the SD card
	if (openAsWAV(fname)) { //returns TRUE if the file opened successfully
	  if (serial_ptr) {
		serial_ptr->print("AudioSDWriter: Opened ");
		serial_ptr->println(fname);
	  }
	  
	  //start the queues.  Then, in the serviceSD, the fact that the queues
	  //are getting full will begin the writing
	  buffSDWriter->resetBuffer();
	  current_SD_state = STATE::RECORDING;
	  setStartTimeMillis();
	  current_filename = String(fname);
	  
	} else {
	  if (serial_ptr) {
		serial_ptr->print(F("AudioSDWriter: start: Failed to open "));
		serial_ptr->println(fname);
	  }
	  return_val = -1;
	}
  } else {
	if (serial_ptr) serial_ptr->println(F("AudioSDWriter: start: not in correct state to start."));
	return_val = -1;
  }
  return return_val;
}

void AudioSDWriter_F32::stopRecording(void) {
  __disable_irq();
  if (current_SD_state == STATE::RECORDING) {
	current_SD_state = STATE::STOPPED;
	__enable_irq();
	
	//close the file
	//if (serial_ptr) serial_ptr->println("stopRecording: Closing SD File...");
	close(); 
	current_filename = String("Not Recording");

	//clear the buffer
	if (buffSDWriter) buffSDWriter->resetBuffer();
  }
}

//update is called by the Audio processing ISR.  This update function should
//only service the recording queues so as to buffer the audio data.
//The acutal SD writing should occur in the loop() as invoked by a service routine
void AudioSDWriter_F32::update(void) {
	audio_block_f32_t *audio_blocks[AUDIOSDWRITER_MAX_CHAN];
	//bool data_present[AUDIOSDWRITER_MAX_CHAN];
	bool any_data_present = false;

	//get the audio
	for (int Ichan=0; Ichan < numWriteChannels; Ichan++) {
		audio_blocks[Ichan] = receiveReadOnly_f32(Ichan); //get the data block

		//did we get any audio for this channel?
		if (audio_blocks[Ichan]) any_data_present = true;
	}
  
	//fill in any missing data blocks
	if (any_data_present) {  //only fill in missing data channels if there is any data at all in any of the channels
		for (int Ichan=0; Ichan < numWriteChannels; Ichan++) {   //loop over channels
			if (!audio_blocks[Ichan]) { //is it empty (null)
				//no audio!  create blank data.
				audio_block_f32_t *block = allocate_f32();
				if (block) {
					//fill with zeros
					for (int I=0; I < block->length; I++) block->data[I] = 0.0;
					//add to stack of data blocks
					block->id = 0;
					audio_blocks[Ichan] = block;  
				}
			}
		}
	}

	//copy the audio to the big write buffer
	if (current_SD_state == STATE::RECORDING) {
		//if (buffSDWriter) buffSDWriter->copyToWriteBuffer(audio_blocks,numWriteChannels);
		copyAudioToWriteBuffer(audio_blocks, numWriteChannels);
	}

	//release the audio blocks
	for (int Ichan=0; Ichan < numWriteChannels; Ichan++) {
		if (audio_blocks[Ichan]) AudioStream_F32::release(audio_blocks[Ichan]);
	}   
}

void AudioSDWriter_F32::copyAudioToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan) {
  static unsigned long last_audio_block_id[AUDIOSDWRITER_MAX_CHAN];
  if (numChan == 0) return;
  bool chan_received[numChan];
  
  //do any of the given audio blocks actually contain data
  int any_data = 0, nsamps = 0;
  for (int Ichan = 0; Ichan < numChan; Ichan++) {
		if (audio_blocks[Ichan]) { //looking for anything NOT null
			any_data++;  //this audio_block[Ichan] is not null, so count it
			chan_received[Ichan] = true;
			nsamps = audio_blocks[Ichan]->length; //how long is it?
			//Serial.print("SDWriter: copyToWriteBuffer: good "); Serial.println(Ichan);
		} else {
			chan_received[Ichan] = false;
		}
  }
  if (any_data == 0) return;  //if there's no data, return;
  if (any_data < numChan) { // do we have all the channels?  If not, send error?
    Serial.print("AudioSDWriter: copyToWriteBuffer: only got "); Serial.print(any_data);
    Serial.print(" of ");  Serial.print(numChan);  Serial.print(" channels: ");
		for(int Ichan = 0; Ichan < numChan; Ichan++) { Serial.print(chan_received[Ichan]); }
		Serial.println();
		return;  //return if not all data is present
  }

  //check to see if there have been any jumps in the data counters
  for (int Ichan = 0; Ichan < numChan; Ichan++) {
		if (audio_blocks[Ichan] != NULL) {
			if (((audio_blocks[Ichan]->id - last_audio_block_id[Ichan]) != 1) && (last_audio_block_id[Ichan] != 0)) {
				//Serial.print("AudioSDWriter: chan "); Serial.print(Ichan);
				//Serial.print(", data skip? This ID = "); Serial.print(audio_blocks[Ichan]->id);
				//Serial.print(", Previous ID = "); Serial.println(last_audio_block_id[Ichan]);
			}
			last_audio_block_id[Ichan] = audio_blocks[Ichan]->id;
		}
  }

  //data looks good, prep for handoff
  float32_t *ptr_audio[numChan] = {}; //the braces cause it to init all to zero (null)
  for (int Ichan = 0; Ichan < numChan; Ichan++) { 
		if (audio_blocks[Ichan]) {
			ptr_audio[Ichan] = audio_blocks[Ichan]->data;
		} else {
			//create a block, fill with zeros and store it
		}
  }

  //now push it into the buffer via the base class BufferedSDWriter
  if (buffSDWriter) buffSDWriter->copyToWriteBuffer(ptr_audio,nsamps,numChan);
}



int AudioSDWriter_F32::serviceSD_withWarnings(AudioInputI2S_F32 &i2s_in) {
  int bytes_written = serviceSD_withWarnings();
  if ( bytes_written > 0 ) checkMemoryI2S(i2s_in);
  return bytes_written;
}
int AudioSDWriter_F32::serviceSD_withWarnings(AudioInputI2SQuad_F32 &i2s_in) {
  int bytes_written = serviceSD_withWarnings();
  if ( bytes_written > 0 ) checkMemoryI2S(i2s_in);
  return bytes_written;
}
int AudioSDWriter_F32::serviceSD_withWarnings(AudioInputI2SHex_F32 &i2s_in) {
  int bytes_written = serviceSD_withWarnings();
  if ( bytes_written > 0 ) checkMemoryI2S(i2s_in);
  return bytes_written;
}

int AudioSDWriter_F32::serviceSD_withWarnings(void) {

  unsigned long dT_millis = millis();  //for timing diagnotstics
  //int bytes_written = audioSDWriter.serviceSD();
  int bytes_written = serviceSD(); 
  dT_millis = millis() - dT_millis;  //timing calculation

  if ( bytes_written > 0 ) {
    
		//if slow, issue warning
    bool criteria1 = (dT_millis > 150);
		uint32_t samples_filled = buffSDWriter->getNumSampsInBuffer();
		uint32_t buffer_len = buffSDWriter->getLengthOfBuffer();
		uint32_t samples_empty = buffer_len - samples_filled;
		float samples_per_second = buffSDWriter->getNChanWAV() * buffSDWriter->getSampleRateWAV();
		float remaining_time_sec = ((float)samples_empty) / samples_per_second;
  	float buffer_empty_frac = ((float)samples_empty)/((float)buffer_len);
		//bool criteria2 = (dT_millis > 20) && (buffer_fill_frac > 0.7f);
    bool criteria2 = ((dT_millis > 25) && (remaining_time_sec < 0.100));
		if (criteria1 || criteria2) {
			Serial.print("AudioSDWriter_F32: Warning: long write: ");
			Serial.print(dT_millis); Serial.print(" msec");
			Serial.print(" for "); Serial.print(bytes_written); Serial.print(" bytes");
			//Serial.print(" at "); Serial.print(((float)bytes_written)/((float)(dT_millis)),1);Serial.print(" kB/sec");
			Serial.print(", buffer is " ); Serial.print(buffSDWriter->getNumSampsInBuffer());
			Serial.print("/"); Serial.print(buffSDWriter->getLengthOfBuffer());
			Serial.print(" used = "); Serial.print(100.0f*buffer_empty_frac, 2); Serial.print("% open");
			Serial.print(" = ");Serial.print((int)(remaining_time_sec*1000)); Serial.print(" msec remain.");
			Serial.println();
    }
  }
  return bytes_written;
}     

void AudioSDWriter_F32::checkMemoryI2S(AudioInputI2S_F32 &i2s_in) {
    //print a warning if there has been an SD writing hiccup

	//if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
	if (i2s_in.get_isOutOfMemory()) {
		float approx_time_sec = ((float)(millis()-getStartTimeMillis()))/1000.0;
		if (approx_time_sec > 0.1) {
		  Serial.print("SD Write Warning: there was a hiccup in the writing. ");//  Approx Time (sec): ");
		  Serial.println(approx_time_sec);
		}
	}
  i2s_in.clear_isOutOfMemory();
}

void AudioSDWriter_F32::checkMemoryI2S(AudioInputI2SQuad_F32 &i2s_in) {
	//print a warning if there has been an SD writing hiccup

	//if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
	if (i2s_in.get_isOutOfMemory()) {
		float approx_time_sec = ((float)(millis()-getStartTimeMillis()))/1000.0;
		if (approx_time_sec > 0.1) {
			Serial.print("SD Write Warning: there was a hiccup in the writing. ");//  Approx Time (sec): ");
			Serial.println(approx_time_sec);
		}
	}
	i2s_in.clear_isOutOfMemory();
}
void AudioSDWriter_F32::checkMemoryI2S(AudioInputI2SHex_F32 &i2s_in) {
	//print a warning if there has been an SD writing hiccup

	//if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
	if (i2s_in.get_isOutOfMemory()) {
		float approx_time_sec = ((float)(millis()-getStartTimeMillis()))/1000.0;
		if (approx_time_sec > 0.1) {
			Serial.print("SD Write Warning: there was a hiccup in the writing. ");//  Approx Time (sec): ");
			Serial.println(approx_time_sec);
		}
	}
	i2s_in.clear_isOutOfMemory();
}




// ////////////////////////////////////////////// Implement the UI methods

void AudioSDWriter_F32_UI::printHelp(void) {
	String prefix = getPrefix();  //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(F(" AudioSDWriter: Prefix = ") + prefix);
	Serial.println(F("   r,s,d: SD record/stop/deleteAll")); 
};


bool AudioSDWriter_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	bool return_val = false;
	if (mode_char != ID_char) return return_val; //does the mode character match our ID character?  if so, it's us!

	//we ignore the chan_char and only work with the data_char
	return_val = true;  //assume that we will find this character
	switch (data_char) {    
		case 'r':
			Serial.println("AudioSDWriter_F32_UI: begin SD recording");
			startRecording(); 			//AudioSDWriter_F32 method
			setSDRecordingButtons();	//update the GUI buttons
			break;
		case 's':
			Serial.println("AudioSDWriter_F32_UI: stop SD recording");
			stopRecording(); 			//AudioSDWriter_F32 method
			setSDRecordingButtons();	//update the GUI buttons
			break;
		case 'd':
			Serial.println("AudioSDWriter_F32_UI: deleting all recordings");
		    stopRecording();			//AudioSDWriter_F32 method
			deleteAllRecordings();		//AudioSDWriter_F32 method
			setSDRecordingButtons();    //update the GUI buttons
			break;
		default:
			return_val = false;  //we did not process this character
	}
	return return_val;		
};

void AudioSDWriter_F32_UI::setFullGUIState(bool activeButtonsOnly) {
	setSDRecordingButtons(activeButtonsOnly);
}
void AudioSDWriter_F32_UI::setSDRecordingButtons(bool activeButtonsOnly) {
	if (getState() == AudioSDWriter_F32::STATE::RECORDING) {
		setButtonState("recordStart",true);
	} else {
		setButtonState("recordStart",false);
	}
	setButtonText("sdFname",getCurrentFilename());
};

TR_Card* AudioSDWriter_F32_UI::addCard_sdRecord(TR_Page *page_h) {
	return addCard_sdRecord(page_h, getPrefix());
}
TR_Card* AudioSDWriter_F32_UI::addCard_sdRecord(TR_Page *page_h, String prefix) {
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard(F("Audio to SD Card (") + String(numWriteChannels) + " Chan)");
	if (card_h == NULL) return NULL;
	
	card_h->addButton("Record", prefix+"r", "recordStart", 6);  //label, command, id, width
	card_h->addButton("Stop",  prefix+"s", "",            6);  //label, command, id, width
	card_h->addButton("",      "",         "sdFname",     12); //label, command, id, width  //display the filename
	return card_h;
}

TR_Page* AudioSDWriter_F32_UI::addPage_sdRecord(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("SD Audio Writer");
  if (page_h == NULL) return NULL;
  addCard_sdRecord(page_h);
  return page_h;

}






