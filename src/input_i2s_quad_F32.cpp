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
 
 
 /* ******************************
 * NOTE FROM CHIP:  The DMA is configured to invoke the ISR whenever every half of an audio block
 * So, of the audio_block_size is 128 samples, the ISR gets fired every 64 samples.
 * This does not care about the bit-length of each sample.
 * This does not care that this is the "quad" class that's transfering 4 channels per "sample"
 * This code needs to be written under the assumption that the ISR is being called every half of an audio_block,
 * Then the correct number of bytes need to be transfered (audio_block_size / 2) * n_bytes_per_sample * n_channels
 ********************************* */

#include <Arduino.h>
#include "input_i2s_quad_F32.h"
#include "output_i2s_quad_F32.h"
#include "output_i2s_F32.h"

#define NUM_CHAN_TRANSFER (4)         //this class is for quad (4-channel)
#define MAX_BYTES_PER_SAMPLE (4)      //assume 32-bit transfers is the max supported by this class
#define BIG_BUFFER_TYPE uint32_t
#define BYTES_PER_BIG_BUFF_ELEMENT (sizeof(BIG_BUFFER_TYPE)) //assumes the rx buffer is made up of uint32_t
#define LEN_BIG_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_BIG_BUFF_ELEMENT)
DMAMEM __attribute__((aligned(32))) static BIG_BUFFER_TYPE i2s_default_rx_buffer[LEN_BIG_BUFFER];

//To support 16-bit or 32-bit transfers, let's define how much of the RX buffer we're using ...remember, we're only transfering half of audio_block_samples at a time
#define I2S_BUFFER_TO_USE_BYTES (audio_block_samples*NUM_CHAN_TRANSFER*sizeof(i2s_rx_buffer[0]) /  (transferUsing32bit ? 1 : 2)) //divide in half if transferring using 16 bits
#define I2S_BUFFER_MID_POINT_INDEX ((audio_block_samples/2) * NUM_CHAN_TRANSFER * (transferUsing32bit ? 4 : 2) / (sizeof(i2s_rx_buffer[0])))


// initialize static data members
BIG_BUFFER_TYPE *AudioInputI2SQuad_F32::i2s_rx_buffer = i2s_default_rx_buffer;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch1 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch2 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch3 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch4 = NULL;
uint32_t AudioInputI2SQuad_F32::block_offset = 0;
bool AudioInputI2SQuad_F32::update_responsibility = false;
DMAChannel AudioInputI2SQuad_F32::dma(false);



//only compile this file if it is a KinetisK or IMRXT processor
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__IMXRT1062__)

void AudioInputI2SQuad_F32::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	AudioOutputI2SQuad_F32::sample_rate_Hz = sample_rate_Hz;  //these were given in the AudioSettings in the Contructor
	AudioOutputI2SQuad_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the Contructor
	
#if defined(KINETISK)  // This part is only for Teensy3 (ie, Tympan Rev A-D)
	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2SQuad_F32::config_i2s();  //as of Aug 8, 2025, this is still 16-bit only

	//All of the code in this KINETISK section assumes 16-bit I2S transfers, consistent with the 16-bit I2S setup from AudioOutputI2SQuad_F32::config_i2s();
	if (AudioOutputI2SQuad_F32::transferUsing32bit == true) {
		Serial.println("AudioInputI2SQuad_F32: begin: *** WARNING! ***: configured for 32-bit transfers when 16-bit is expected.");
		Serial.flush();
	}

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
	#if defined(__MK20DX256__)
		CORE_PIN30_CONFIG = PORT_PCR_MUX(4); // pin 30, PTC11, I2S0_RXD1
	#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
		CORE_PIN38_CONFIG = PORT_PCR_MUX(4); // pin 38, PTC11, I2S0_RXD1
	#endif

	dma.TCD->SADDR = &I2S0_RDR0;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_SMOD(3) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = 2;
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 4; //original quad
	//dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer			//original quad
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 4; //original quad
	
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;			//new quad, enable diff len audio blocks
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
	
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
	dma.attachInterrupt(isr);

