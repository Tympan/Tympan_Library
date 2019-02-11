/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * Modified to floating point by Oyvind Mjanger.  
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


#include "AudioAnalyzeRMS_F32.h"
#include "utility/dspinst.h"

void AudioAnalyzeRMS_F32::update(void)
{
	audio_block_f32_t *block = receiveReadOnly_f32();
	if (!block) {
		count++;
		return;
	}
#if defined(KINETISK)
	float32_t *p = (float32_t *)(block->data);
	float32_t *end = p + AUDIO_BLOCK_SAMPLES;
	float64_t sum = accum;
	float32_t in;                                  // Temporary variable to store input value 
	
	do {
		in = *p++;
		sum += in * in;
		in = *p++;
		sum += in * in;
		in = *p++;
		sum += in * in;
		in = *p++;
		sum += in * in;
	} while (p < end);
	accum = sum;
	count++;
#else
	float32_t *p = block->data;
	float32_t *end = p + AUDIO_BLOCK_SAMPLES;
	float64_t sum = accum;
	do {
		float32_t n = *p++;
		sum += n * n;
	} while (p < end);
	accum = sum;
	count++;
#endif
	release(block);
}

float AudioAnalyzeRMS_F32::read(void)
{
	__disable_irq();
	float64_t sum = accum;
	accum = 0;
	float32_t num = count;
	count = 0;
	__enable_irq();
	float32_t meansq = sum / (num * AUDIO_BLOCK_SAMPLES);
	return sqrtf(meansq);
}

