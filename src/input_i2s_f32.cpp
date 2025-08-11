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
 
 
#include "input_i2s_F32.h"
#include "output_i2s_F32.h"
#include <arm_math.h>

#define NUM_CHAN_TRANSFER (2)         //this class is for stereo (2-channel)
#define MAX_BYTES_PER_SAMPLE (4)      //assume 32-bit transfers is the max supported by this class
#define BIG_BUFFER_TYPE uint32_t
#define BYTES_PER_BIG_BUFF_ELEMENT (sizeof(BIG_BUFFER_TYPE)) //assumes the rx buffer is made up of uint32_t
#define LEN_BIG_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_BIG_BUFF_ELEMENT)
DMAMEM __attribute__((aligned(32))) static BIG_BUFFER_TYPE i2s_default_rx_buffer[LEN_BIG_BUFFER];

//To support 16-bit or 32-bit transfers, let's define how much of the RX buffer we're using ...remember, we're only transfering half of audio_block_samples at a time
#define I2S_BUFFER_TO_USE_BYTES (audio_block_samples*NUM_CHAN_TRANSFER*sizeof(i2s_rx_buffer[0]) / (transferUsing32bit ? 1 : 2))
#define I2S_BUFFER_MID_POINT_INDEX ((audio_block_samples/2) * NUM_CHAN_TRANSFER * (transferUsing32bit ? 4 : 2) / (sizeof(i2s_rx_buffer[0])))


// Initialize some static variables
BIG_BUFFER_TYPE * AudioInputI2S_F32::i2s_rx_buffer = i2s_default_rx_buffer;
audio_block_f32_t * AudioInputI2S_F32::block_left_f32 = NULL;
audio_block_f32_t * AudioInputI2S_F32::block_right_f32 = NULL;
uint16_t AudioInputI2S_F32::block_offset = 0;
bool AudioInputI2S_F32::update_responsibility = false;
DMAChannel AudioInputI2S_F32::dma(false);

int AudioInputI2SBase_F32::flag_out_of_memory = 0;
unsigned long AudioInputI2SBase_F32::update_counter = 0;

float AudioInputI2SBase_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;  //set to default, gets set again later during constructor
int AudioInputI2SBase_F32::audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32; //set to default, gets set again later during constructor



void AudioInputI2S_F32::begin(void) {
	dma.begin(true); // Allocate the DMA channel first

	//these static variables for AudioOutputI2S_F32 might be used by the AudioOutputI2S_F32::config_i2s() that we 
	//might be calling here in this methodfor AudioIinputI2S_F32.  In case the user does not instantiate an AudioOutputI2S_F32,
	//we want to make sure thta these static variables match the values given here in AudioInputI2S_F32.
	AudioOutputI2S_F32::sample_rate_Hz = sample_rate_Hz; //these were given in the AudioSettings in the contructor
	AudioOutputI2S_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the contructor
	
	//block_left_1st = NULL;
	//block_right_1st = NULL;

#if defined(KINETISK)
	transferUsing32bit = false;  //is this class ready for 32-bit transfers?  As of Aug 5, 2025, no.  It is not.  So, force to false

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2S_F32::config_i2s(transferUsing32bit);

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
	dma.TCD->SADDR = (void *)((uint32_t)&I2S0_RDR0 + 2);  //From Teensy Audio Library...but why "+ 2"  (Chip 2020-10-31)
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = 2;
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;  //original from Teensy Audio Library
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 2;
	//dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);  //original from Teensy Audio Library
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;  //original from Teensy Audio Library
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX

#elif defined(__IMXRT1062__)
	transferUsing32bit = true;

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2S_F32::config_i2s(transferUsing32bit);

	//setup the pins that control the I2S transfer for one stereo pair
	CORE_PIN8_CONFIG  = 3;  //1:RX_DATA0
	IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2;
	
	// DMA
	//   Each minor loop copies one audio sample from the CODEC (either 1 left or 1 right)
	//   Major loop repeats for audio_block_samples * 2 (stereo)
	#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
	#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
	#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
	#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)
	if (transferUsing32bit) {
		// For 32-bit samples:
		dma.TCD->SADDR = &I2S1_RDR0;
		dma.TCD->SOFF = 0;    // 0 or 4 for stereo?  This is the separation between sequential RDR registers 
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
	} else {
		// For 16-bit samples:
		dma.TCD->SADDR = (void *)((uint32_t)&I2S1_RDR0 + 2);  // "+ 2" to shift to start of high 16-bits in 32-bit register
		dma.TCD->SOFF = 0;    // 0 or 4 for stereo?  This is the separation between sequential RDR registers 
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
	}
	dma.TCD->NBYTES_MLNO = (transferUsing32bit ? 4 : 2); //is this right for 32-bit?
	
	dma.TCD->SLAST = 0;   // 0 or 4 for stereo?  (N_CHAN/2) rx registers @ 4 byte spacing
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = (AudioI2SBase::transferUsing32bit ? 4 : 2);  	// Number of bytes per sample in i2s_rx_buffer
	
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;     //allows variable block length
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.
	
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);

	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