#elif defined(__IMXRT1062__)  //This part is for Teensy4 (ie, Tympan Rev E/F)

	//Setup the pins that control the I2S transfer for two stereo pairs.
	//There are several sets of choices that one can select via "pinoffset"
	const int pinoffset = 0; // TODO: make this configurable...
	AudioOutputI2S_F32::config_i2s(transferUsing32bit, sample_rate_Hz);
	I2S1_RCR3 = I2S_RCR3_RCE_2CH << pinoffset;
	switch (pinoffset) {
	  case 0:
			CORE_PIN8_CONFIG = 3;
			CORE_PIN6_CONFIG = 3;
			IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873
			IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
			break;
	  case 1:
			CORE_PIN6_CONFIG = 3;
			CORE_PIN9_CONFIG = 3;
			IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873
			IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
			break;
	  case 2:
			CORE_PIN9_CONFIG = 3;
			CORE_PIN32_CONFIG = 3;
			IOMUXC_SAI1_RX_DATA2_SELECT_INPUT = 1; // GPIO_B0_11_ALT3, pg 874
			IOMUXC_SAI1_RX_DATA3_SELECT_INPUT = 1; // GPIO_B0_12_ALT3, pg 875
			break;
	}

	// DMA
	//   Each minor loop copies one audio sample from each CODEC (either 2 left or 2 right)
	//   Major loop repeats for audio_block_samples * 2 (stereo)
	#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
	#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
	#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
	#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)
	#define I2S1_RDR                          (IMXRT_SAI1.RDR)
	if (transferUsing32bit) {
		// For 32-bit samples: 
		dma.TCD->SADDR = &(I2S1_RDR[pinoffset]);
		dma.TCD->SOFF = 4;  // This is the separation between sequential RDR registers (def works for 32-bit)
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
	} else {
		// For 16-bit samples:
		dma.TCD->SADDR = (void *)((uint32_t)&I2S1_RDR0 + 2 + pinoffset * 4);  //The "pinoffset *4" shifts to SDR[pinoffset].  The "+ 2" shifts from start of int32 to start of upper int16
		dma.TCD->SOFF = 4;  // This is the separation between sequential RDR registers (is this right for 16 bit?) 
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
	}
	dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
		DMA_TCD_NBYTES_MLOFFYES_MLOFF(-(NUM_CHAN_TRANSFER/2)*4) |  // Restore SADDR after each minor loop. 2 rx registers @ 4 byte spacing
		DMA_TCD_NBYTES_MLOFFYES_NBYTES((NUM_CHAN_TRANSFER/2)*(AudioI2SBase::transferUsing32bit ? 4 : 2));   // copy 2 samples @ 4 bytes or 2 bytes each
		
	dma.TCD->SLAST = -(NUM_CHAN_TRANSFER/2)*4;   // Restore SADDR after last major loop. 2 rx registers @ 4 byte spacing
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = (AudioI2SBase::transferUsing32bit ? 4 : 2);  	// For 32-bit (or 16-bit) samples:

	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;     //allows variable block length
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
	
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);

	I2S1_RCSR = 0;
	I2S1_RCR3 = I2S_RCR3_RCE_2CH << pinoffset;

	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	update_responsibility = update_setup();
	dma.enable();
	dma.attachInterrupt(isr);
#endif
}

