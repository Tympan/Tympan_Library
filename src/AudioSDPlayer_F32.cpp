/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 *  Extended by Chip Audette, OpenAudio, Dec 2019
 *  Converted to F32 and to variable audio block length
 *  The F32 conversion is under the MIT License.  Use at your own risk.
 *
 * Heaviy refactored to remove SD reading from interupts.
 * Chip Audette (Open Audio), March 2024, 
 * All changes are MIT License, Use at your own risk.
 * 
*/

#include <Arduino.h>
#include "AudioSDPlayer_F32.h"

//#include <spi_interrupt.h>

//#include <SD.h> //used by isSdCardPresent()
#define SD_PLAYER_DEBUG_PRINTING		(false)	// Additional print statements to aid in debugging.

#define STATE_DIRECT_8BIT_MONO    0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO  1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO   2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO 3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO   4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO 5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO  6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO  7  // playing stereo, converting sample rate
#define STATE_PARSE1      8  // looking for 20 byte ID header
//#define STATE_PARSE2      9  // looking for 16 byte format header
//#define STATE_PARSE3      10 // looking for 8 byte data header
//#define STATE_PARSE4      11 // ignoring unknown chunk after "fmt "
//#define STATE_PARSE5      12 // ignoring unknown chunk before "fmt "
#define STATE_STOP      13
#define STATE_NOT_BEGUN  99

#define READ_STATE_NORMAL  1
#define READ_STATE_FILE_EMPTY 2

#define SD_CONFIG SdioConfig(FIFO_SDIO) //is this correct for both Teensy 3 and Teensy 4?

constexpr uint16_t MAX_WAV_PLAYBACK_CHAN = 2;		// Sets the maximum # of WAV channels


unsigned long AudioSDPlayer_F32::update_counter = 0;

void AudioSDPlayer_F32::init(void) {
  state = STATE_NOT_BEGUN;
}
void AudioSDPlayer_F32::begin(void)
{
  if (state == STATE_NOT_BEGUN) {
	if (sd_ptr == NULL) {
		//Serial.println("AudioSDPlayer_F32: creating new SdFs.");
		sd_ptr = new SdFs();
	}
    if (!(sd_ptr->begin(SD_CONFIG))) {
      Serial.println("AudioSDPlayer_F32: cannot open SD.");
      return;
    }
  }
  
  state = STATE_STOP;
  state_play = STATE_STOP;
  state_read = READ_STATE_NORMAL;
  data_length = 0;
/*   if (block_left_f32) {
    AudioStream_F32::release(block_left_f32);
    block_left_f32 = NULL;
  }
  if (block_right_f32) {
    AudioStream_F32::release(block_right_f32);
    block_right_f32 = NULL;
  } */

  update_counter = 0;
}

bool AudioSDPlayer_F32::sendFilenames(void) {
	if (state == STATE_NOT_BEGUN) begin();
	return sd_ptr->ls(); //add LS_R to make recursive.  otherwise, just does the local directory
}


// Here is the "open" function.  It is rare that the user will want to call this directly.
// Basically, only use this if you are going to stream the data out over Serial rather than
// playing the audio in real time.
bool AudioSDPlayer_F32::open(const char *filename, const bool flag_preload_buffer) {
	bool okFlag = true;

	// --- Clear buffer for parsing header ---
	wavAudioFormat = Wave_Format_e::Wav_Format_Uninitialized;
	memset(&riffChunk.byteStream[0], 0, sizeof(Riff_Header_u) );
	memset(&fmtPcmChunk.byteStream[0], 0, sizeof(Fmt_Pcm_Header_u) );
	memset(&fmtIeeeChunk.byteStream[0], 0, sizeof(Fmt_Ieee_Header_u) );
	memset(&fmtExtChunk.byteStream[0], 0, sizeof(Fmt_Ext_Header_u) );
	memset(&factChunk.byteStream [0], 0, sizeof(Fact_Header_u) );
	memset(&listChunk.byteStream [0], 0, sizeof(List_Tag_Header_u) );
	memset(&dataChunk.byteStream [0], 0, sizeof(Data_Header_u) );
	infoTagStr.clear();	// clear map of info list tag values

	if (state == STATE_NOT_BEGUN) begin();
  	//stop();
	
	active = false;  //from AudioStream.h.  Setting to false to prevent update() from being called.  Only for open().
	closeFile();

	__disable_irq();
	okFlag = file.open(filename,O_READ);   //open for reading
	__enable_irq();
	if (!okFlag || !isFileOpen()) {	
		return false;
	}
	
	// Store filename for reference later
	_filename = filename;
	
	//buffer_length = 0;
	state_play = STATE_STOP;
	data_length = file.size(); //set to a number bigger than zero to enable the readHeader() to really read some bytes
	
	// Check file size
	if (file.size() <=0 ) {
		okFlag = false;

	} else {
		state = STATE_PARSE1;
	
		//read header ... added Chip Audette March 2024
		buffer_read = 0;
		buffer_write = 0;
		state_read = READ_STATE_NORMAL;
		okFlag = readHeader();
		
		//pre-fill the buffer?
		if ( okFlag && flag_preload_buffer) {
			debugPrint("AudioSDPlayer_F32::open:fillBufferFromSD");

			auto errCode = fillBufferFromSD();
			if(errCode < 0) {
				debugPrint("***Error*** fillBufferFromSD: Failed to prefill SD Buffer: Error:");
				debugPrint(String(errCode));
				okFlag = false;
			} else {
				debugPrint(String("Filled # of bytes: ") + String( getNumBytesInBuffer() ) );
			}
		} // else skip prefilling buffer.
	}

  return okFlag;
}

//"play" is the usual way of opening a file and playing it.
bool AudioSDPlayer_F32::play(const char *filename)
{
	bool isOpen = open(filename,true); //the "true" tells it to fill the read buffer from the SD right now
	
	//now, activate the instance so that update() will be called
	active = true;  //in AudioStream.h.  Activates this instance so that update() gets called
	
	return isOpen;
}

//Play an already-opened file
bool AudioSDPlayer_F32::play(void) {
	if (isFileOpen()) {
		debugPrint("AudioSDPlayer_F32: file was already open.  Starting playing");
		active = true;  //in AudioStream.h.  Activates this instance so that update() gets called
	} else {
		Serial.println("AudiOSDPlayer_F32: play: cannot play file because no file has been opened.");
		active = false;
	}
	return active;
}

void AudioSDPlayer_F32::stop(void)
{
  //if (state == STATE_NOT_BEGUN) begin();
	if (state == STATE_NOT_BEGUN) return;

  __disable_irq();
  if (state != STATE_STOP) {
    state = STATE_STOP;
	__enable_irq();
    //audio_block_f32_t *b1 = block_left_f32;
    //block_left_f32 = NULL;
    //audio_block_f32_t *b2 = block_right_f32;
    //block_right_f32 = NULL;


    //if (b1) AudioStream_F32::release(b1);
    //if (b2) AudioStream_F32::release(b2);

  } else {
    __enable_irq();
  }
	
	closeFile();

}

// --- Get Functions for returning reference to WAV header properties ---
/**
 * @brief Get the Riff Header object
 * 
 * @return Riff_s& 
 */
const Riff_s& AudioSDPlayer_F32::getRiffHeader(void) {
	return riffChunk.S;
};

/**
 * @brief Get the Wav Format Type
 * 
 * @return const Wave_Format_e& Reference to WAV format, as determined by WAV header
 */
const Wave_Format_e& AudioSDPlayer_F32::getWavFormatType(void) {
	return wavAudioFormat;
}

/**
 * @brief Get the Fmt Header object, only valid if getWavFormatType() == Wave_Format_e::Wav_Format_Pcm_Float
 * @return Fmt_Pcm_s
 */
const Fmt_Pcm_s& AudioSDPlayer_F32::getFmtPcmHeader(void) {
	return fmtPcmChunk.S;
}

/**
 * @brief Get the Fmt Header object, only valid if getWavFormatType() == Wave_Format_e::Wav_Format_Ieee_Float
 * @return Fmt_Ieee_s
 */
const Fmt_Ieee_s& AudioSDPlayer_F32::getFmtIeeeHeader(void) {
	return fmtIeeeChunk.S;
}

