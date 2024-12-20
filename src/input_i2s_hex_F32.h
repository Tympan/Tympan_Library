/* Audio Library for Teensy 3.X // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
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
 *  Extended by Chip Audette, OpenAudio, Dec 2024
 *  Converted to F32 and to variable audio block length
 *	The F32 conversion is under the MIT License.  Use at your own risk.
 */
 

#ifndef _input_i2s_hex_f32_h_
#define _input_i2s_hex_f32_h_

#include <Arduino.h>     // github.com/PaulStoffregen/cores/blob/master/teensy4/Arduino.h
#include <AudioStream_F32.h> // github.com/PaulStoffregen/cores/blob/master/teensy4/AudioStream.h
//include "AudioStream.h"   //do we really need this? (from Chip)
#include <DMAChannel.h>  // github.com/PaulStoffregen/cores/blob/master/teensy4/DMAChannel.h
#include "input_i2s_F32.h" //for scale_i16_to_f32() and for AudioInputI2SBase_F32


class AudioInputI2SHex_F32 : public AudioInputI2SBase_F32  //which also inherits from AudioStream_F32
{
public:
	AudioInputI2SHex_F32(void)  { begin(); }
	AudioInputI2SHex_F32(const AudioSettings_F32 &settings) : AudioInputI2SHex_F32(settings, true) {}
	AudioInputI2SHex_F32(const AudioSettings_F32 &settings, bool flag_callBegin) { 
		sample_rate_Hz = settings.sample_rate_Hz;
		audio_block_samples = settings.audio_block_samples;
		if (flag_callBegin) begin(); 
	}
	virtual void update(void);
	void begin(void);
	int get_isOutOfMemory(void) { return flag_out_of_memory; }
	void clear_isOutOfMemory(void) { flag_out_of_memory = 0; }
	static uint32_t *i2s_rx_buffer; 
protected:
	static bool update_responsibility;
	static DMAChannel dma;
	static void isr(void);
	virtual void update_1chan(int, unsigned long, audio_block_f32_t *&);
private:
	static audio_block_f32_t *block_ch1;
	static audio_block_f32_t *block_ch2;
	static audio_block_f32_t *block_ch3;
	static audio_block_f32_t *block_ch4;
	static audio_block_f32_t *block_ch5;
	static audio_block_f32_t *block_ch6;
	static uint32_t block_offset;
};


#endif