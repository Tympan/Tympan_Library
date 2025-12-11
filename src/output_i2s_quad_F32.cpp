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
#include "output_i2s_quad_F32.h"
//include "memcpy_audio.h"
#include "utility/memcpy_audio_tympan.h"
#include <arm_math.h>
#include "output_i2s_F32.h"  //for config_i2s() and setI2Sfreq_T3() and scale_f32_to_i16()

//only compile this file if it is a KinetisK or IMRXT processor
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__IMXRT1062__)


// Define the full TX buffer, big enough to support 32-bit or 16-bit transfers
#define NUM_CHAN_TRANSFER (4)         //this class is for quad (4-channel)
#define MAX_BYTES_PER_SAMPLE (4)      //assume 32-bit transfers is the max supported by this class
#define LEN_I2S_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_I2S_BUFFER_ELEMENT)
DMAMEM __attribute__((aligned(32))) static I2S_BUFFER_TYPE i2s_default_tx_buffer[LEN_I2S_BUFFER];



void AudioOutputI2SQuad_F32::begin(void)
{
	if (i2s_buffer_was_given_by_user == false) i2s_tx_buffer = i2s_default_tx_buffer;
	dma.begin(true); // Allocate the DMA channel first

	initPointers();

	#if defined(KINETISK) //this code is for Teensy 3.x, not Teensy 4
		transferUsing32bit=false; //As it currently stands, this Teensy 3.x code only supports 16-bit transfers

		// TODO: can we call normal config_i2s, and then just enable the extra output?
		config_i2s();  //this is the local config_i2s(), not the one in the non-quad version of output_i2s.  does it know about block size?  (sample rate is defined is set ~25 rows below here)
		CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0 -> ch1 & ch2
		CORE_PIN15_CONFIG = PORT_PCR_MUX(6); // pin 15, PTC0, I2S0_TXD1 -> ch3 & ch4

		dma.TCD->SADDR = i2s_tx_buffer;
		dma.TCD->SOFF = 2;
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1) | DMA_TCD_ATTR_DMOD(3);
		dma.TCD->NBYTES_MLNO = 4;
		//dma.TCD->SLAST = -sizeof(i2s_tx_buffer); //orig
		dma.TCD->SLAST = -get_i2sBufferToUse_bytes(); //allows for variable audio block length
		dma.TCD->DADDR = &I2S0_TDR0;
		dma.TCD->DOFF = 4;
		//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
		dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
		dma.TCD->DLASTSGA = 0;
		//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
		dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
		dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
		update_responsibility = update_setup();
		dma.enable();

		I2S0_TCSR = I2S_TCSR_SR;
		I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
		dma.attachInterrupt(isr);
		
		// change the I2S frequencies to make the requested sample rate
		AudioOutputI2S_F32::setI2SFreq_T3(sample_rate_Hz);
	
	#elif defined(__IMXRT1062__) //this is for Teensy 4
		transferUsing32bit = true;  //The Teensy 4 code within this section is all written assuming 32-bit transfers

		//Setup which sets of pins we'll be using for the Quad I2S (there are a few choices)
		const int pinoffset = 0; // TODO: make this configurable...
		//memset(i2s_tx_buffer, 0, sizeof(i2s_tx_buffer)); //WEA 2023-09-28: commented out because we've already initialized the array to zero when we created the array 
		AudioOutputI2S_F32::config_i2s(transferUsing32bit,sample_rate_Hz);
		I2S1_TCR3 = I2S_TCR3_TCE_2CH << pinoffset;
		switch (pinoffset) {
		  case 0:
				CORE_PIN7_CONFIG  = 3;
				CORE_PIN32_CONFIG = 3;
				break;
		  case 1:
				CORE_PIN32_CONFIG = 3;
				CORE_PIN9_CONFIG  = 3;
				break;
		  case 2:
				CORE_PIN9_CONFIG  = 3;
				CORE_PIN6_CONFIG  = 3;
		}

		// DMA
		//   Each minor loop copies one audio sample destined for each CODEC (either 2 left or 2 right)
		//   Major loop repeats for all samples in i2s_tx_buffer
		#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
		#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
		#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
		#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)
		#define I2S1_TDR                (IMXRT_SAI1.TDR)
		dma.TCD->SADDR = i2s_tx_buffer;
		dma.TCD->SOFF = (transferUsing32bit ? 4 : 2); //4 for 32-bit, 2 for 16-bit
		if (transferUsing32bit) {
			// For 32-bit samples:
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
			dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE |
				DMA_TCD_NBYTES_MLOFFYES_MLOFF(-((n_chan/2)*4)) |  // Restore DADDR after each minor loop. 2 samples @ 4 bytes each (unrelated to bit depth)
				DMA_TCD_NBYTES_MLOFFYES_NBYTES((n_chan/2)*get_i2sBytesPerSample());   // Minor loop is 2 samples @ 4 (or 2) bytes each
			dma.TCD->SLAST = -get_i2sBufferToUse_bytes(); //allows for variable audio block length		
			dma.TCD->DADDR = &(I2S1_TDR[pinoffset]);
		} else {
			// For 16-bit samples:
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
			dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE |
				DMA_TCD_NBYTES_MLOFFYES_MLOFF(-((n_chan/2)*4)) |  // Restore DADDR after each minor loop. 2 samples @ 4 bytes each (unrelated to bit depth)
				DMA_TCD_NBYTES_MLOFFYES_NBYTES((n_chan/2)*get_i2sBytesPerSample());   // Minor loop is 2 samples @ 4 (or 2) bytes each
			dma.TCD->SLAST = -get_i2sBufferToUse_bytes(); //allows for variable audio block length
			dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2 + pinoffset * 4);
		}
		
		//settings that are common between 32-bit and 16-bit transfers
		dma.TCD->DOFF = 4;  // This is the separation between sequential TDR registers
		dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
		dma.TCD->DLASTSGA = -(n_chan/2)*4;
		dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.  (Yes, quad uses 2 I2S channels, but that isn't relevant here?)
		
		dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
		dma.enable();
		I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
		I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
		I2S1_TCR3 = I2S_TCR3_TCE_2CH << pinoffset;
		update_responsibility = update_setup();
		dma.attachInterrupt(isr);
		
	#endif  //ends   elif defined(__IMXRT1062__)
	
	//enabled = 1;

}


#if defined(KINETISK)   //the code below is only used by Teensy3 (ie, Tympan Rev 
// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 4
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 8
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 16
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 1
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#elif F_CPU == 240000000
  #define MCLK_MULT 2
  #define MCLK_DIV  85
  #define MCLK_SRC  0
#elif F_CPU == 16000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif

void AudioOutputI2SQuad_F32::config_i2s(void)  //this is only used by Teensy3 (ie, Tympan Revs A-D) ???
{
	//this method is only used by Teensy3.x and this Teensy3 code is written assuming 16-bit I2S transfers
	transferUsing32bit = false;
	
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	I2S0_TCR3 = I2S_TCR3_TCE_2CH;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(3);
	I2S0_RCR3 = I2S_RCR3_RCE_2CH;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}

#endif // KINETISK


#else // not supported

void AudioOutputI2SQuad::begin(void)
{
}

void AudioOutputI2SQuad::update(void)
{
	audio_block_t *block;

	block = receiveReadOnly(0);
	if (block) release(block);
	block = receiveReadOnly(1);
	if (block) release(block);
	block = receiveReadOnly(2);
	if (block) release(block);
	block = receiveReadOnly(3);
	if (block) release(block);
}

#endif