/**
 * @brief Get the Fmt Header object, only valid if getWavFormatType() == Wave_Format_e::Wav_Format_Extensible
 * @return Fmt_Ext_s
 */
const Fmt_Ext_s& AudioSDPlayer_F32::getFmtExtHeader(void) {
	return fmtExtChunk.S;
}

/**
 * @brief Get the Fact Header object, only valid if getWavFormatType() == Wave_Format_e::Wav_Format_Ieee_Float
 * @return Fact_s 
 */
const Fact_s& AudioSDPlayer_F32::getFactHeader(void) {
	return factChunk.S;
}

/**
 * @brief Get the LIST<INFO> tag strings by tag ID
 * @return InfoKeyVal_t 
 */
const std::string AudioSDPlayer_F32::getInfoTag(const Info_Tags tagId) {
	if ( infoTagStr.count(tagId) > 0 ) {
		return(infoTagStr[tagId]);
	} else {
		return ("");
	}
}

/**
 * @brief Get a vector of LIST<INFO> tagIDs 
 * \note Useful for the calling `getInfoTag()`
 */
const std::vector<Info_Tags> AudioSDPlayer_F32::getInfoTagIds(void) {
	std::vector<Info_Tags> idList;
	
	for (const auto &id:infoTagStr) {
		idList.push_back(id.first);
	}

	return (idList);
}


/**
 * @brief print WAV header information and metadata comment
 */
void AudioSDPlayer_F32::printMetadata(void) {
	// Check if filename is empty
	if ( !_filename.empty() ){
		Serial.print( String(_filename.c_str() ) );

		switch(wavAudioFormat) {
			case Wave_Format_e::Wav_Format_Pcm:
				Serial.println(" (PCM Audio)");
				Serial.println( String("\t# of chan: \t\t ") + String(fmtPcmChunk.S.numChan) ); 
				Serial.println( String("\tSamp Rate (Hz): \t ") + String(fmtPcmChunk.S.sampleRate_Hz) ); 
				Serial.println( String("\tBits Per Sample Rate: \t ") + String(fmtPcmChunk.S.bitsPerSample) ); 
				break;

			case Wave_Format_e::Wav_Format_Ieee_Float:
				Serial.println(" (IEEE Audio)");
				Serial.println( String("\t# of chan: \t ") + String(fmtIeeeChunk.S.numChan) ); 
				Serial.println( String("\tSamp Rate (Hz): \t ") + String(fmtIeeeChunk.S.sampleRate_Hz) ); 
				Serial.println( String("\tBits Per Sample Rate: \t ") + String(fmtIeeeChunk.S.bitsPerSample) ); 
				break;

			case Wave_Format_e::Wav_Format_Extensible:
				Serial.println(" (Extensible Audio)");
				Serial.println( String("\t# of chan: \t ") + String(fmtExtChunk.S.numChan) ); 
				Serial.println( String("\tSamp Rate (Hz): \t ") + String(fmtExtChunk.S.sampleRate_Hz) ); 
				Serial.println( String("\tBits Per Sample Rate: \t ") + String(fmtExtChunk.S.bitsPerSample) ); 
				break;

			default:
			Serial.println(" (Format not supported)");
		}

		// Print metadata comments
		if ( !infoTagStr.empty() ) {
			Serial.println("Metadata: ");
			for (const auto& tagItem : infoTagStr) {

				std::string tagStr = std::string( InfoTagToStr(tagItem.first) );
				Serial.println(String("\t") + String( tagStr.c_str() ) + String (": ") + String( (tagItem.second).c_str() ) );
			}
		}
	}
}



void AudioSDPlayer_F32::update(void) {
	
	// only update if we're playing
	if ((state == STATE_STOP) || (state == STATE_NOT_BEGUN)) return;

	// allocate the audio blocks to transmit...and return early if none can be allocated
	audio_block_f32_t *block_left_f32 = AudioStream_F32::allocate_f32();
	if (block_left_f32 == NULL) return;
	audio_block_f32_t *block_right_f32 = AudioStream_F32::allocate_f32();
	if (block_right_f32 == NULL) {
		AudioStream_F32::release(block_left_f32);
		return;
	}

	//fill the audio blocks from the buffered audio
	const int n_filled = readFromBuffer(block_left_f32->data, block_right_f32->data, audio_block_samples);

	//fill any unfilled samples in he audio blocks with zeros
	if (n_filled < audio_block_samples) {
		for (int i = n_filled; i < audio_block_samples; i++) {
			block_left_f32->data[i] = 0.0f;
			block_right_f32->data[i] = 0.0f;
		}
	}
  
	//final updates to the audio data blocks
	update_counter++;
	block_left_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	block_right_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	block_left_f32->length = audio_block_samples; 
	block_right_f32->length = audio_block_samples; 

	//transmit and release
	AudioStream_F32::transmit(block_left_f32, 0);   //send out as channel 0 (ie, left) of this audio class
	AudioStream_F32::release(block_left_f32);
	AudioStream_F32::transmit(block_right_f32, 1);  //send out as channel 1 (ie, right) of this audio class  
	AudioStream_F32::release(block_right_f32);

	//check to see if we're completely out of data
	if ((data_length == 0) || ((state_read == READ_STATE_FILE_EMPTY) && (n_filled == 0)) ) {
	 	state = STATE_STOP;
		debugPrint("AudioSDPlayer_F32::update: End of audio data");
		debugPrint( String( "\tAudio data remaining: ") + String(data_length) );
		debugPrint( String( "\tFile Size: ") + String(total_length) );
		debugPrint( String( "\tCurrent file position: ") + String( file.curPosition() ) );
	}
}

uint32_t AudioSDPlayer_F32::readFromBuffer(float32_t *left_f32, float32_t *right_f32, int n_samps) {
	//loop through the buffer and copy samples to the output arrays
	uint32_t n_samples_read = 0;
	switch (state_play) {
		case STATE_DIRECT_16BIT_MONO: case STATE_DIRECT_16BIT_STEREO:
			n_samples_read = readBuffer_16bit_to_f32(left_f32, right_f32, n_samps, channels); 		
			break;
		default:
			//file format not accounted for...so just do nothing and (later) code will auto-fill with zeros
			break;
	}
	return n_samples_read;
}

