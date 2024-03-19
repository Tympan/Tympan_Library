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
*/

#ifndef AudioSDPlayerTeensy_F32_h_
#define AudioSDPlayerTeensy_F32_h_

#include "Arduino.h"
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

#include <SdFat.h>  //included in Teensy install as of Teensyduino 1.54-bete3

class AudioSDPlayerTeensy_F32 : public AudioStream_F32
{
//GUI: inputs:0, outputs:2  //this line used for automatic generation of GUI nodes  
  public:
    AudioSDPlayerTeensy_F32(void) : AudioStream_F32(0, NULL), sd_ptr(NULL)
	{ 
      init();
    }
    AudioSDPlayerTeensy_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL), sd_ptr(NULL)
      { 
        init();
        setSampleRate_Hz(settings.sample_rate_Hz);
        setBlockSize(settings.audio_block_samples);
      }
    AudioSDPlayerTeensy_F32(SdFs * _sd,const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL), sd_ptr(_sd)
    { 	 
        init();
        setSampleRate_Hz(settings.sample_rate_Hz);
        setBlockSize(settings.audio_block_samples);
	}
	
	//~AudioSDPlayerTeensy_F32(void) {
	//	delete sd_ptr;
	//}
	
    void init(void);
    void begin(void);  //begins SD card
    bool play(const String &filename) { return play(filename.c_str()); }
    bool play(const char *filename);
    void stop(void);
    bool isPlaying(void);
    uint32_t positionMillis(void);
    uint32_t lengthMillis(void);
    virtual void update(void);
    float setSampleRate_Hz(float fs_Hz) { 
      sample_rate_Hz = fs_Hz; 
      updateBytes2Millis();
      return sample_rate_Hz;
    }
    int setBlockSize(int block_size) { return audio_block_samples = block_size; };
    bool isFileOpen(void) {
      if (file.isOpen()) return true;
      return false;
    }  
	void setSdPtr(SdFs *ptr) { sd_ptr = ptr; }
  
  private:
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
    audio_block_f32_t *block_left_f32 = NULL;
    audio_block_f32_t *block_right_f32 = NULL;
    uint16_t block_offset;    // how much data is in block_left & block_right
    uint8_t buffer[512];    // buffer one block of data
    uint16_t buffer_offset;   // where we're at consuming "buffer"
    uint16_t buffer_length;   // how much data is in "buffer" (512 until last read)
    uint8_t header_offset;    // number of bytes in header[]
    uint8_t state;
    uint8_t state_play;
    uint8_t leftover_bytes;
    static unsigned long update_counter;
    float sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
    int audio_block_samples = AUDIO_BLOCK_SAMPLES;
    

    uint32_t updateBytes2Millis(void);
};

#endif
