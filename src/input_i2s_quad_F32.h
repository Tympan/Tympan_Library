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
 
#ifndef _input_i2s_quad_f32_h_
#define _input_i2s_quad_f32_h_


#include <Arduino.h>
#include <arm_math.h>
#include <AudioStream_F32.h>
#include "DMAChannel.h"
#include <AudioI2SBase.h>   //tympan library, for AudioI2SBase


class AudioInputI2SQuad_F32 : public AudioInputI2SBase_F32  //which also inherits from AudioStream_F32
{
//GUI: inputs:0, outputs:2  //this line used for automatic generation of GUI nodes
public:
	AudioInputI2SQuad_F32(void)                                                   : AudioInputI2SBase_F32(4) { begin(); }
	AudioInputI2SQuad_F32(const AudioSettings_F32 &settings)                      : AudioInputI2SQuad_F32(settings, true) {}
	AudioInputI2SQuad_F32(const AudioSettings_F32 &settings, bool flag_callBegin) : AudioInputI2SBase_F32(4, settings) { if (flag_callBegin) begin(); }
 	//AudioInputI2SQuad_F32(const AudioSettings_F32 &settings, uint32_t *rx_buff)   : AudioInputI2SBase_F32(4, settings)  { 
	//	i2s_rx_buffer = rx_buff;
	//	begin(); 
	//} 
	void begin(void);

protected:

};


#endif