//copy bytes from the main circular buffer into a buffer provided by the user
//so that the user can independtly do something with the bytes (such as transmit
//them back to the PC via Serial or whatever)
uint32_t AudioSDPlayer_F32::readRawBytes(uint8_t *out_buffer, const uint32_t n_bytes_wanted) {
	if (n_bytes_wanted == 0) return 0;
	uint32_t n_bytes_filled = 0;
	
	//ensure that there is some data in the buffer (if any is available)
	if ((state_read == READ_STATE_NORMAL) && (data_length > 0)) readFromSDtoBuffer(MAX_READ_SIZE_BYTES);
	
	//try to copy bytes
	while ((n_bytes_filled < n_bytes_wanted) && (getNumBuffBytes() > 0) && (data_length > 0)) {

		//fill as many bytes as possible from the buffer, until we've hit our
		//target number of bytes are until there aren't any bytes left
		uint32_t n_to_copy = min(getNumBuffBytes(), n_bytes_wanted - n_bytes_filled);
		uint32_t fill_target = n_bytes_filled + n_to_copy;
		while ((data_length > 0) && (n_bytes_filled < fill_target)) {
			out_buffer[n_bytes_filled++] = buffer[buffer_read++];       //copy the byte
			if (buffer_read >= N_BUFFER) buffer_read=0;  //wrap around, if needed

			//keep track of using up the allowed audio data within the WAV file (there can be extraneous 
			//non-audio data at the end of a WAV file).  We just used two bytes.
			data_length--; //we just used one byte
		}
	
		//ensure that there is some data in the buffer (if any is available)
		if ((state_read == READ_STATE_NORMAL) && (data_length > 0)) readFromSDtoBuffer(MAX_READ_SIZE_BYTES);
		
	}
			
		
	//if we didn't get all the bytes that we wanted, fill the rest with zeros
	if (n_bytes_filled < n_bytes_wanted) {
		uint32_t foo_index = n_bytes_filled;
		while (foo_index < n_bytes_wanted) out_buffer[foo_index++] = 0;  //fill with zeros
	}
	
	//return the number of bytes of real data
	return n_bytes_filled;
}
uint32_t AudioSDPlayer_F32::readBuffer_16bit_to_f32(float32_t *left_f32, float32_t *right_f32, uint16_t n_samps_wanted, uint16_t n_chan) {
	uint32_t n_samples_read = 0;
	const uint32_t bytes_perSamp_allChans = 2 * (uint32_t)n_chan; //mono will need 2 bytes (16 bits) per sample and stereo will need 2*2 bytes (32 bits) per sample
	const uint32_t bytes_desired = bytes_perSamp_allChans*n_samps_wanted;
	uint8_t lsb, msb;
	int16_t val_int16;
	float32_t val_f32;
	static unsigned long last_warning_millis = 0;

	
	//check to see if we have enough data in the main buffer.  If not, try to load one read-block of data
	if ((state_read == READ_STATE_NORMAL) && ( (getNumBuffBytes() < bytes_desired) || (data_length < bytes_desired))) {
		if (0) {  //do we allow ourselves to read from the SD during this high-speed update()-driven interrupt?
			Serial.println("AudioSDPlayer_F32 :readBuffer_16bit_to_f32: reading from SD in the high-speed loop.  Danger!");
			readFromSDtoBuffer(MIN_READ_SIZE_BYTES);
		} else {
			//just issue a warning
			if ((bytes_desired > getNumBuffBytes()) && (bytes_desired < data_length)) {
				if ((millis() < last_warning_millis) || (millis() > last_warning_millis+50)) { //limit to a warning every 50 msec
					Serial.println("AudioSDPlayer_F32: readBuffer_16bit_to_f32: insufficient data in RAM buffer.");
					last_warning_millis = millis();
				}
			}
		}
	}
		
	//loop through all desired samples, filing as many samples as possible, until we've hit our
	//target number of samples are until there aren't any samples left
	while ( (n_samples_read < n_samps_wanted) && (getNumBuffBytes() >= bytes_perSamp_allChans) ) { //do we have enough 
		if (data_length >= bytes_perSamp_allChans) { //do we have enough bytes of *allowed* WAV data
			for (int Ichan=0; Ichan < n_chan; Ichan++) {		
				//get the lsb and msb, assuming they form a 16-bit value
				lsb = buffer[buffer_read++]; if (buffer_read >= N_BUFFER) buffer_read=0;  //wrap around, if needed
				msb = buffer[buffer_read++]; if (buffer_read >= N_BUFFER) buffer_read=0;  //wrap around, if needed

				//keep track of using up the allowed audio data within the WAV file (there can be extraneous 
				//non-audio data at the end of a WAV file).  We just used two bytes.
				data_length -= 2;  
				
				//form the 16-bit value and convert to float
				val_int16 = (msb << 8) | lsb;
				val_f32 = ((float32_t)val_int16) / 32767.0;
				
				//push the value into the left or right output
				if (Ichan==0) {
					left_f32[n_samples_read]=val_f32;
				} else {
					right_f32[n_samples_read]=val_f32;
				}
			} //end loop over channels
			n_samples_read++; //acknowledge that we just filled a sample slot (filling both left+right counts as 1 sample)
			
		} else {
			//we've used up all the allowed data that we've read.
			//discard any fractional bytes remaining, just to be clear that we're done.
			data_length = 0;
			buffer_read = buffer_write;
		}
	}
	
	return n_samples_read;
}

//how many valid (waiting-to-be-used) bytes are already in the circular buffer
uint32_t AudioSDPlayer_F32::getNumBuffBytes(void) {
	uint32_t n_buff = 0;
	if (buffer_read == buffer_write) {
		n_buff = 0;  //buffer_read == buffer_write is assumed to be an empty circular buffer?
	} else if (buffer_read > buffer_write) {
		n_buff = N_BUFFER - buffer_read + buffer_write;
	} else {
		n_buff = buffer_write - buffer_read;
	}
	return n_buff;
}

int AudioSDPlayer_F32::serviceSD(void) {
	return fillBufferFromSD();
	//return readFromSDtoBuffer(MAX_READ_SIZE_BYTES);
}

// Fill the circular buffer from the SD (should leave one block empty, however)
//This function should be called from the Arduino loop() function.
int AudioSDPlayer_F32::fillBufferFromSD(void) {
	//This function loads data from the SD card into a RAM buffer, as
	//long as there's room in the buffer to do the read

	//should we be reading?  If not, return.
	if (state == STATE_STOP) return 0;

	//keep reading until the buffer is full (or an error occurs)
	int error_code = 0; //assume no error at first
	uint32_t actual_space_in_buffer = N_BUFFER - getNumBuffBytes();  //how much space is in the circular buffer
	const uint32_t min_required_space_in_buffer_after_reads =  1;    //don't allow ourself to completely fill the buffer...we cannot have read and write pointers land together
	uint32_t space_to_fill_in_buffer = actual_space_in_buffer - min_required_space_in_buffer_after_reads;

	//if ((state_read != READ_STATE_FILE_EMPTY) && (file.available()) && (space_to_fill_in_buffer >= MIN_READ_SIZE_BYTES)) {
	//	Serial.println("AudioSDPlayer_F32: fillBufferFromSD: space_to_fill_in_buffer = " + String(space_to_fill_in_buffer));
	//}
	
	while ((error_code == 0) && (space_to_fill_in_buffer > MIN_READ_SIZE_BYTES)) { //only continue of there is space
		//set the read size to the maximum allowed
		uint16_t n_bytes_to_read = MAX_READ_SIZE_BYTES;
		
		//if this read size is larger than the space available, reduce read size to fit
		while ((n_bytes_to_read > space_to_fill_in_buffer) && (n_bytes_to_read > MIN_READ_SIZE_BYTES)) n_bytes_to_read -= MIN_READ_SIZE_BYTES;

		//read the bytes from the SD into the circular buffer
		error_code = readFromSDtoBuffer(n_bytes_to_read);
		
		//update our assessment of the fullness of the circular buffer...which prepares us to loop around for the test in the "while"
		actual_space_in_buffer = N_BUFFER - getNumBuffBytes();
		space_to_fill_in_buffer = actual_space_in_buffer - min_required_space_in_buffer_after_reads;
	}

	return error_code;
}

/* int AudioPlayer_F32::fillBufferFromSD_longReads(void) {
	//This function should be called from the Arduino loop() function.
	//This function loads data from the SD card into a RAM buffer, as
	//long as there's room in the buffer to do the read

	//should we be reading?  If not, return.
	if ((state == STATE_STOP) || (state_read == READ_STATE_FILE_EMPTY)) return 0;
	
	//read the first leg
	uint8_t *start = buffer + buffer_write;
	uint32_t n_bytes_to_read = 0;
	if (buffer_write >= buffer_read) {
		n_bytes_to_read = N_BUFFER - buffer_write;
	} else {
		n_bytes_to_read = buffer_read - buffer_write;
	}
	
		//stopped writing code here
	
	return 0; //fix this
}	
} */