#endif
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
	
	update_counter = 0;
}


void AudioInputI2S_F32::isr(void)
{
	uint32_t daddr, offset;
	const int16_t *src16 = nullptr, *end16 = nullptr;
	const int32_t *src32 = nullptr;
	float32_t *dest_left_f32, *dest_right_f32;
	audio_block_f32_t *left_f32, *right_f32;

	//update the dma 
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	//get pointer for the destination tx buffer
	if (transferUsing32bit) {
		//32-bit
		if (daddr < ((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX)) { //we're copying half an audio block times four channels. One address is already 4bytes, like our 32-bit audio sampels
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src32 = (int32_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX];
			update_counter++; //let's increment the counter here to ensure that we get every ISR resulting in audio
			if (AudioInputI2S_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src32 = (int32_t *)&i2s_rx_buffer[0];
		}
		
		left_f32 = AudioInputI2S_F32::block_left_f32;
		right_f32 = AudioInputI2S_F32::block_right_f32;
		if (left_f32 && right_f32) {
			offset = AudioInputI2S_F32::block_offset;
			if (offset <= (uint32_t)(audio_block_samples/2)) {
				arm_dcache_delete((void*)src32, I2S_BUFFER_TO_USE_BYTES/2);
				dest_left_f32 = &(left_f32->data[offset]);
				dest_right_f32 = &(right_f32->data[offset]);
				AudioInputI2S_F32::block_offset = offset + audio_block_samples/2;
				if (src32 != nullptr) {
					for (int i=0; i < audio_block_samples/2; i++) {
						*dest_left_f32++ = (float32_t) *src32++;
						*dest_right_f32++ = (float32_t) *src32++;
					} 
				} else {
					//shouldn't be here, but let's be defensive
					for (int i=0; i < audio_block_samples/2; i++) {
						*dest_left_f32++ = 0.0f;
						*dest_right_f32++ = 0.0f;
					}
				}					
			}
		}

	} else {
		//16-bit
		if (daddr < (uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX) {
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src16 = (int16_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX];
			end16 = (int16_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX + audio_block_samples/2];			
			update_counter++; //let's increment the counter here to ensure that we get every ISR resulting in audio
			if (AudioInputI2S_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src16 = (int16_t *)&i2s_rx_buffer[0];
			end16 = (int16_t *)&i2s_rx_buffer[0 + audio_block_samples/2];
		}
		left_f32 = AudioInputI2S_F32::block_left_f32;
		right_f32 = AudioInputI2S_F32::block_right_f32;
		if (left_f32 != NULL && right_f32 != NULL) {
			offset = AudioInputI2S_F32::block_offset;
			if (offset <= ((uint32_t) audio_block_samples/2)) {
				arm_dcache_delete((void*)src16, I2S_BUFFER_TO_USE_BYTES/2);
				dest_left_f32 = &(left_f32->data[offset]);
				dest_right_f32 = &(right_f32->data[offset]);
				AudioInputI2S_F32::block_offset = offset + audio_block_samples/2;
				do {
					*dest_left_f32++ = (float32_t) *src16++;
					*dest_right_f32++ = (float32_t) *src16++;
				} while (src16 < end16);
			}
		}
		
	}		
}

#define I16_TO_F32_NORM_FACTOR (3.051850947599719e-05)  //which is 1/32767 
void AudioInputI2S_F32::scale_i16_to_f32( float32_t *p_i16, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i16++) * I16_TO_F32_NORM_FACTOR); }
}
#define I24_TO_F32_NORM_FACTOR (1.192093037616377e-07)   //which is 1/(2^23 - 1)
void AudioInputI2S_F32::scale_i24_to_f32( float32_t *p_i24, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i24++) * I24_TO_F32_NORM_FACTOR); }
}
#define I32_TO_F32_NORM_FACTOR (4.656612875245797e-10)   //which is 1/(2^31 - 1)
void AudioInputI2S_F32::scale_i32_to_f32( float32_t *p_i32, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i32++) * I32_TO_F32_NORM_FACTOR); }
}

 void AudioInputI2S_F32::update_1chan(int chan, audio_block_f32_t *&out_f32) {
	 if (!out_f32) return;
	 
	//scale the float values so that the maximum possible audio values span -1.0 to + 1.0
	if (transferUsing32bit) {
		scale_i32_to_f32(out_f32->data, out_f32->data, audio_block_samples);
	} else {
		scale_i16_to_f32(out_f32->data, out_f32->data, audio_block_samples);
	}

	//prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	out_f32->id = update_counter;
	out_f32->length = audio_block_samples;
		
	//transmit the f32 data!
	AudioStream_F32::transmit(out_f32,chan);

	//release the memory blocks
	AudioStream_F32::release(out_f32);
}
 
