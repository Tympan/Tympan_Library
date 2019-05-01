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
 *  Extended by Chip Audette, OpenAudio, May 2019
 *  Converted to F32 and to variable audio block length
 *	The F32 conversion is under the MIT License.  Use at your own risk.
 */
 
#ifndef output_i2s_quad_f32_h_
#define output_i2s_quad_f32_h_

#include "Arduino.h"
#include "AudioStream_F32.h"
//include "AudioStream.h"
#include "DMAChannel.h"
#include "output_i2s_f32.h"  //for setI2Sfreq()

class AudioOutputI2SQuad_F32 : public AudioStream_F32
{
public:
	AudioOutputI2SQuad_F32(void) : AudioStream_F32(4, inputQueueArray) { begin(); }
	AudioOutputI2SQuad_F32(const AudioSettings_F32 &settings) : AudioStream_F32(2, inputQueueArray)
	{ 
		sample_rate_Hz = settings.sample_rate_Hz;
		audio_block_samples = settings.audio_block_samples;
		begin(); 	
	}
	virtual void update(void);
	void begin(void);
	friend class AudioInputI2SQuad_F32;
	static void scale_f32_to_i16( float32_t *p_f32, float32_t *p_i16, int len) ;
	static void scale_f32_to_i24( float32_t *p_f32, float32_t *p_i16, int len) ;
	static void scale_f32_to_i32( float32_t *p_f32, float32_t *p_i32, int len) ;
protected: 
	static void config_i2s(void);
	static audio_block_f32_t *block_ch1_1st;
	static audio_block_f32_t *block_ch2_1st;
	static audio_block_f32_t *block_ch3_1st;
	static audio_block_f32_t *block_ch4_1st;
	static bool update_responsibility;
	static DMAChannel dma;
	static void isr(void);
private:
	static audio_block_f32_t *block_ch1_2nd;
	static audio_block_f32_t *block_ch2_2nd;
	static audio_block_f32_t *block_ch3_2nd;
	static audio_block_f32_t *block_ch4_2nd;
	static uint16_t ch1_offset;
	static uint16_t ch2_offset;
	static uint16_t ch3_offset;
	static uint16_t ch4_offset;
	audio_block_f32_t *inputQueueArray[4];
	static float sample_rate_Hz;
	static int audio_block_samples;
	volatile uint8_t enabled = 1;
};

#endif