//Read bytes from the SD directly to the circular buffer.
//Currently, I've limited it to only reading 2^16 (ie, 64K) bytes so that I can
//return negative values as error codes.  This is a dumb reason, but it's what I did.
int AudioSDPlayer_F32::readFromSDtoBuffer(const uint16_t n_bytes_requested) {
	if (state_read == READ_STATE_FILE_EMPTY) return -1;
	if (file.available() == false) return -1;
	if (n_bytes_requested == 0) return 0;
	
	//initialize
	bool file_has_data = true; //assume at first that the file does have data
	
	//is there enough space for at least a minimum read operation?
	uint32_t space_in_circular_buffer = N_BUFFER - getNumBuffBytes();
	const uint32_t min_required_space_in_buffer_after_reads =  1;    //don't allow ourself to completely fill the buffer...we cannot have read and write pointers land together
	uint32_t allowed_space_in_circular_buffer = space_in_circular_buffer - min_required_space_in_buffer_after_reads; 
	if (n_bytes_requested >= MIN_READ_SIZE_BYTES) {
		if (allowed_space_in_circular_buffer < MIN_READ_SIZE_BYTES) return 0; //not enough space!
	} else {
		if (allowed_space_in_circular_buffer == 0) return 0; //not enough space!
	}
	
	//choose how many bytes to actually read
	uint16_t bytes_remaining = min(n_bytes_requested,allowed_space_in_circular_buffer); //here at the start, we have to read all the bytes
	if (bytes_remaining > MIN_READ_SIZE_BYTES) {
		//make multiple of MIN_READ_SIZE_BYTES
		uint16_t new_bytes_remaining = MIN_READ_SIZE_BYTES; //this should 
		while ((new_bytes_remaining < bytes_remaining) && (new_bytes_remaining <= (65535-MIN_READ_SIZE_BYTES-1))) new_bytes_remaining += MIN_READ_SIZE_BYTES;
		if (new_bytes_remaining > bytes_remaining) new_bytes_remaining -= MIN_READ_SIZE_BYTES;
		bytes_remaining = new_bytes_remaining;
	}		
			
	//loop until all the data is read (or the file runs out)
	while ((bytes_remaining > 0) && (file_has_data)) { 
		//how much to read
		uint16_t n_bytes_to_read = min(MAX_READ_SIZE_BYTES,bytes_remaining);
		if (buffer_write >= buffer_read) {
			//we can read to the end of the circular buffer, if we want
			uint32_t n_bytes_to_end_of_buffer = N_BUFFER - buffer_write;
			n_bytes_to_read = min(n_bytes_to_read,n_bytes_to_end_of_buffer);
		} else {
			//we can read up to the read pointer, if we want
			uint32_t n_bytes_to_read_pointer = buffer_read - buffer_write;
			n_bytes_to_read = min(n_bytes_to_read,n_bytes_to_read_pointer);
		}
			
		//read the bytes from the SD into a temporary linear buffer
		uint8_t *start_ptr = buffer + buffer_write;
    	elapsedMicros usec = 0;
		int16_t bytes_read = file.read(start_ptr, n_bytes_to_read);
    	unsigned long dT_millis = usec/1000UL;  //timing calculation
		buffer_write += bytes_read; if (buffer_write >= N_BUFFER) buffer_write = 0; //wrap around as needed
		if (bytes_read != n_bytes_to_read) file_has_data = false;
			
			
		//if slow, issue warning
		const bool criteria1 = (dT_millis > 150);
		const uint32_t bytes_filled = getNumBuffBytes();
		const uint32_t buffer_len = getBufferLengthBytes();
		const float bytes_per_second = channels * sample_rate_Hz * bits/8;  //2 bytes per sample for int16 data in the WAV
		const float remaining_time_sec = ((float)bytes_filled) / bytes_per_second;
  	const float buffer_fill_frac = ((float)bytes_filled)/((float)buffer_len);
		//bool criteria2 = (dT_millis > 20) && (buffer_fill_frac < 0.3f);
		const bool criteria2 = ((dT_millis > 25) && (remaining_time_sec < 0.100));
		if (criteria1 || criteria2) {  //if the read took a while, print some diagnostic info
			Serial.print("AudioSDPlayer_F32: Warning: long read: ");
			Serial.print(dT_millis); Serial.print(" msec");
			Serial.print(" for "); Serial.print(bytes_read); Serial.print(" bytes");
			//Serial.print(" at "); Serial.print(((float)bytes_read)/((float)(dT_millis)),1);Serial.print(" kB/sec");
			Serial.print(", buffer is " ); Serial.print(getNumBuffBytes());
			Serial.print("/"); Serial.print(getBufferLengthBytes());
			Serial.print(" = "); Serial.print(100.0f*buffer_fill_frac, 2); Serial.print("% filled");
			Serial.print(" = ");Serial.print((int)(remaining_time_sec*1000)); Serial.print(" msec remain.");
      Serial.println();
		}
			
		//prepare for next loop
		bytes_remaining -= bytes_read;
	}
	
	//did the file run out of data?
	if (file_has_data == false) {
		file.close();
		state_read = READ_STATE_FILE_EMPTY;
	}
	
	return 0;  //normal return
}
		
		
//Read bytes from the SD to the temp buffer and then to the full circular buffer.
//Currently, I've limited it to only reading 2^16 (ie, 64K) bytes so that I can
//return negative values as error codes.  This is a dumb reason, but it's what I did.
/*
int AudioSDPlayer_F32::readFromSDtoBuffer_old(const uint16_t n_bytes_requested) {
	if (state_read == READ_STATE_FILE_EMPTY) return -1;
	if (file.available() == false) return -1;
	if (n_bytes_requested == 0) return 0;
	
	//initialize
	bool file_has_data = true; //assume at first that the file does have data
	uint32_t space_in_circular_buffer = N_BUFFER - getNumBuffBytes(); // how much to read
	uint16_t bytes_remaining = min(n_bytes_requested,space_in_circular_buffer); //here at the start, we have to read all the bytes
		
	//loop until all the data is read (or the file runs out)
	while ((bytes_remaining >= MIN_READ_SIZE_BYTES) && (file_has_data)) { //by using ">", this should never completely fill the buffer, which is good...the read_ptr and write_ptr should only equal each other if the buffer is empty
		//how much to read
		uint16_t n_bytes_to_read = min(MAX_READ_SIZE_BYTES,bytes_remaining); 
	
		//read the bytes from the SD into a temporary linear buffer
		int16_t bytes_read = file.read(temp_buffer, n_bytes_to_read);
		if (bytes_read != n_bytes_to_read) file_has_data = false;
		
		//copy the bytes from the temporary linear buffer into the full circular buffer
		for (int16_t i=0; i <  bytes_read; i++) {
			buffer[buffer_write++] = temp_buffer[i];
			if (buffer_write >= N_BUFFER) buffer_write = 0; //wrap around as needed
		}
		
		//prepare for next loop
		bytes_remaining -= bytes_read;
	}
	
	//did the file run out of data?
	if (file_has_data == false) {
		file.close();
		state_read = READ_STATE_FILE_EMPTY;
	}
	
	return 0;  //normal return
}
*/

/* 
  // we only get to this point when buffer[512] is empty
  if (state != STATE_STOP && file.available()) {
    // we can read more data from the file...
    readagain:
    buffer_length = file.read(buffer, 512);  //read 512 bytes  (128 samples, 2 channels, 2 bytes/sample
	
	//Serial.println("AudioSDPlayer_F32: update: after file read, buffer_length = " + String(buffer_length));
	//Serial.println("AudioSDPlayer_F32: update: after file read, state = " + String(state));
	
    if (buffer_length == 0) goto end;
    buffer_read = 0;
    bool parsing = (state >= STATE_PARSE1);
	//Serial.println("AudioSDPlayer_F32: update: calling consume...");
    bool txok = consume(buffer_length);
    if (txok) {
      if (state != STATE_STOP) return;
    } else {
      if (state != STATE_STOP) {
        if (parsing && state < STATE_PARSE1) goto readagain;
        else goto cleanup;
      }
    }
  }
end:  // end of file reached or other reason to stop
	//Serial.println("AudioSDPlayer_F32: update(): in 'end' section");
  file.close(); 
  state_play = STATE_STOP;
  state = STATE_STOP;
cleanup:
	//Serial.println("AudioSDPlayer_F32: update(): in 'cleanup' section");
  if (block_left_f32) {
    if (block_offset > 0) {
      for (uint32_t i=block_offset; i < ((uint32_t)audio_block_samples); i++) {
        block_left_f32->data[i] = 0.0f;
      }
      update_counter++; //let's increment the counter here to ensure that we get every ISR resulting in audio
      block_left_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
      block_left_f32->length = audio_block_samples;
      AudioStream_F32::transmit(block_left_f32, 0);
      if (state < STATE_PARSE1 && (state & 1) == 0) {
        AudioStream_F32::transmit(block_left_f32, 1);
      }
    }
    AudioStream_F32::release(block_left_f32);
    block_left_f32 = NULL;
  }
  if (block_right_f32) {
    if (block_offset > 0) {
      for (uint32_t i=block_offset; i < (uint32_t)audio_block_samples; i++) {
        block_right_f32->data[i] = 0.0f;
      }
      block_right_f32->id = update_counter; //prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
      block_right_f32->length = audio_block_samples;
      AudioStream_F32::transmit(block_right_f32, 1);
    }
    AudioStream_F32::release(block_right_f32);
    block_right_f32 = NULL;
  }
}
 */

// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

