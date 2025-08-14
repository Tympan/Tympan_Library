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
#define LEN_I2S_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_I2S_BUFFER_ELEMENT)
DMAMEM __attribute__((aligned(32))) static I2S_BUFFER_TYPE i2s_default_rx_buffer[LEN_I2S_BUFFER];

void AudioInputI2S_F32::begin(void) 
{
	i2s_rx_buffer = i2s_default_rx_buffer; 
	dma.begin(true); // Allocate the DMA channel first

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
	dma.TCD->CITER_ELINKNO = get_i2sBufferToUse_bytes() / 2;
	//dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);  //original from Teensy Audio Library
	dma.TCD->DLASTSGA = -get_i2sBufferToUse_bytes();
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;  //original from Teensy Audio Library
	dma.TCD->BITER_ELINKNO = get_i2sBufferToUse_bytes() / 2;
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
		dma.TCD->SOFF = 0;    // This is the separation between sequential RDR registers.
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
	}
	dma.TCD->NBYTES_MLNO = get_i2sBytesPerSample(); //is this right for 32-bit?
	
	dma.TCD->SLAST = 0;   // 0 or 4 for stereo?  (N_CHAN/2) rx registers @ 4 byte spacing
	dma.TCD->DADDR = i2s_rx_buffer;
	dma.TCD->DOFF = get_i2sBytesPerSample();  	// Number of bytes per sample in i2s_rx_buffer
	
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  
	dma.TCD->DLASTSGA = -get_i2sBufferToUse_bytes();     //allows variable block length
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






