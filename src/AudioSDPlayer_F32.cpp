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

#define STATE_DIRECT_8BIT_MONO    0  // playing mono at native sample rate
#define STATE_DIRECT_8BIT_STEREO  1  // playing stereo at native sample rate
#define STATE_DIRECT_16BIT_MONO   2  // playing mono at native sample rate
#define STATE_DIRECT_16BIT_STEREO 3  // playing stereo at native sample rate
#define STATE_CONVERT_8BIT_MONO   4  // playing mono, converting sample rate
#define STATE_CONVERT_8BIT_STEREO 5  // playing stereo, converting sample rate
#define STATE_CONVERT_16BIT_MONO  6  // playing mono, converting sample rate
#define STATE_CONVERT_16BIT_STEREO  7  // playing stereo, converting sample rate
#define STATE_PARSE1      8  // looking for 20 byte ID header
#define STATE_PARSE2      9  // looking for 16 byte format header
#define STATE_PARSE3      10 // looking for 8 byte data header
#define STATE_PARSE4      11 // ignoring unknown chunk after "fmt "
#define STATE_PARSE5      12 // ignoring unknown chunk before "fmt "
#define STATE_STOP      13
#define STATE_NOT_BEGUN  99

#define READ_STATE_NORMAL  1
#define READ_STATE_FILE_EMPTY 2

#define SD_CONFIG SdioConfig(FIFO_SDIO) //is this correct for both Teensy 3 and Teensy 4?

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

bool AudioSDPlayer_F32::listFiles(void) {
	if (state == STATE_NOT_BEGUN) begin();
	return sd_ptr->ls(); //add LS_R to make recursive.  otherwise, just does the local directory
}


// Here is the "open" function.  It is rare that the user will want to call this directly.
// Basically, only use this if you are going to stream the data out over Serial rather than
// playing the audio in real time.
bool AudioSDPlayer_F32::open(const char *filename, const bool flag_preload_buffer)
{
  if (state == STATE_NOT_BEGUN) begin();
  //stop();
	
	active = false;  //from AudioStream.h.  Setting to false to prevent update() from being called.  Only for open().

  __disable_irq();
  file.open(filename,O_READ);   //open for reading
  __enable_irq();
  if (!isFileOpen()) return false;
  
  //buffer_length = 0;
  state_play = STATE_STOP;
  data_length = 20; //set to a number bigger than zero to enable the readHeader() to really read some bytes
  header_offset = 0;
  state = STATE_PARSE1;
  
  //read header ... added Chip Audette March 2024
  buffer_read = 0;
  buffer_write = 0;
  state_read = READ_STATE_NORMAL;
  readHeader();
	
	//pre-fill the buffer?
	if (flag_preload_buffer) readFromSDtoBuffer(READ_SIZE_BYTES);
		
  return true;
}

//"play" is the usual way of opening a file and playing it.
bool AudioSDPlayer_F32::play(const char *filename)
{
	bool isOpen = open(filename,false);
	
	//now, activate the instance so that update() will be called
	active = true;  //in AudioStream.h.  Activates this instance so that update() gets called
	
	return isOpen;
}

void AudioSDPlayer_F32::stop(void)
{
  if (state == STATE_NOT_BEGUN) begin();

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
    file.close();

  } else {
    __enable_irq();
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
	}
}