// Consume already buffered data.  Returns true if audio transmitted.
//bool AudioSDPlayer_F32::consume(uint32_t size)


/**
 * @brief Reads and parses WAV header.  Leaves file pointer at beginning of audio data. 
 * \note Requires file to be open and available.  Reads directly from file, and does not use AudioSDPlayer_F32' ring buffer
 * 	After parsing, sets the following
 * 		total_length: remaining bytes in file;
 * 		data_length: remaining audio bytes;
 * 		state_read: READ_STATE_NORMAL;
		state = state_play;
 * @return true Success
 * @return false Error, check "err" for Wav_Header_Err error # 
 */
bool AudioSDPlayer_F32::readHeader(void) {
	Wav_Header_Err err = Wav_Header_Err::ok;
	uint32_t chunkId = 0; 	// Temp buffer for storing chunk ID
	uint32_t chunkLen = 0;	// Temp buffer for storing chunk LED

	// --- Clear buffer for parsing header ---
	wavAudioFormat = Wave_Format_e::Wav_Format_Uninitialized;
	memset(&riffChunk.byteStream[0], 0, sizeof(Riff_Header_u) );
	memset(&fmtPcmChunk.byteStream[0], 0, sizeof(Fmt_Pcm_Header_u) );
	memset(&fmtIeeeChunk.byteStream[0], 0, sizeof(Fmt_Ieee_Header_u) );
	memset(&fmtExtChunk.byteStream[0], 0, sizeof(Fmt_Ext_Header_u) );
	memset(&factChunk.byteStream [0], 0, sizeof(Fact_Header_u) );
	memset(&listChunk.byteStream [0], 0, sizeof(List_Tag_Header_u) );
	memset(&dataChunk.byteStream [0], 0, sizeof(Data_Header_u) );
	infoTagStr.clear();	// clear map of info list tag values

	debugPrint("AudioSDPlayer_F32:readHeader: begin parsing header.");
  	

	// Check correct state for parsing wav header
	if (state != STATE_PARSE1) {
		err = Wav_Header_Err::invalid_state;
		Serial.println("\t***Error*** unknown state = " + String(state));

	// Else If WAV file has no header, bypass this method.
	} else if ( (state == STATE_DIRECT_16BIT_MONO) || (state == STATE_DIRECT_16BIT_STEREO) ) {
		return true;
	}

	// Check that file handle is valid
	if ( !file || !file.isOpen() ) {
		Serial.println( "\t***Error*** file not instantiated.");
		err = Wav_Header_Err::invalid_file;
	}


 	// --- RIFF Chunk---
	if(err == Wav_Header_Err::ok ) {
		debugPrint("\tParsing RIFF chunk.");
		err = parseRiffChunk(file, riffChunk);
	}
	

	/* --- FMT CHUNK --- */
	// Read in "fmt " chunk ID and Length, to determine if audio format is 16-bit PCM or 32-bit IEEE
	if (err == Wav_Header_Err::ok) {
		err = peekHeaderChunk(file, chunkId, chunkLen);
	
		// Check "fmt " chunk ID is invalid
		if (chunkId != FMT_LSB) {
			err = Wav_Header_Err::chunk_id_not_found;
			debugPrint("\t***Error*** Chunk ID invalid.");
			debugPrint( String("\tExpected: '_fmt'; Received: ") + String(chunkId) );

		// Else If length matches PCM format....
		} else if (chunkLen == sizeof(Fmt_Pcm_Header_u) - 8) {		// - 8 to exclude chunk ID and length
			debugPrint("\tParsing fmt PCM chunk.");
			err = parseFmtPcmChunk(file, fmtPcmChunk);

			// Set which audio format this header uses
			if (err == Wav_Header_Err::ok) {
				wavAudioFormat = Wave_Format_e::Wav_Format_Pcm;
			} // else let err pass thru
	
		// Else if length matches IEEE format
		} else if ( chunkLen == sizeof(Fmt_Ieee_Header_u) - 8 ) {		// - 8 to exclude chunk ID and length
			debugPrint("\tParsing fmt IEEE chunk.");
			err = parseFmtIeeeChunk(file, fmtIeeeChunk);

			// Check IEEE audio format
			if (err == Wav_Header_Err::ok) {
				// Set which audio format this header uses
				wavAudioFormat = Wave_Format_e::Wav_Format_Ieee_Float;

				/*---  Parse FACT chunk --- */
				debugPrint("\tParsing FACT chunk.");
				err = peekHeaderChunk(file, chunkId, chunkLen);
				
				// If FACT chunk found, parse it.  The RIFF standard is vague as to whether this is required.
				if ( (err == Wav_Header_Err::ok) && (chunkId == FACT_LSB) && 
						(chunkLen == sizeof(Fact_Header_u) - 8) ) {	// Subtract 8 for chunk ID and chunk Len variables.
					err = parseFactChunk(file, factChunk);
				}
			} // else let err pass thru
		
		// Else if length matches WAV_FORMAT_EXTENSIBLE (40 bytes)
		} else if ( chunkLen == sizeof(Fmt_Ext_Header_u) - 8 ) {		// - 8 to exclude chunk ID and length
			debugPrint("\tParsing fmt Extensible chunk.");
			err = parseFmtExtChunk(file, fmtExtChunk);
			
			if (err == Wav_Header_Err::ok) {
				// Set which audio format this header uses
				wavAudioFormat = Wave_Format_e::Wav_Format_Extensible;

				/*---  Parse FACT chunk --- */
				debugPrint("\tParsing FACT chunk.");
				err = peekHeaderChunk(file, chunkId, chunkLen);

				// If FACT chunk found, parse it.  The RIFF standard is vague as to whether this is required.
				if ( (err == Wav_Header_Err::ok) && (chunkId == FACT_LSB) && 
						(chunkLen == sizeof(Fact_Header_u) - 8) ) {	// Subtract 8 for chunk ID and chunk Len variables.
					err = parseFactChunk(file, factChunk);
				}
			} // else let err pass thru

		// Else length does not match PCM or IEEE format.  Could be incompatible `extensible format`.
		} else {
			debugPrint("\t***Error*** Invalid fmt format. Chunk Length: " + String(chunkLen) );
			err= Wav_Header_Err::invalid_file_format;
		}
	}


	/* --- FIND DATA or LIST CHUNK --- */
	// Remaining header may consist of "data" or "INFO" chunk, in any order
	// Does not search entire file. Jumps from chunk to chunk based on stated chunk length
	// Ignores chunks it does not recognize.
	bool dataChunkFound = false;
	uint32_t audioDataFilePos = 0;		// For storing position of data chunk to rewind to at the end of this method.
	uint8_t maxNumChunks = 10;			// Maximum # of chunks to jump to, in case there is some erratic behavior

	// Parse while no error and not EOF
	while ( (err == Wav_Header_Err::ok) && ( (file.curPosition()+8) < file.size() ) ) { // Add 8 to read chunk ID and len

		// Clear temp buffers
		chunkId = 0;
		chunkLen = 0;
		bool chunkIdFound = false;
		size_t maxShift = 3;		// max # of bytes to shift while looking for chunk ID
		
		// Peek at chunk ID and len
		while ((err==Wav_Header_Err::ok) && (!chunkIdFound) ) {
			err = peekHeaderChunk(file, chunkId, chunkLen);

			// Is valid chunk?			
			chunkIdFound = isValidChunkId(chunkId);
				
			// If not, shift one byte
			if (!chunkIdFound) {
				debugPrint("Shifting: "+ String(chunkId, HEX));
				if (maxShift > 0) {
					maxShift--;
					if (!file.seekCur(1) ) {
						// if failed to seek, seek to end of file, ignoring error
						file.seekEnd();
					}
				// Else still not found; seek to end of file. ignoring error
				} else {
					debugPrint("No valid chunk ID found. Skipping to end of file.");
					file.seekEnd();
				}
			} // else chunk found... let it pass thru
		}

		if (err == Wav_Header_Err::ok) {
			// Try to identify chunk
			switch(chunkId) {
				
				// Read LIST chunk
				case LIST_LSB:
					debugPrint("\tAudioSDPlayer_F32:readHeader: parseListChunk()");
					err = parseListChunk(file, listChunk, infoTagStr);
					if (err ==  Wav_Header_Err::ok) {
						debugPrint("\t\t-- Success!");
					}
				break; 

				// Read Data chunk
				case DATA_LSB:
					/* --- DATA CHUNK --- */
					debugPrint("\tAudioSDPlayer_F32:readHeader: parsing beginnning of data chunk");
					// Read beginning of data header, and stop at the start of the audio data
					if( file.read( &dataChunk.byteStream[0], sizeof(Data_Header_u) ) != sizeof(Data_Header_u) ) {
						err = Wav_Header_Err::file_read_err;
					
					// Else check that audio data starts on even byte
					} else if ( (file.curPosition() % 2) != 0 ) {
						err = Wav_Header_Err::audio_data_odd_pos;
					} else {
						// data chunk found. Record start of audio position
						debugPrint(String("\t\t-- Success! Position of audio data: ") + file.curPosition() );
						audioDataFilePos = file.curPosition();
						dataChunkFound = true;

						// Seek to end of audio data (should not hit EOF as there should be room for another chunk)
						if( !file.seekCur(dataChunk.S.chunkLenBytes) ) {
							debugPrint( String("AudioSDPlayer_F32:readHeader:seekCur***Error***; filePos: ") + file.curPosition() + 
									String("; fileSize: ") + String( file.size() ) );
							err = Wav_Header_Err::file_seek_err;
						}
					}
					break;

				// Ignore chunk and jump over to next chunk
				default:
					debugPrint("Unknown Chunk ID: " + String(chunkId) +
								String("\n\tfilePos: ") + String( file.curPosition() ) + 
								String("\n\tjump # of bytes: ") + String(chunkLen + 8) +
								String("\n\tfileSize: ") + String( file.size()) );
					if( !file.seekCur(chunkLen + 8) ) {	// Add 8 for current id and len bytes
						debugPrint( String("AudioSDPlayer_F32:readHeader:seekCur***Error***; Ignore chunk; filePos: ") + file.curPosition() + 
								String("; fileSize: ") + String( file.size() ) );
						err = Wav_Header_Err::file_seek_err;
					}
			} // end switch()

			// Count down max number of chunks to look for
			if (maxNumChunks > 0) {
				maxNumChunks--;
			} else {
				err = Wav_Header_Err::chunk_id_not_found;
			}
		} // error check for peeking.  If it failed, then let it pass through.
		
	} // end while()

	// If data chunk was found, seek to beginning of audio data
	if (dataChunkFound) {
		if( !file.seekSet(audioDataFilePos) ){
			debugPrint(String("\tAudioSDPlayer_F32:readHeader:Set file position to start of audio: ") + String(audioDataFilePos) ); 
			err = Wav_Header_Err::file_seek_err;
		}
	// Else throw error
	} else {
		debugPrint("\tAudioSDPlayer_F32:readHeader: *** Error *** data chunk not found");
		err = Wav_Header_Err::chunk_id_not_found;
	}
	
	/* --- Set parameters --- */
	if (err == Wav_Header_Err::ok) {
		// Set data_length to # of audio bytes
		data_length = dataChunk.S.chunkLenBytes;
		debugPrint( String("\tAudioSDPlayer_F32:readHeader: Remaining audio data = ") + String(data_length) );

		// Set audio format
		switch (wavAudioFormat) {
			// PCM Audio
			case Wave_Format_e::Wav_Format_Pcm:
				debugPrint("\tAudioSDPlayer_F32:readHeader: Detected PCM audio format");
				if ( (fmtPcmChunk.S.numChan > 0) && 
						(fmtPcmChunk.S.numChan <= MAX_WAV_PLAYBACK_CHAN) ) {
					channels = fmtPcmChunk.S.numChan;
					debugPrint( String("\tAudioSDPlayer_F32:readHeader: numChan = ") + String(channels) );
				} else {
					err = Wav_Header_Err::invalid_num_chan;
				}
				break;
			// IEEE 32-bit Float
			case Wave_Format_e::Wav_Format_Ieee_Float:
				debugPrint("\tAudioSDPlayer_F32:readHeader: Detected IEEE audio format");
				if ( (fmtIeeeChunk.S.numChan > 0) && 
						(fmtIeeeChunk.S.numChan <= MAX_WAV_PLAYBACK_CHAN) ) {
					channels = fmtIeeeChunk.S.numChan;
					debugPrint( String("\tAudioSDPlayer_F32:readHeader: numChan = ") + String(channels) );
				} else {
					err = Wav_Header_Err::invalid_num_chan;
				}
				break;
			// Extensible format
			case Wave_Format_e::Wav_Format_Extensible:
				debugPrint("\tAudioSDPlayer_F32:readHeader: Detected Extensible audio format");
				if ( (fmtExtChunk.S.numChan > 0) && 
						(fmtExtChunk.S.numChan <= MAX_WAV_PLAYBACK_CHAN) ) {
					channels = fmtExtChunk.S.numChan;
					debugPrint( String("\tAudioSDPlayer_F32:readHeader: numChan = ") + String(channels) );
				} else {
					err = Wav_Header_Err::invalid_num_chan;
				}
				break;
			// Error Incompatible file format
			default:
				err = Wav_Header_Err::invalid_file_format;
		}
	}

	// Call legacy parse format
	if (err == Wav_Header_Err::ok) {
		if ( !parse_format() ) {
			err = Wav_Header_Err::invalid_file;
		}
	}

	// Check audio format
	if (err == Wav_Header_Err::ok) {
		switch (wavAudioFormat) {
			// PCM 
			case Wave_Format_e::Wav_Format_Pcm:
				break; // valid, so do nothing

			// IEEE
			case Wave_Format_e::Wav_Format_Ieee_Float:
				// IEEE F32 is invalid for SD Playback, for now.
				err = Wav_Header_Err::IEEE_format_incompatible;
				break;

			// Extensible
			case Wave_Format_e::Wav_Format_Extensible:
				// Check if subFormat field is not PCM audio
				if (fmtExtChunk.S.subFormat != Wave_Format_e::Wav_Format_Pcm) {
					if (fmtExtChunk.S.subFormat == Wave_Format_e::Wav_Format_Ieee_Float) {
						err = Wav_Header_Err::IEEE_format_incompatible;
					} else {
						err = Wav_Header_Err::invalid_file_format;
					}
				} // else PCM format, which is valid
				break;
			
			// Default - Error
			default: 
				debugPrint( String("Bad Audio Format; Code: ") + String(wavAudioFormat) );
				err = Wav_Header_Err::invalid_file_format;
		}
	}

	// Report # of remaining bytes in file
	total_length = file.size() - file.curPosition() - 1;  // "1" accounts for index-0 based position
	debugPrint( String("\tAudioSDPlayer_F32:readHeader: Remaining bytes = ") + String(total_length) );
	
	// Set length of header
	debugPrint( String("\tAudioSDPlayer_F32:readHeader: Header len = ") + String( file.curPosition() ) );
	
	// If error print hex code
	if (err != Wav_Header_Err::ok) {
		state = STATE_STOP;
		state_read = STATE_STOP;

		// Seek to end of file
		file.seekEnd(0);
		Serial.print("AudioSDPlayer_F32: readHeader: *** Error ***: 0x"); 
		Serial.println( (uint16_t) err, HEX);
		return (false);
	
	// Else success!
	} else {
		// Set states
		state = state_play;
		state_read = READ_STATE_NORMAL;
		debugPrint("AudioSDPlayer_F32:readHeader(): Success!" );
		return (true);
	}
}


