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
 *
 *  Extended by Chris Brooks and Chip Audette, June/July 2025
 *  Converted to 32-bit data transfers.
 *	The F32 conversion is under the MIT License.  Use at your own risk.
 */
 
#include <Arduino.h>
#include "input_i2s_hex_f32.h"
#include "output_i2s_f32.h"
#include "output_i2s_quad_f32.h"


#define NUM_CHAN_TRANSFER (6)         //this class is for hex (6-channel)
#define MAX_BYTES_PER_SAMPLE (4)      //assume 32-bit transfers is the max supported by this class
#define BIG_BUFFER_TYPE uint32_t
#define BYTES_PER_BIG_BUFF_ELEMENT (sizeof(BIG_BUFFER_TYPE)) //assumes the rx buffer is made up of uint32_t
#define LEN_BIG_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_BIG_BUFF_ELEMENT)
DMAMEM __attribute__((aligned(32))) static BIG_BUFFER_TYPE i2s_default_rx_buffer[LEN_BIG_BUFFER];

//To support 16-bit or 32-bit transfers, let's define how much of the RX buffer we're using ...remember, we're only transfering half of audio_block_samples at a time
#define I2S_BUFFER_TO_USE_BYTES (audio_block_samples*NUM_CHAN_TRANSFER*sizeof(i2s_rx_buffer[0]) /  (transferUsing32bit ? 1 : 2)) //divide in half if transferring using 16 bits
#define I2S_BUFFER_MID_POINT_INDEX ((audio_block_samples/2) * NUM_CHAN_TRANSFER * (transferUsing32bit ? 4 : 2) / (sizeof(i2s_rx_buffer[0])))


// initialize static data members
BIG_BUFFER_TYPE *AudioInputI2SHex_F32::i2s_rx_buffer = i2s_default_rx_buffer;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch1 = NULL;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch2 = NULL;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch3 = NULL;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch4 = NULL;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch5 = NULL;
audio_block_f32_t * AudioInputI2SHex_F32::block_ch6 = NULL;
uint32_t AudioInputI2SHex_F32::block_offset = 0;
bool AudioInputI2SHex_F32::update_responsibility = false;
DMAChannel AudioInputI2SHex_F32::dma(false);


#if defined(__IMXRT1062__)   //This whole class only works for Teensy 4 (Tympan Rev E/F)

void AudioInputI2SHex_F32::begin(void)
{
	transferUsing32bit = true; //default to 32-bit transfers when using this class

	//Serial.println("AudioInputI2SHex_F32: begin: starting...");
	dma.begin(true); // Allocate the DMA channel first

	//AudioOutputI2SHex_F32::sample_rate_Hz = sample_rate_Hz; //these were given in the AudioSettings in the contructor
	//AudioOutputI2SHex_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the contructor
	AudioOutputI2SQuad_F32::sample_rate_Hz = sample_rate_Hz; //these were given in the AudioSettings in the contructor
	AudioOutputI2SQuad_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the contructor
	AudioOutputI2S_F32::sample_rate_Hz = sample_rate_Hz; //these were given in the AudioSettings in the contructor
	AudioOutputI2S_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the contructor
		
	//Setup the pins that control the I2S transfer for two stereo pairs.
	//There are several sets of choices that one can select via "pinoffset"
	const int pinoffset = 0; // TODO: make this configurable...
	AudioOutputI2S_F32::config_i2s(transferUsing32bit, sample_rate_Hz);
	I2S1_RCR3 = I2S_RCR3_RCE_3CH << pinoffset;
	switch (pinoffset) {
	  case 0:
			CORE_PIN8_CONFIG = 3;
			CORE_PIN6_CONFIG = 3;
			CORE_PIN9_CONFIG = 3;
			IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873
			IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
			IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
			break;
	  case 1:
			CORE_PIN6_CONFIG = 3;
			CORE_PIN9_CONFIG = 3;
			CORE_PIN32_CONFIG = 3;
			IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
			IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
			IOMUXC_SAI1_RX_DATA3_SELECT_INPUT = 1; // GPIO_B0_12_ALT3, pg 875
			break;
	}

	// DMA
	//   Each minor loop copies one audio sample from each CODEC (either 3 left or 3 right)
	//   Major loop repeats for audio_block_samples * 2 (stereo)	
	#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
	#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
	#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
	#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)
	#define I2S1_RDR                          (IMXRT_SAI1.RDR)
	if (transferUsing32bit) {
		// For 32-bit samples:
		dma.TCD->SADDR = &(I2S1_RDR[pinoffset]);
		dma.TCD->SOFF = 4;  // This is the separation between sequential RDR registers 
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
	} else {
		// For 16-bit samples:
		dma.TCD->SADDR = (void *)((uint32_t)&(I2S1_RDR[pinoffset]) + 2);  // "+ 2" to shift to start of high 16-bits in 32-bit register
		dma.TCD->SOFF = 4;  // This is the separation between sequential RDR registers 
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
	}
	dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
	  DMA_TCD_NBYTES_MLOFFYES_MLOFF(-(NUM_CHAN_TRANSFER/2)*4) |  // Restore SADDR after each minor loop. 3 rx registers @ 4 byte spacing
	  DMA_TCD_NBYTES_MLOFFYES_NBYTES((NUM_CHAN_TRANSFER/2)*(AudioI2SBase::transferUsing32bit ? 4 : 2));   // copy 3 samples @ 4 bytes or 2 bytes each

	dma.TCD->SLAST = -(NUM_CHAN_TRANSFER/2)*4;   // Restore SADDR after last major loop. 3 rx registers @ 4 byte spacing
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = (AudioI2SBase::transferUsing32bit ? 4 : 2);  	// For 32-bit (or 16-bit) samples:
	
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, Hex uses 3 I2S channels, but that isn't relevant here?)
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;     //allows variable block length
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, Hex uses 3 I2S channels, but that isn't relevant here?)
	
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);

	//I2S1_RCSR = 0;
	//I2S1_RCR3 = I2S_RCR3_RCE_2CH << pinoffset;
	
	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
}