uint16_t AudioSDPlayer_F32::readFromBuffer(float32_t *left_f32, float32_t *right_f32, int n_samps) {
	//loop through the buffer and copy samples to the output arrays
	uint16_t n_samples_read = 0;
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


uint32_t AudioSDPlayer_F32::readRawBytes(uint8_t *out_buffer, const uint32_t n_bytes_wanted) {
	if (n_bytes_wanted == 0) return 0;
	uint32_t n_bytes_filled = 0;
	
	//ensure that there is some data in the buffer (if any is available)
	if ((state_read == READ_STATE_NORMAL) && (data_length > 0)) readFromSDtoBuffer(READ_SIZE_BYTES);
	
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
		if ((state_read == READ_STATE_NORMAL) && (data_length > 0)) readFromSDtoBuffer(READ_SIZE_BYTES);
		
	}
			
		
	//if we didn't get all the bytes that we wanted, fill the rest with zeros
	if (n_bytes_filled < n_bytes_wanted) {
		uint32_t foo_index = n_bytes_filled;
		while (foo_index < n_bytes_wanted) out_buffer[foo_index++] = 0;  //fill with zeros
	}
	
	//return the number of bytes of real data
	return n_bytes_filled;
}
uint16_t AudioSDPlayer_F32::readBuffer_16bit_to_f32(float32_t *left_f32, float32_t *right_f32, int n_samps_wanted, int n_chan) {
	if (n_chan < 1) return 0;
	uint16_t n_samples_read = 0;
	const uint16_t bytes_perSamp_allChans = 2 * n_chan; //mono will need 2 bytes (16 bits) per sample and stereo will need 2*2 bytes (32 bits) per sample
	const int bytes_total = bytes_perSamp_allChans*n_chan;
	uint8_t lsb, msb;
	int16_t val_int16;
	float32_t val_f32;
	
	//check to see if we have enough data in the main buffer.  If not, try to load some, if possible
	while ((state_read == READ_STATE_NORMAL) && ( (getNumBuffBytes() < bytes_total) || (data_length < (uint32_t)bytes_total) ) )
	{
		readFromSDtoBuffer(READ_SIZE_BYTES);
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
				
				//form the 16-bit value and confirm to float
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


int AudioSDPlayer_F32::getNumBuffBytes(void) {
	int n_buff = 0;
	if (buffer_read == buffer_write) {
		n_buff = 0;
	} else if (buffer_read > buffer_write) {
		n_buff = N_BUFFER - buffer_read + buffer_write;
	} else {
		n_buff = buffer_write - buffer_read;
	}
	return n_buff;
}

int AudioSDPlayer_F32::serviceSD(void) {
	//This function should be called from the Arduino loop() function.
	//This function loads data from the SD card into a RAM buffer, as
	//long as there's room in the buffer to do a full read (512 bytes?)

	//should we be reading?  If not, return.
	if (state == STATE_STOP) return 0;

	return readFromSDtoBuffer(READ_SIZE_BYTES);

}

int AudioSDPlayer_F32::readFromSDtoBuffer(const uint16_t n_bytes_to_read) {
	if (state_read == READ_STATE_FILE_EMPTY) return -1;
	if (file.available() == false) return -1;
	if (n_bytes_to_read == 0) return 0;

	//try reading data (might be getting header first, though
	bool file_has_data = true;
	while ((N_BUFFER - getNumBuffBytes() > n_bytes_to_read) && (file_has_data)) {
	
		//read the bytes from the SD into a temporary linear buffer
		int read_length = file.read(temp_buffer, n_bytes_to_read);
		if (read_length != n_bytes_to_read) { 
			file_has_data = false;
			//Serial.println("AudioSDPlayer_F32: readFromSDtoBuffer: read_length was only " + String(read_length) + " versus expected " + String(READ_SIZE_BYTES));
		}
		
		//copy the bytes from the temporary linear buffer into the full circular buffer
		for (int i=0; i <  read_length; i++) {
			buffer[buffer_write++] = temp_buffer[i];
			if (buffer_write >= N_BUFFER) buffer_write = 0; //wrap around as needed
		}
	}
	
	//did the file run out of data?
	if (file_has_data == false) {
		file.close();
		state_read = READ_STATE_FILE_EMPTY;
	}
	
	return 0;  //normal return
	
}

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
bool AudioSDPlayer_F32::readHeader(void) {
  uint32_t len;
  //uint8_t lsb, msb;
  const uint8_t *p;
  //int16_t val_int16;

  //read first bytes from file into our global buffer
  readFromSDtoBuffer(READ_SIZE_BYTES); //this should be more than enough
  uint32_t size = getNumBuffBytes();
  if (size == 0) {
	  Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: Failed to read any bytes.");
	  return false;
  }
  
  //make pointer to location in the buffer
  p = buffer + buffer_read;

start:

 #if 0
  Serial.print("AudioSDPlayer_F32: readHeader, start: ");
  Serial.print("size = ");
  Serial.print(size);
  Serial.print(", buffer_read = ");
  Serial.print(buffer_read);
  Serial.print(", data_length = ");
  Serial.print(data_length);
  Serial.print(", space = ");
  Serial.print((audio_block_samples - block_offset) * 2);
  Serial.print(", state = ");
  Serial.println(state);
#endif

  switch (state) {
    case STATE_PARSE1:  //parse wav file header, is this really a .wav file?
		//pull some header data out of the main buffer
		len = data_length;           //based on play(), data_length should be 20 at this point?
		if (size < len) len = size;  //size should be READ_SIZE_BYTES, which should be 512, so much bigger than len
		memcpy((uint8_t *)header + header_offset, p, len); //copy from the regular buffer to the header buffer?
		header_offset += len;   //increment the write pointer for the header buffer
		buffer_read += len;     //increment the read pointer for the general buffer
		data_length -= len;     //given logic above, len should equal buffer_read, so this should set data_length to zero
		if (data_length > 0) {
			//given the logic above, data_length should now be zero, so should not be here
			Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: STATE_PARSE1: data_length should be zero but is " + String(data_length));
			return false;  
		}
		// parse the header bytes
		if (header[0] == 0x46464952 && header[2] == 0x45564157) {  //this is "FFIR" or "EVAW", which backwards is  "RIFF" or "WAVE"
		  //Serial.println("AudioSDPlayer_F32: readHeader:  is wav file");
		  if (header[3] == 0x20746D66) {  //look for "tmf", which backwards is "fmt"
			// "fmt " header
			if (header[4] < 16) {
			  // WAV "fmt " info must be at least 16 bytes
			  break;
			}
			if (header[4] > sizeof(header)) {
			  // if such .wav files exist, increasing the
			  // size of header[] should accomodate them...
			  //Serial.println("AudioSDPlayer_F32: readHeader:  WAVEFORMATEXTENSIBLE too long");
			  break;
			}
			//Serial.println("header ok");
			header_offset = 0;
			state = STATE_PARSE2;  //move on to next stage of parsing
		  } else {
			// first chuck is something other than "fmt "
			#if 0
			Serial.print("AudioSDPlayer_F32: readHeader:  skipping \"");
			Serial.printf("\" (%08X), ", __builtin_bswap32(header[3]));
			Serial.print(header[4]);
			Serial.println(" bytes");
			#endif
			header_offset = 12;
			state = STATE_PARSE5;  //or jump ahead?
		  }
		  p += len;
		  size -= len;
		  data_length = header[4]; //how much more of the data is header?  (hopefully is still less than 
		  goto start;  //jump back up to the beginning to go back through the big switch block
		}
		//Serial.println("AudioSDPlayer_F32: readHeader: unknown WAV header");
		break;

    case STATE_PARSE2:   // check & extract key audio parameters
		len = data_length;             //data_length was set based on what the WAV header had said (see STATE_PARSE1)
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);  //copy from the regular buffer to the header buffer?
		header_offset += len;
		buffer_read += len;
		data_length -= len;
		if (data_length > 0) {
			//should not be here
			Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: STATE_PARSE2: data_length should be zero but is " + String(data_length));
			return false;  
		}
		if (parse_format()) { //here's the key operation
		  p += len;
		  size -= len;
		  data_length = 8;
		  header_offset = 0;
		  state = STATE_PARSE3;
		  goto start;
		}
		Serial.println("AudioSDPlayer_F32: readHeader: unknown audio format");
		break;

    case STATE_PARSE3: // 10  // find the data chunk
		len = data_length; 
		if (size < len) len = size;
		memcpy((uint8_t *)header + header_offset, p, len);
		header_offset += len;
		buffer_read += len;
		data_length -= len;
		if (data_length > 0) {
			//should not be here
			Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: STATE_PARSE3: data_length should be zero but is " + String(data_length));
			return false;  
		}
		#if 0
			Serial.print("AudioSDPlayer_F32: readHeader: chunk id = ");
			Serial.print(header[0], HEX);
			Serial.print(", length = ");
			Serial.println(header[1]);
		#endif
		p += len;
		size -= len;
		data_length = header[1];
		if (header[0] == 0x61746164) {  //this are characters for "atad" which is same as "data" backwards
		  //Serial.println("AudioSDPlayer_F32: readHeader: wav: found data chunk, len=" + String(data_length));    // TODO: verify offset in file is an even number
		  // as required by WAV format.  abort if odd.  Code
		  // below will depend upon this and fail if not even.
		  leftover_bytes = 0;
		  state = state_play;
		  state_read = READ_STATE_NORMAL;
		  total_length = data_length;
		} else {
		  state = STATE_PARSE4;
		}
		goto start;

    
    case STATE_PARSE4: // 11,  // ignore any extra unknown chunks (title & artist info)
		if (size < data_length) {
		  data_length -= size;
		  buffer_read += size;
		  Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: STATE_PARSE4: size is " + String(size) + " but should be less than data_length = " + String(data_length)); 
		  return false;
		}
		p += data_length;
		size -= data_length;
		buffer_read += data_length;
		data_length = 8;
		header_offset = 0;
		state = STATE_PARSE3;
		//Serial.println("AudioSDPlayer_F32: readHeader: consumed unknown chunk");
		goto start;


    case STATE_PARSE5:       // skip past "junk" data before "fmt " header
		len = data_length;
		if (size < len) len = size;
		buffer_read += len;
		data_length -= len;
		if (data_length > 0) {
			//should not be here
			Serial.println("AudioSDPlayer_F32: readHeader: *** ERROR ***: STATE_PARSE5: data_length should be zero but is " + String(data_length));
			return false;  
		}
		p += len;
		size -= len;
		data_length = 8;
		state = STATE_PARSE1;
		goto start;

	case STATE_DIRECT_16BIT_MONO: case STATE_DIRECT_16BIT_STEREO:
		//we could be here after parsing successfully, so let's assume it's good and simply break out of the switch block
		break;

    // ignore any extra data after playing
    // or anything following any error
    //case STATE_STOP:
	//	return false;

    // this is not supposed to happen!
    default:
		Serial.println("AudioSDPlayer_F32: readHeader: unknown state = " + String(state));
  }
  
  //Serial.println("AudioSDPlayer_F32: readHeader: finished parsing?  state = " + String(state));
  
  //state_play = STATE_STOP;
  //state = STATE_STOP;
  return false;
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


bool AudioSDPlayer_F32::parse_format(void)
{
  uint8_t num = 0;
  uint16_t format;
  //uint16_t channels;
  uint32_t rate; //b2m;
  //uint16_t bits;

  format = header[0];
  if (format != 1) {
	Serial.println("AudioSDPlayer_F32: *** ERROR ***: parse_format: WAV format not equal to 1.  Format = " + String(format));
    return false;
  }

  rate = header[1];
  //Serial.println("AudioSDPlayer_F32::parse_format: rate = " + String(rate));
  
  //if (rate == 96000) {
  //  //b2m = B2M_96000;
  //} else if (rate == 88200) {
  //  //b2m = B2M_88200;
  //} else if (rate == 44100) {
  //  //b2m = B2M_44100;
  if (rate > (22050+50)) { //replaced the logic shown above to allow for more sample rates
	  //do nothing.  assume good.
  } else if (rate == 22050) {
    //b2m = B2M_22050;
    num |= 4;
  } else if (rate == 11025) {
    //b2m = B2M_11025;
    num |= 4;
  } else {
    return false;
  }

  channels = header[0] >> 16;
  //Serial.println("AudioSDPlayer_F32::parse_format: channels = " + String(channels));
  if (channels == 1) {
  } else if (channels == 2) {
    //b2m >>= 1;
    num |= 1;
  } else {
    return false;
  }

  bits = header[3] >> 16;
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