/**
 * @brief Peek at chunk ID and Length
 * 
 * @param file reference to WAV file
 * @param chhunkId buffer to store chunkId in
 * @param chunkLen buffer to store chunkLen in
 * @return Wav_Header_Err 
 */
 Wav_Header_Err AudioSDPlayer_F32::peekHeaderChunk(SdFile &file, uint32_t &chunkId, uint32_t &chunkLen) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Record current file position
	uint32_t filePos = file.curPosition();

	// Read chunk ID and len	
	uint32_t bytesRead = file.read( &chunkId, sizeof(uint32_t) );
	bytesRead += file.read( &chunkLen, sizeof(uint32_t) );

	if (bytesRead != 8) {
		err = Wav_Header_Err::file_read_err;
	} 
	
	// Seek back to previous file pos
	if ( file.seekSet(filePos)==false ) {
		// If no previous error, store the error
		if (err == Wav_Header_Err::ok) {
			err = Wav_Header_Err::file_seek_err;
		} // else let the previous error pass thru
	}

	return err;
 }


/**
 * @brief Parses WAV header RIFF chunk
 * @param file reference to WAV file
 * @param riff Riff union-struct to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseRiffChunk(SdFile &file, Riff_Header_u &riff) {
	Wav_Header_Err err = Wav_Header_Err::ok;
	
	// Check that file is available and filesize < max U32
	if ( file.available() && (file.size() < UINT32_MAX ) ) {
		
		// Read in RIFF chunk, checking that 12Bytes were returned
		if( file.read(&riff.byteStream[0], sizeof(Riff_Header_u) ) == sizeof(Riff_Header_u) ) {
			
			// Check for "RIFF" and "WAVE"
			if ( (riff.S.chunkId != RIFF_LSB) || (riff.S.format != WAVE_LSB) ) {
				err = Wav_Header_Err::chunk_id_not_found;
				debugPrint("AudioSDPlayer_F32:parseListChunk(): ***Error*** Chunk ID Invalid");
				debugPrint( String("\tExpected: 'RIFF'; Received: ") + String(riff.S.chunkId) );
				debugPrint( String("\tExpected: 'WAVE'; Received: ") + String(riff.S.format) );

			// Check file size according to expected chunk size (+8 bytes as this doesn't include "RIFF" or chunk size variable)
			} else if ( file.size() != (riff.S.chunkLenBytes + 8) ) {	// add 8 bytes for "RIFF" and RIFF length
				err = Wav_Header_Err::invalid_chunk_len;
				Serial.println(  String( "Error: File size (" + String(file.size(), DEC) + "!= " + String (riff.S.chunkLenBytes+8, DEC) ) );
			}
		//ELSE error reading in header (or ran out of bytes)
		} else {
			err = Wav_Header_Err::invalid_file;
		}
	// Error File Unavailable or exceeds max file size
	} else {
		err = Wav_Header_Err::invalid_file;
	}

	return err;
}


/**
 * @brief Parses WAV header "fmt " chunk, if audio format is PCM
 * 
 * @param file reference to WAV file
 * @param fmtPcmChunk union-struct to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseFmtPcmChunk(SdFile &file, Fmt_Pcm_Header_u &fmtPcmChunk) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Read in fmt PCM chunk
	uint32_t bytesRead = file.read( &fmtPcmChunk.byteStream[0], sizeof(Fmt_Pcm_Header_u) );
	
	if ( bytesRead != sizeof(Fmt_Pcm_Header_u) ){
		err = Wav_Header_Err::file_read_err;
	
	// Check audio format is 16-bit PCM audio format
	} else if (fmtPcmChunk.S.audioFmt != Wave_Format_e::Wav_Format_Pcm) {
		err = Wav_Header_Err::invalid_file_format;

	} else if( fmtPcmChunk.S.bitsPerSample != Wave_Bitrate_e::Wav_Bitrate_16) {
		err = Wav_Header_Err::invalid_bit_rate; 
	}

	return err;
}


/**
 * @brief Parses WAV header "fmt " chunk, if audio format is IEEE
 * 
 * @param file reference to WAV file
 * @param fmtIeeeChunk union-struct to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseFmtIeeeChunk(SdFile &file, Fmt_Ieee_Header_u &fmtIeeeChunk) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Read into fmt IEEE chunk
	uint32_t bytesRead = file.read( &fmtIeeeChunk.byteStream[0], sizeof(Fmt_Ieee_Header_u) );

	if ( bytesRead != sizeof(Fmt_Ieee_Header_u) ) {
		err = Wav_Header_Err::file_read_err;
	
	// Check audio format is IEEE and 32-bit float format
	} else if (fmtIeeeChunk.S.audioFmt !=Wave_Format_e::Wav_Format_Ieee_Float) {
		err = Wav_Header_Err::invalid_file_format;

	} else if (fmtIeeeChunk.S.bitsPerSample != Wave_Bitrate_e::Wav_Bitrate_32) {
		err = Wav_Header_Err::invalid_bit_rate;
	}

	return err;
}


/**
 * @brief Parses WAV header "fmt " chunk, if audio format is EXTENSIBLE
 * 
 * @param file reference to WAV file
 * @param fmtExtChunk union-struct to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseFmtExtChunk(SdFile &file, Fmt_Ext_Header_u &fmtExtChunk) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Read into fmt Ext chunk
	uint32_t bytesRead = file.read( &fmtExtChunk.byteStream[0], sizeof(Fmt_Ext_Header_u) );

	if ( bytesRead != sizeof(Fmt_Ext_Header_u) ) {
		err = Wav_Header_Err::file_read_err;
	
	// Check audio format is Ext
	} else if (fmtExtChunk.S.audioFmt !=Wave_Format_e::Wav_Format_Extensible) {
		err = Wav_Header_Err::invalid_file_format;

	// Check that subformat is either PCM or IEEE
	} else if ( (fmtExtChunk.S.subFormat != Wave_Format_e::Wav_Format_Pcm) && 
			(fmtExtChunk.S.subFormat != Wave_Format_e::Wav_Format_Ieee_Float) )  {
		err = Wav_Header_Err::invalid_file_format;
	}
	return err;
}


/**
 * @brief Parses WAV header "fact" chunk, which is used by the IEEE audio format
 * 
 * @param file reference to WAV file
 * @param factChunk union-struct to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseFactChunk(SdFile &file, Fact_Header_u &factChunk) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Read in Fact chunk
	if( file.read( &factChunk.byteStream[0], sizeof(Fact_Header_u) ) != sizeof(Fact_Header_u) ) {
		err = Wav_Header_Err::file_read_err;
	}
	return err;
}


/**
 * @brief Parse LIST<INFO> Chunk
 * 
 * @param file reference to WAV file
 * @param listChunk union-struct to populate
 * @param infoTagStr reference to InfoKeyVal_t Map of <tag, string> to populate
 * @return Wav_Header_Err 
 */
