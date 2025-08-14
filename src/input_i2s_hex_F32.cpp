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
#define LEN_I2S_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_I2S_BUFFER_ELEMENT)
DMAMEM __attribute__((aligned(32))) static I2S_BUFFER_TYPE i2s_default_rx_buffer[LEN_I2S_BUFFER];


#if defined(__IMXRT1062__)   //This whole class only works for Teensy 4 (Tympan Rev E/F)

void AudioInputI2SHex_F32::begin(void)
{
	if (i2s_buffer_was_given_by_user == false) i2s_rx_buffer = i2s_default_rx_buffer; 
	dma.begin(true); // Allocate the DMA channel first

	transferUsing32bit = true; //default to 32-bit transfers when using this class
	AudioOutputI2S_F32::config_i2s(transferUsing32bit, sample_rate_Hz);

	//Setup the pins that control the I2S transfer for two stereo pairs.
	//There are several sets of choices that one can select via "pinoffset"
	const int pinoffset = 0; // TODO: make this configurable...
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
	  DMA_TCD_NBYTES_MLOFFYES_MLOFF(-(n_chan/2)*4) |  // Restore SADDR after each minor loop. 3 rx registers @ 4 byte spacing
	  DMA_TCD_NBYTES_MLOFFYES_NBYTES((n_chan/2)*get_i2sBytesPerSample());   // copy 3 samples @ 4 bytes or 2 bytes each

	dma.TCD->SLAST = -(n_chan/2)*4;   // Restore SADDR after last major loop. 3 rx registers @ 4 byte spacing
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF =  get_i2sBytesPerSample();  	// For 32-bit (or 16-bit) samples:
	
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, Hex uses 3 I2S channels, but that isn't relevant here?)
	dma.TCD->DLASTSGA = -get_i2sBufferToUse_bytes();     //allows variable block length
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


#else // HEX is not supported for non-Teensy4 platforms

void AudioInputI2SHex_F32::begin(void)
{
}

#endif