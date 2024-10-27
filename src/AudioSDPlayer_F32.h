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
 *
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

#ifndef AudioSDPlayer_F32_h_
#define AudioSDPlayer_F32_h_

#include "Arduino.h"
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

#include <SdFat.h>  //included in Teensy install as of Teensyduino 1.54-bete3

class AudioSDPlayer_F32 : public AudioStream_F32
{
	//GUI: inputs:0, outputs:2  //this line used for automatic generation of GUI nodes  
	public:
		AudioSDPlayer_F32(void) : AudioStream_F32(0, NULL), sd_ptr(NULL) { 
			init();
		}
		AudioSDPlayer_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL), sd_ptr(NULL) { 
			init();
			setSampleRate_Hz(settings.sample_rate_Hz);
			setBlockSize(settings.audio_block_samples);
		}
		AudioSDPlayer_F32(SdFs * _sd,const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL), sd_ptr(_sd) { 	 
			init();
			setSampleRate_Hz(settings.sample_rate_Hz);
			setBlockSize(settings.audio_block_samples);
		}

		//~AudioSDPlayer_F32(void) {
		//	delete sd_ptr;
		//}

		void init(void);
		virtual void begin(void);  //begins SD card
		virtual bool play(const String &filename) { return play(filename.c_str()); } //"play" opens the file and activates for using update()
		virtual bool play(const char *filename);  //"play" opens the file and activates for using update()
		virtual bool play(void);    //plays a file that has already been opened
		virtual bool open(const String &filename) { return open(filename.c_str(),true); } //"open" opens the file but does NOT activate using update()
		virtual bool open(const char *filename, bool flag_preload_buffer);  //"open" opens the file but does NOT activate using update()
		virtual void stop(void);
		virtual bool isPlaying(void);
		virtual uint32_t positionMillis(void);
		virtual uint32_t lengthMillis(void);
		virtual void update(void);
		float setSampleRate_Hz(float fs_Hz) { 
			sample_rate_Hz = fs_Hz; 
			updateBytes2Millis();
			return sample_rate_Hz;
		}
		int setBlockSize(int block_size) { return audio_block_samples = block_size; };
		virtual bool isFileOpen(void) {
			if (file.isOpen()) return true;
			return false;
		}  
		virtual void setSdPtr(SdFs *ptr) { sd_ptr = ptr; }
		virtual bool sendFilenames(void);

		virtual int serviceSD(void);
		int fillBufferFromSD(void);
		
		//get some sizes
		int16_t getNumChannels(void) { return channels; }
		int16_t getBytesPerSample(void) { return bits/8; }
		uint32_t getBufferLengthBytes(void) { return N_BUFFER; }  //what is the full length of the buffer, regardless of how much data is in it
		virtual uint32_t getNumBuffBytes(void);  //what is the number of bytes in the buffer, which will include any non-audio data that might be at end of WAV
		uint32_t getNumberRemainingBytes(void) { return data_length; } //what is the number of audio bytes remaining to be returned (ie, what's in the buffer *plus* what is still on the SD)
		uint32_t getNumBytesInBuffer(void) { return getNumBuffBytes(); }
		uint32_t getNumBytesInBuffer_msec(void) {
			uint32_t bytes_in_buffer = getNumBytesInBuffer();
			float bytes_per_msec = (sample_rate_Hz*channels*(bits/8)) / 1000.0f;  //these data memersare in the SDWriter class
			return (uint32_t)((float)bytes_in_buffer/bytes_per_msec + 0.5f); //the "+0.5"rounds to the nearest millisec
		}

		// copy some data from SD to a local buffer (RAM).  Because accessing
		// the SD can be slow and unpredictable, don't do this in the high
		// high priority interrupt-driven part of your code (ie, update()).
		// Instead only call it from the low priority main-loop-driven part of
		// your code.
		int readFromSDtoBuffer(const uint16_t n_bytes_to_read);  //returns 0 if normal.  Can only read 2^16 bytes!! (64K)
		int readFromSDtoBuffer_old(const uint16_t n_bytes_to_read);  //returns 0 if normal.  Can only read 2^16 bytes!! (64K)
  
		// Use this to access the raw bytes in the WAV file (excluding the header).
		// As this code might force reading from the SD card, don't do this in the
		// high priority interrupt-driven part of your code (ie, update()).  
		// Instead only call it from the low priority main-loop-driven part of
		// your code.
		uint32_t readRawBytes(uint8_t *out_buffer, const uint32_t n_bytes_to_read);
		
	protected:
		//SdFs sd;
		SdFs *sd_ptr;
		SdFile file;
		bool consume(uint32_t size);
		bool parse_format(void);
		uint32_t header[10];    // temporary storage of wav header data
		uint32_t data_length;   // number of bytes remaining in current section
		uint32_t total_length;    // number of audio data bytes in file
		uint16_t channels = 1; //number of audio channels
		uint16_t bits = 16;  // number of bits per sample
		uint32_t bytes2millis;
		//audio_block_f32_t *block_left_f32 = NULL;
		//audio_block_f32_t *block_right_f32 = NULL;
		uint16_t block_offset;    // how much data is in block_left & block_right

		constexpr static uint32_t MIN_READ_SIZE_BYTES = 512;
		constexpr static uint32_t MAX_READ_SIZE_BYTES = min(8*MIN_READ_SIZE_BYTES,65535); //keep as integer multiple of MIN_READ_SIZE_BYTES
		//uint32_t READ_SIZE_BYTES = MAX_READ_SIZE_BYTES;  //was 512...will larger reads be faster overall?
		//uint8_t temp_buffer[MAX_READ_SIZE_BYTES];  //make same size as the above
		#if defined(KINETISK)
			constexpr static uint32_t N_BUFFER = 32*MIN_READ_SIZE_BYTES;  //Tympan Rev A-D is a Teensy 3.6, which has less RAM, so use a smaller buffer
		#else
			constexpr static uint32_t N_BUFFER = 256*MIN_READ_SIZE_BYTES;  //Newer Tympans have more RAM, so use a biffer buffer.  (originall was 32*READ_SIZE_BYTES)
		#endif
		uint8_t buffer[N_BUFFER];       // buffer X blocks of data
		uint32_t buffer_write = 0;
		uint32_t buffer_read = 0;
		//uint16_t buffer_offset;   // where we're at consuming "buffer"
		//uint16_t buffer_length;   // how much data is in "buffer" (512 until last read)
		uint8_t header_offset;    // number of bytes in header[]
		uint8_t state;
		uint8_t state_play;
		uint8_t state_read;
		uint8_t leftover_bytes;
		static unsigned long update_counter;
		float sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
		int audio_block_samples = AUDIO_BLOCK_SAMPLES;

		uint32_t updateBytes2Millis(void);

		uint32_t readFromBuffer(float32_t *left_f32, float32_t *right_f32, int n_samps);
		//uint32_t readFromSDtoBuffer(float32_t *left_f32, float32_t *right_f32, int n);
		uint32_t readBuffer_16bit_to_f32(float32_t *left_f32, float32_t *right_f32, uint16_t n_samps, uint16_t n_chan);
		bool readHeader(void);
};

#endif