Wav_Header_Err AudioSDPlayer_F32::parseListChunk(SdFile &file, List_Header_u &listChunk, InfoKeyVal_t &infoTagStr, bool checkTagIDFlag) {
	Wav_Header_Err err = Wav_Header_Err::ok;

	// Read in LIST chunk ID and Len
	uint32_t bytesRead = file.read( &listChunk.byteStream[0], sizeof(List_Header_u) );

	// Track total # of bytes left in LIST chunk
	uint32_t bytesLeft = listChunk.S.chunkLenBytes - 4;  // subtract 4 for subchunkID "INFO" 

	// Check for errors
	if ( bytesRead != sizeof(List_Header_u) ) {
		err = Wav_Header_Err::file_read_err;

	} else if ( (listChunk.S.chunkId != LIST_LSB) || (listChunk.S.subChunkId != INFO_LSB) ) {
		err = Wav_Header_Err::chunk_id_not_found;
		debugPrint("AudioSDPlayer_F32:parseListChunk(): ***Error*** Chunk ID Invalid");
		debugPrint( String("\tExpected: LIST; Received: ") + String(listChunk.S.chunkId) );
		debugPrint( String("\tExpected: INFO; Received: ") + String(listChunk.S.subChunkId) );

	} else if ( (listChunk.S.chunkLenBytes <= 0) || (listChunk.S.chunkLenBytes > MAX_CHUNK_LEN) ) {
		err = Wav_Header_Err::invalid_chunk_len;
		debugPrint("AudioSDPlayer_F32:parseListChunk(): ***Error*** LIST Chunk Length Invalid");
		debugPrint( String("\tReceived: ") + String(listChunk.S.chunkLenBytes) );
		debugPrint( String("\tMax: ") + String(MAX_CHUNK_LEN) );
	}

	// Keep reading tag and tag strings until out of bytes (according to LIST chunk header)
	while ( (bytesLeft > sizeof(List_Tag_Header_u) ) && (err == Wav_Header_Err::ok) ) {
		List_Tag_Header_u listTagChunk;
		std::string tagIdStr;	// temp buffer to store tag ID as string

		// Read in tag ID and length; keep track of total bytes read
		bytesRead = file.read( &listTagChunk.byteStream[0], sizeof(List_Tag_Header_u) );
		bytesLeft -= bytesRead;

		// Check that read() worked
		if (bytesRead != sizeof(List_Tag_Header_u) ) {
			err = Wav_Header_Err::file_read_err;
		} else { // success
			// Copy (uint32_t) tagID to string
			tagIdStr.resize( sizeof(uint32_t) );  // set buffer size
			memcpy( tagIdStr.data(), &listTagChunk.S.tagId, sizeof(uint32_t) );
		}

		// If tag ID is not recognized, Error
		if ( (StrToTagId(tagIdStr) == Info_Tags::ERRO) && (checkTagIDFlag) ) {
			err = Wav_Header_Err::chunk_id_not_found;
			debugPrint("AudioSDPlayer_F32:parseListChunk(): ***Error*** LIST<INFO> Tag ID Invalid");
			debugPrint( String("\tReceived: ") + String(tagIdStr.c_str() ) );

		// check if chunk len is valid
		} else if ( (listTagChunk.S.tagLenBytes <=0) || (listTagChunk.S.tagLenBytes > MAX_CHUNK_LEN) ) {
			err = Wav_Header_Err::invalid_chunk_len;
			debugPrint("AudioSDPlayer_F32:parseListChunk(): ***Error*** LIST<INFO> tag length Invalid");
			debugPrint( String("tag: ") + String( tagIdStr.c_str() ) );
			debugPrint( String("\tLength Received: ") + String(listTagChunk.S.tagLenBytes) );
			debugPrint( String("\tMax: ") + String(MAX_CHUNK_LEN) );
		}

		// Assign info string to the map of <Info_Tags, std::string>
		if (err == Wav_Header_Err::ok) {
			// temporary buffer
			std::string tagStrBuff;
			tagStrBuff.resize(listTagChunk.S.tagLenBytes);
			
			// Read tag string
			bytesRead = file.read( tagStrBuff.data(), listTagChunk.S.tagLenBytes );
			bytesLeft -= bytesRead;

			if ( bytesRead != listTagChunk.S.tagLenBytes) {
				err = Wav_Header_Err::file_read_err;
			} else {
				// Insert into map
				infoTagStr[ StrToTagId(tagIdStr)] = tagStrBuff;
				debugPrint( String("\t\tFound LIST<INFO> tag: ") + String(tagIdStr.c_str() ) + String(": ") + String(tagStrBuff.c_str() ) );
			}
		} // else err
	} // end while

	// if there are bytes left in the chunk, seek past them.  for example, could be padded bytes.
	file.seekCur(bytesLeft);

	return err;
}