void AudioInputI2SQuad_F32::isr(void)
{
	uint32_t daddr;
	const int32_t *src32=nullptr;  //*end;
	const int16_t *src16=nullptr;  //*end;
	uint32_t offset;
	float32_t *dest1_f32, *dest2_f32, *dest3_f32, *dest4_f32;


	//update the dma 
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();
	
	//get pointer for the destination rx buffer
	if (transferUsing32bit) {
		//32-bit transfers
		if (daddr < ((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX)) { //we're copying half an audio block times four channels. One address is already 4bytes, like our 32-bit audio sampels
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src32 = (int32_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX]; //For 32-bit transfers...we are transfering half an audio block and 4 channels each		
			if (AudioInputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src32 = (int32_t *)&i2s_rx_buffer[0];  //for 32-bit transfers
		}
		
	} else {
		//16-bit transfers
		if (daddr < ((uint32_t)i2s_rx_buffer + I2S_BUFFER_MID_POINT_INDEX)) { //we're copying half an audio block times four channels. But, one address is 4bytes whereas our data is only 2bytes, so divide by two
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src16 = (int16_t *)&i2s_rx_buffer[I2S_BUFFER_MID_POINT_INDEX]; //For 16-bit transfers...we are transfering half an audio block and 4 channels each...divided by two because two samples fit in each 32-bit slot
			if (AudioInputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src16 = (int16_t *)&i2s_rx_buffer[0];  //for 16bit transfers
		}
	} 

	//
	//De-interleave and copy to destination audio buffers.  
	//Note the unexpected order!!! Chan 1, 3, 2, 4
	if (block_ch1 && block_ch2 && block_ch3 && block_ch4) {
		offset = AudioInputI2SQuad_F32::block_offset;
		if (offset <= (uint32_t)(audio_block_samples/2)) {  //transfering half of an audio_block_samples at a time
			//This block of code only copies the data into F32 buffers but leaves 
			// the scaling at +/- 2^(32-1) scaling (or at +/- 2^(16-1) for 16-bit transfers)
			//which will then be scaled in the update() method instead of here
			AudioInputI2SQuad_F32::block_offset = offset + audio_block_samples/2;
			dest1_f32 = &(block_ch1->data[offset]);
			dest2_f32 = &(block_ch2->data[offset]);
			dest3_f32 = &(block_ch3->data[offset]);
			dest4_f32 = &(block_ch4->data[offset]);
			
			//copy the data values! (32bit or 16bit, as specified)
			if ((AudioOutputI2SQuad_F32::transferUsing32bit) && (src32 != nullptr)) {
				//32-bits per audio sample
				arm_dcache_delete((void*)src32, I2S_BUFFER_TO_USE_BYTES/2); //ensure cache is up-to-date
				for (int i=0; i < audio_block_samples/2; i++) {
					*dest1_f32++ = ((float32_t) *src32++);  //left 1
					*dest3_f32++ = ((float32_t) *src32++);  //left 2 (note chan 3!!)
					*dest2_f32++ = ((float32_t) *src32++);  //right 1 (note chan 2!!)
					*dest4_f32++ = ((float32_t) *src32++);  //right 2
				}
			} else if ((AudioOutputI2SQuad_F32::transferUsing32bit==false) && (src16 != nullptr)) {
				//16-bits per audio sample
				arm_dcache_delete((void*)src16, I2S_BUFFER_TO_USE_BYTES/2); //ensure cache is up-to-date
				for (int i=0; i < audio_block_samples/2; i++) {
					*dest1_f32++ = ((float32_t) *src16++);  //left 1
					*dest3_f32++ = ((float32_t) *src16++);  //left 2 (note chan 3!!)
					*dest2_f32++ = ((float32_t) *src16++);  //right 1 (note chan 2!!)
					*dest4_f32++ = ((float32_t) *src16++);  //right 2
				}
			} else {	
				//should never be here..but let's be defensive just in case and copy in zeros
				for (int i=0; i < audio_block_samples/2; i++) { *dest1_f32++ = 0.0f;  *dest2_f32++ = 0.0f;  *dest3_f32++ = 0.0f; *dest4_f32++ = 0.0f; }
			}
		}		
	}
}

/* #define I16_TO_F32_NORM_FACTOR (3.051850947599719e-05)  //which is 1/32767 
void AudioInputI2SQuad_F32::scale_i16_to_f32( float32_t *p_i16, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i16++) * I16_TO_F32_NORM_FACTOR); }
}
#define I24_TO_F32_NORM_FACTOR (1.192093037616377e-07)   //which is 1/(2^23 - 1)
void AudioInputI2SQuad_F32::scale_i24_to_f32( float32_t *p_i24, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i24++) * I24_TO_F32_NORM_FACTOR); }
}
#define I32_TO_F32_NORM_FACTOR (4.656612875245797e-10)   //which is 1/(2^31 - 1)
void AudioInputI2SQuad_F32::scale_i32_to_f32( float32_t *p_i32, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i32++) * I32_TO_F32_NORM_FACTOR); }
} */

void AudioInputI2SQuad_F32::update_1chan(int chan, unsigned long counter, audio_block_f32_t *&out_block) {
	if (!out_block) return;
		
	//incoming data is still scaled like int16 (so, +/-32767.).  Here we need to re-scale
	//the values so that the maximum possible audio values spans the F32 stadard of +/-1.0
	if (AudioOutputI2SQuad_F32::transferUsing32bit) {
		AudioInputI2S_F32::scale_i32_to_f32(out_block->data, out_block->data, audio_block_samples);   //for 32-bit transfers
	} else {
		AudioInputI2S_F32::scale_i16_to_f32(out_block->data, out_block->data, audio_block_samples); //for 16-bit transfers
	}
	
	//prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	out_block->id = counter;
	out_block->length = audio_block_samples;

	// then transmit and release the DMA's former blocks
	AudioStream_F32::transmit(out_block, chan);
	AudioStream_F32::release(out_block);
}

void AudioInputI2SQuad_F32::update(void)
{
	audio_block_f32_t *new1, *new2, *new3, *new4;
	audio_block_f32_t *out1, *out2, *out3, *out4;

	// allocate 4 new blocks
	new1 = AudioStream_F32::allocate_f32();
	new2 = AudioStream_F32::allocate_f32();
	new3 = AudioStream_F32::allocate_f32();
	new4 = AudioStream_F32::allocate_f32();
	// but if any fails, allocate none
	if (!new1 || !new2 || !new3 || !new4) {
		flag_out_of_memory = 1;
		//Serial.println("Quad In: out of memory.");
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
	}
	__disable_irq();
	if (block_offset >= (uint32_t)audio_block_samples) {
		// the DMA filled 4 blocks, so grab them and get the
		// 4 new blocks to the DMA, as quickly as possible
		out1 = block_ch1;
		block_ch1 = new1;
		out2 = block_ch2;
		block_ch2 = new2;
		out3 = block_ch3;
		block_ch3 = new3;
		out4 = block_ch4;
		block_ch4 = new4;
		block_offset = 0;
		__enable_irq();
		
		//service the data that we just got out of the DMA
		update_counter++;
		update_1chan(0, update_counter, out1); //uses audio_block_samples and update_counter
		update_1chan(1, update_counter, out2); //uses audio_block_samples and update_counter
		update_1chan(2, update_counter, out3); //uses audio_block_samples and update_counter
		update_1chan(3, update_counter, out4); //uses audio_block_samples and update_counter
		
		
	} else if (new1 != NULL) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_ch1 == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_ch1 = new1;
			block_ch2 = new2;
			block_ch3 = new3;
			block_ch4 = new4;
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			AudioStream_F32::release(new1);
			AudioStream_F32::release(new2);
			AudioStream_F32::release(new3);
			AudioStream_F32::release(new4);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}

#else // not __MK20DX256__

void AudioInputI2SQuad_F32::begin(void)
{
}


#endif