void AudioInputI2SHex_F32::isr(void)
{
	uint32_t daddr, offset;
	const int32_t *src32 = nullptr;
	const int16_t *src16 = nullptr;
	float32_t *dest1, *dest2, *dest3, *dest4, *dest5, *dest6;

	//update the DMA
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	//get pointer for the destination rx buffer
	if (transferUsing32bit) {
		//32-bit transfers
		
		if (daddr < ((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX)) { //we're copying half an audio block times four channels. One address is already 4bytes, like our 32-bit audio sampels
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src32 = (int32_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX]; //For 32-bit transfers...we are transfering half an audio block and 4 channels each		
			if (AudioInputI2SHex_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src32 = (int32_t *)&i2s_rx_buffer[0];
		}
		
		//This block of code only copies the data into F32 buffers but leaves the scaling at +/-32767.0
		//which will then be scaled in the update() method instead of here
		if (block_ch1 && block_ch2 && block_ch3 && block_ch4 && block_ch5 && block_ch6) {
			offset = AudioInputI2SHex_F32::block_offset;
			if (offset <= (uint32_t)(audio_block_samples/2)) {
				arm_dcache_delete((void*)src32, I2S_BUFFER_TO_USE_BYTES/2);
				
				//block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
				AudioInputI2SHex_F32::block_offset = offset + audio_block_samples/2;
				dest1 = &(block_ch1->data[offset]);
				dest2 = &(block_ch2->data[offset]);
				dest3 = &(block_ch3->data[offset]);
				dest4 = &(block_ch4->data[offset]);
				dest5 = &(block_ch5->data[offset]);
				dest6 = &(block_ch6->data[offset]);
				for (int i=0; i < audio_block_samples/2; i++) {
					*dest1++ = ((float32_t) *src32++); //chan 1
					*dest3++ = ((float32_t) *src32++); //chan 3!
					*dest5++ = ((float32_t) *src32++); //chan 5!
					*dest2++ = ((float32_t) *src32++); //chan 2
					*dest4++ = ((float32_t) *src32++); //chan 4!
					*dest6++ = ((float32_t) *src32++); //chan 6!
				}
			}
		}
		
	} else {
		//16-bit transfers

		if (daddr < ((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX)){ //from Chip: enable diff audio block lengths
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src16 = (int16_t *)((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX); 
			if (AudioInputI2SHex_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src16 = (int16_t *)&i2s_rx_buffer[0];
		}
		
		//This block of code only copies the data into F32 buffers but leaves the scaling at +/-32767.0
		//which will then be scaled in the update() method instead of here
		if (block_ch1 && block_ch2 && block_ch3 && block_ch4 && block_ch5 && block_ch6) {
			offset = AudioInputI2SHex_F32::block_offset;
			if (offset <= (uint32_t)(audio_block_samples/2)) {
				arm_dcache_delete((void*)src16, I2S_BUFFER_TO_USE_BYTES/2);
				
				//block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
				AudioInputI2SHex_F32::block_offset = offset + audio_block_samples/2;
				dest1 = &(block_ch1->data[offset]);
				dest2 = &(block_ch2->data[offset]);
				dest3 = &(block_ch3->data[offset]);
				dest4 = &(block_ch4->data[offset]);
				dest5 = &(block_ch5->data[offset]);
				dest6 = &(block_ch6->data[offset]);
				for (int i=0; i < audio_block_samples/2; i++) {
					*dest1++ = ((float32_t) *src16++); //chan 1
					*dest3++ = ((float32_t) *src16++); //chan 3!
					*dest5++ = ((float32_t) *src16++); //chan 5!
					*dest2++ = ((float32_t) *src16++); //chan 2
					*dest4++ = ((float32_t) *src16++); //chan 4!
					*dest6++ = ((float32_t) *src16++); //chan 6!
				}
			}
		}
	
	} //end is transferUsing32bit
}

void AudioInputI2SHex_F32::update_1chan(int chan, unsigned long counter, audio_block_f32_t *&out_block) {
	if (!out_block) return;
		
	//incoming data is still scaled like int16 (so, +/-32767.).  Here we need to re-scale
	//the values so that the maximum possible audio values spans the F32 stadard of +/-1.0
	if (transferUsing32bit) {
		AudioInputI2S_F32::scale_i32_to_f32(out_block->data, out_block->data, audio_block_samples);	
	} else {
		AudioInputI2S_F32::scale_i16_to_f32(out_block->data, out_block->data, audio_block_samples);
	}
	
	//prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	out_block->id = counter;
	out_block->length = audio_block_samples;

	// then transmit and release the DMA's former blocks
	AudioStream_F32::transmit(out_block, chan);
	AudioStream_F32::release(out_block);
}

void AudioInputI2SHex_F32::update(void)
{
	audio_block_f32_t *new1, *new2, *new3, *new4, *new5, *new6;
	audio_block_f32_t *out1, *out2, *out3, *out4, *out5, *out6;

	// allocate 6 new blocks
	new1 = AudioStream_F32::allocate_f32();
	new2 = AudioStream_F32::allocate_f32();
	new3 = AudioStream_F32::allocate_f32();
	new4 = AudioStream_F32::allocate_f32();
	new5 = AudioStream_F32::allocate_f32();
	new6 = AudioStream_F32::allocate_f32();
	// but if any fails, allocate none
	if (!new1 || !new2 || !new3 || !new4 || !new5 || !new6) {
		flag_out_of_memory = 1;
		//Serial.println("AudioInputI2SHex_F32 In: out of memory.");
		if (new1) {
			AudioStream_F32::release(new1);
			new1 = NULL;
		}
		if (new2) {
			AudioStream_F32::release(new2);
			new2 = NULL;
		}
		if (new3) {
			AudioStream_F32::release(new3);
			new3 = NULL;
		}
		if (new4) {
			AudioStream_F32::release(new4);
			new4 = NULL;
		}
		if (new5) {
			AudioStream_F32::release(new5);
			new5 = NULL;
		}
		if (new6) {
			AudioStream_F32::release(new6);
			new6 = NULL;
		}
	}
	__disable_irq();
	if (block_offset >= (uint32_t)audio_block_samples) {
		// the DMA filled 6 blocks, so grab them and get the
		// 6 new blocks to the DMA, as quickly as possible
		out1 = block_ch1;
		block_ch1 = new1;
		out2 = block_ch2;
		block_ch2 = new2;
		out3 = block_ch3;
		block_ch3 = new3;
		out4 = block_ch4;
		block_ch4 = new4;
		out5 = block_ch5;
		block_ch5 = new5;
		out6 = block_ch6;
		block_ch6 = new6;
		block_offset = 0;
		__enable_irq();


		update_1chan(0, update_counter, out1); //uses audio_block_samples and update_counter
		update_1chan(1, update_counter, out2); //uses audio_block_samples and update_counter
		update_1chan(2, update_counter, out3); //uses audio_block_samples and update_counter
		update_1chan(3, update_counter, out4); //uses audio_block_samples and update_counter
		update_1chan(4, update_counter, out5); //uses audio_block_samples and update_counter
		update_1chan(5, update_counter, out6); //uses audio_block_samples and update_counter
		

	} else if (new1 != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_ch1 == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_ch1 = new1;
			block_ch2 = new2;
			block_ch3 = new3;
			block_ch4 = new4;
			block_ch5 = new5;
			block_ch6 = new6;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			AudioStream_F32::release(new1);
			AudioStream_F32::release(new2);
			AudioStream_F32::release(new3);
			AudioStream_F32::release(new4);
			AudioStream_F32::release(new5);
			AudioStream_F32::release(new6);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

#else // not supported

void AudioInputI2SHex_F32::begin(void)
{
}



#endif