/*
00000000  52494646 66EA6903 57415645 666D7420  RIFFf.i.WAVEfmt 
00000010  10000000 01000200 44AC0000 10B10200  ........D.......
00000020  04001000 4C495354 3A000000 494E464F  ....LIST:...INFO
00000030  494E414D 14000000 49205761 6E742054  INAM....I Want T
00000040  6F20436F 6D65204F 76657200 49415254  o Come Over.IART
00000050  12000000 4D656C69 73736120 45746865  ....Melissa Ethe
00000060  72696467 65006461 746100EA 69030100  ridge.data..i...
00000070  FEFF0300 FCFF0400 FDFF0200 0000FEFF  ................
00000080  0300FDFF 0200FFFF 00000100 FEFF0300  ................
00000090  FDFF0300 FDFF0200 FFFF0100 0000FFFF  ................
*/





// SD library on Teensy3 at 96 MHz
//  256 byte chunks, speed is 443272 bytes/sec
//  512 byte chunks, speed is 468023 bytes/sec

//define B2M_96000 (uint32_t)((double)4294967296000.0 / 96000.f) // 97352592
//define B2M_88200 (uint32_t)((double)4294967296000.0 / 88200.f)) // 97352592
//define B2M_44100 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT) // 97352592
//define B2M_22050 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 2.0)
//define B2M_11025 (uint32_t)((double)4294967296000.0 / AUDIO_SAMPLE_RATE_EXACT * 4.0)

/**/
bool AudioSDPlayer_F32::parse_format(void)
{
  uint8_t num = 0;
  //uint16_t format;
  //uint16_t channels;
  //uint32_t rate; //b2m;
  //uint16_t bits;

  //format = header[0];
  //if (format != 1) {
	//Serial.println( "AudioSDPlayer_F32: *** ERROR ***: parse_format: WAV format not equal to 1.  Format = " + String(format) );
    //return false;
  //}

  //rate = header[1];
  //Serial.println("AudioSDPlayer_F32::parse_format: rate = " + String(rate));
  
  //if (rate == 96000) {
  //  //b2m = B2M_96000;
  //} else if (rate == 88200) {
  //  //b2m = B2M_88200;
  //} else if (rate == 44100) {
  //  //b2m = B2M_44100;

  if ( sample_rate_Hz > (22050+50) ) { //replaced the logic shown above to allow for more sample rates
	  //do nothing.  assume good.
  } else if (sample_rate_Hz == 22050) {
    //b2m = B2M_22050;
    num |= 4;
  } else if (sample_rate_Hz == 11025) {
    //b2m = B2M_11025;
    num |= 4;
  } else {
    return false;
  }

  //channels = header[0] >> 16;
  if (channels == 1) {
  } else if (channels == 2) {
    //b2m >>= 1;
    num |= 1;
  } else {
    return false;
  }

  //bits = header[3] >> 16;
  //Serial.println("AudioSDPlayer_F32::parse_format: bits = " + String(bits));
  if (bits == 8) {
  } else if (bits == 16) {
    //b2m >>= 1;
    num |= 2;
  } else {
    return false;
  }

  updateBytes2Millis();
  
  //Serial.print("  bytes2millis = ");
  //Serial.println(b2m);

  // we're not checking the byte rate and block align fields
  // if they're not the expected values, all we could do is
  // return false.  Do any real wav files have unexpected
  // values in these other fields?
  state_play = num;

  return true;
}

uint32_t AudioSDPlayer_F32::updateBytes2Millis(void)
{
  double b2m;

  //account for sample rate
  b2m = ((double)4294967296000.0 / ((double)sample_rate_Hz));

  //account for channels
  b2m = b2m / ((double)channels);

  //account for bits per second
  if (bits == 16) {
    b2m = b2m / 2;
  } else if (bits == 24) {
    b2m = b2m / 3;  //can we handle 24 bits?  I don't think that we can.
  }

  return bytes2millis = ((uint32_t)b2m);
}

bool AudioSDPlayer_F32::isPlaying(void)
{
	
  //if (state < STATE_STOP) Serial.println("AudioSDPlayer_F32: isPlaying: state = " + String(state));
  uint8_t s = *(volatile uint8_t *)&state;
  //if (state < STATE_STOP) Serial.println("AudioSDPlayer_F32: isPlaying: s = " + String(state) + ", is less than " + String(STATE_STOP) + "?");
  return ((s < STATE_STOP) && (active));  //used to be (s < 8) which rejected if still parsing the header
}



uint32_t AudioSDPlayer_F32::positionMillis(void)
{
  uint8_t s = *(volatile uint8_t *)&state;
  if (s >= 8) return 0;
  uint32_t tlength = *(volatile uint32_t *)&total_length;
  uint32_t dlength = *(volatile uint32_t *)&data_length;
  uint32_t offset = tlength - dlength;
  uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
  return ((uint64_t)offset * b2m) >> 32;
}


uint32_t AudioSDPlayer_F32::lengthMillis(void)
{
  uint8_t s = *(volatile uint8_t *)&state;
  if (s >= 8) return 0;
  uint32_t tlength = *(volatile uint32_t *)&total_length;
  uint32_t b2m = *(volatile uint32_t *)&bytes2millis;
  return ((uint64_t)tlength * b2m) >> 32;
}

//int AudioSDPlayer_F32::isSdCardPresent(void) {
//	if (!hasSdBegun) hasSdBegun = SD.begin(BUILTIN_SDCARD);
//	return SD.mediaPresent();
//}


/**
 * @brief Print debug statements, if SD_PLAYER_DEBUG_PRINTING == true
 * 
 * @param msg 
 */
void AudioSDPlayer_F32::debugPrint(const String &msg) {
	#if (SD_PLAYER_DEBUG_PRINTING == true)
	Serial.println(msg);
	#endif
}