void AudioInputI2S_F32::update(void)
{
	static bool flag_beenSuccessfullOnce = false;
	audio_block_f32_t *new_left=NULL, *new_right=NULL, *out_left=NULL, *out_right=NULL;
	
	new_left = AudioStream_F32::allocate_f32();
	new_right = AudioStream_F32::allocate_f32();
	if ((!new_left) || (!new_right)) {
		//ran out of memory.  Clear and return!
		if (new_left) AudioStream_F32::release(new_left);
		if (new_right) AudioStream_F32::release(new_right);
		new_left = NULL;  new_right = NULL;
		flag_out_of_memory = 1;
		if (flag_beenSuccessfullOnce) Serial.println("Input_I2S_F32: update(): WARNING!!! Out of Memory.");
	} else {
		flag_beenSuccessfullOnce = true;
	}
	
	__disable_irq();
	if (block_offset >= audio_block_samples) {
		// the DMA filled 2 blocks, so grab them and get the
		// 2 new blocks to the DMA, as quickly as possible
		out_left = block_left_f32;
		block_left_f32 = new_left;
		out_right = block_right_f32;
		block_right_f32 = new_right;
		block_offset = 0;
		__enable_irq();
		
		//update_counter++; //I chose to update it in the ISR instead.
		update_1chan(0,out_left);  //uses audio_block_samples and update_counter
		update_1chan(1,out_right);  //uses audio_block_samples and update_counter
		
		
	} else if (new_left != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_left_f32 == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_left_f32 = new_left;
			block_right_f32 = new_right;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			AudioStream_F32::release(new_left);
			AudioStream_F32::release(new_right);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

/******************************************************************/


void AudioInputI2Sslave_F32::begin(void)
{
	//Slave mode not valid for Tympan

}






