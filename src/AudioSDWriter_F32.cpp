
#include "AudioSDWriter_F32.h"

void AudioSDWriter_F32::prepareSDforRecording(void) {
  if (current_SD_state == STATE::UNPREPARED) {
	if (buffSDWriter) {
	  buffSDWriter->init(); //part of SDWriter, which is the base for BufferedSDWriter_I16
	  if (PRINT_FULL_SD_TIMING) buffSDWriter->setPrintElapsedWriteTime(true); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz
	}
	current_SD_state = STATE::STOPPED;
  }
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
		if (serial_ptr) serial_ptr->println("AudioSDWriter: clear: not in correct state to start.");
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
				if (serial_ptr) serial_ptr->println("AudioSDWriter: start: Cannot do more than 999 files.");
				done = true; //don't loop again
			} //close if (recording_count)
				
		} //close the while loop

	} else {
		//SD subsystem is in the wrong state to start recording
		if (serial_ptr) serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
		return_val = -1;
	}
	
	return return_val;
}

int AudioSDWriter_F32::startRecording(char* fname) {
  int return_val = 0;
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
		serial_ptr->print("AudioSDWriter: start: Failed to open ");
		serial_ptr->println(fname);
	  }
	  return_val = -1;
	}
  } else {
	if (serial_ptr) serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
	return_val = -1;
  }
  return return_val;
}

void AudioSDWriter_F32::stopRecording(void) {
  if (current_SD_state == STATE::RECORDING) {
	//if (serial_ptr) serial_ptr->println("stopRecording: Closing SD File...");

	//close the file
	close(); current_SD_state = STATE::STOPPED;
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
		Serial.print("AudioSDWriter: chan "); Serial.print(Ichan);
		Serial.print(", data skip? This ID = "); Serial.print(audio_blocks[Ichan]->id);
		Serial.print(", Previous ID = "); Serial.println(last_audio_block_id[Ichan]);
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