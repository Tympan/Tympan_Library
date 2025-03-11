/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * Extended by Chip Audette, Open Audio, Apr 2018
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

#ifndef AudioEffecDelay_F32_h_
#define AudioEffecDelay_F32_h_

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "utility/dspinst.h"

#define AUDIO_BLOCK_SIZE_F32 AUDIO_BLOCK_SAMPLES    //what is the maximum length of the F32 audio blocks


#if defined(__MK66FX1M0__)
  // 2.41 second maximum on Teensy 3.6
  #define DELAY_QUEUE_SIZE_F32  (106496 / AUDIO_BLOCK_SIZE_F32)
#elif defined(__MK64FX512__)
  // 1.67 second maximum on Teensy 3.5
  #define DELAY_QUEUE_SIZE_F32  (73728 / AUDIO_BLOCK_SIZE_F32)
#elif defined(__MK20DX256__)
  // 0.45 second maximum on Teensy 3.1 & 3.2
  #define DELAY_QUEUE_SIZE_F32  (19826 / AUDIO_BLOCK_SIZE_F32)
#else
  // 0.14 second maximum on Teensy 3.0
  #define DELAY_QUEUE_SIZE_F32  (6144 / AUDIO_BLOCK_SIZE_F32)
#endif


class AudioEffectDelay_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:delay
public:
	AudioEffectDelay_F32() : AudioStream_F32(1, inputQueueArray) {
		activemask = 0;
		headindex = 0;
		tailindex = 0;
		maxblocks = 0;
		memset(queue, 0, sizeof(queue));
	}
	AudioEffectDelay_F32(const AudioSettings_F32 &settings) : 
		AudioStream_F32(1,inputQueueArray) {
			activemask = 0;
			headindex = 0;
			tailindex = 0;
			maxblocks = 0;
			memset(queue, 0, sizeof(queue));
			setSampleRate_Hz(settings.sample_rate_Hz);
	}	
	void setSampleRate_Hz(float _fs_Hz) { 
		//Serial.print("AudioEffectDelay_F32: setSampleRate_Hz to ");
		//Serial.println(_fs_Hz);
		sampleRate_Hz = _fs_Hz; 
		//audio_block_len_samples = block_size;  //this is the actual size that is being used in each audio_block_f32 
	}
	float getSampleRate_Hz(void) { return sampleRate_Hz; }
	
	void delay(uint8_t channel, float milliseconds) {
		if (channel >= 8) return;
		if (milliseconds < 0.0) milliseconds = 0.0;
		uint32_t n = (milliseconds*(sampleRate_Hz/1000.0))+0.5;
		uint32_t nmax = AUDIO_BLOCK_SIZE_F32 * (DELAY_QUEUE_SIZE_F32-1);
		if (n > nmax) n = nmax;
		uint32_t blks = (n + (AUDIO_BLOCK_SIZE_F32-1)) / AUDIO_BLOCK_SIZE_F32 + 1;
		if (!(activemask & (1<<channel))) {
			// enabling a previously disabled channel
			delay_samps[channel] = n;
			if (blks > maxblocks) maxblocks = blks;
			activemask |= (1<<channel);
		} else {
			if (n > delay_samps[channel]) {
				// new delay is greater than previous setting
				if (blks > maxblocks) maxblocks = blks;
				delay_samps[channel] = n;
			} else {
				// new delay is less than previous setting
				delay_samps[channel] = n;
				recompute_maxblocks();
			}
		}
	}
	void disable(uint8_t channel) {
		if (channel >= 8) return;
		// diable this channel
		activemask &= ~(1<<channel);
		// recompute maxblocks for remaining enabled channels
		recompute_maxblocks();
	}
	virtual void update(void);
	virtual void processData(audio_block_f32_t *input,audio_block_f32_t *output);
	virtual void processData(audio_block_f32_t *input,audio_block_f32_t *all_output[8]);
	
private:
	void recompute_maxblocks(void) {
		uint32_t max=0;
		uint32_t channel = 0;
		do {
			if (activemask & (1<<channel)) {
				uint32_t n = delay_samps[channel];
				n = (n + (AUDIO_BLOCK_SIZE_F32-1)) / AUDIO_BLOCK_SIZE_F32 + 1;
				if (n > max) max = n;
			}
		} while(++channel < 8);
		maxblocks = max;
	}
	uint8_t activemask;   // which output channels are active
	uint16_t headindex;    // head index (incoming) data in queue
	uint16_t tailindex;    // tail index (outgoing) data from queue
	uint16_t maxblocks;    // number of blocks needed in queue
//#if DELAY_QUEUE_SIZE_F32 * AUDIO_BLOCK_SAMPLES < 65535
//	uint16_t writeposition;
//	uint16_t delay_samps[8]; // # of samples to delay for each channel
//#else
	int writeposition = 0;	   //position within current head buffer in the queue
	uint32_t delay_samps[8]; // # of samples to delay for each channel
//#endif
	audio_block_f32_t *queue[DELAY_QUEUE_SIZE_F32];
	audio_block_f32_t *inputQueueArray[1];
	float sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT; //default.  from AudioStream.h??
	//int audio_block_len_samples = AUDIO_BLOCK_SAMPLES;
	void receiveIncomingData(audio_block_f32_t *input);
	void discardUnneededBlocksFromQueue(void);
	void transmitOutgoingData(audio_block_f32_t *all_output[]);
	unsigned long last_received_block_id = 0;
};

#endif