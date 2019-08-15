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

#include <Arduino.h>
#include "output_i2s_quad_f32.h"
#include "memcpy_audio.h"
#include <arm_math.h>

#if defined(KINETISK)   //only include these for Teensy 3.x (and not Teensy 4)


#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch1_1st = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch2_1st = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch3_1st = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch4_1st = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch1_2nd = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch2_2nd = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch3_2nd = NULL;
audio_block_f32_t * AudioOutputI2SQuad_F32::block_ch4_2nd = NULL;
uint32_t  AudioOutputI2SQuad_F32::ch1_offset = 0;
uint32_t  AudioOutputI2SQuad_F32::ch2_offset = 0;
uint32_t  AudioOutputI2SQuad_F32::ch3_offset = 0;
uint32_t  AudioOutputI2SQuad_F32::ch4_offset = 0;
//audio_block_f32_t * AudioOutputI2SQuad_F32::inputQueueArray[4];
bool AudioOutputI2SQuad_F32::update_responsibility = false;
//DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2*4];  //pack 2 int16s into 1 int32 to make dense, so that 4 channels = 4*(audio_block_samples/2)
DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2*4];
DMAChannel AudioOutputI2SQuad_F32::dma(false);

//static const uint32_t zerodata[AUDIO_BLOCK_SAMPLES/4] = {0};
static const float32_t zerodata[AUDIO_BLOCK_SAMPLES/2] = {0};

//initialize some static variables.  Likely get overwritten by constructor.
float AudioOutputI2SQuad_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;
int AudioOutputI2SQuad_F32::audio_block_samples = AUDIO_BLOCK_SAMPLES;

//#define I2S_BUFFER_TO_USE_BYTES ((AudioOutputI2SQuad_F32::audio_block_samples)*2*sizeof(i2s_tx_buffer[0]))
#define I2S_BUFFER_TO_USE_BYTES ((AudioOutputI2SQuad_F32::audio_block_samples)*4*(sizeof(i2s_tx_buffer[0])/2))


void AudioOutputI2SQuad_F32::begin(void)
{
#if 1
	dma.begin(true); // Allocate the DMA channel first

	block_ch1_1st = NULL;
	block_ch2_1st = NULL;
	block_ch3_1st = NULL;
	block_ch4_1st = NULL;

	// TODO: can we call normal config_i2s, and then just enable the extra output?
	config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0 -> ch1 & ch2
	CORE_PIN15_CONFIG = PORT_PCR_MUX(6); // pin 15, PTC0, I2S0_TXD1 -> ch3 & ch4

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1) | DMA_TCD_ATTR_DMOD(3);
	dma.TCD->NBYTES_MLNO = 4;
	//dma.TCD->SLAST = -sizeof(i2s_tx_buffer); //orig
	dma.TCD->SLAST = -I2S_BUFFER_TO_USE_BYTES; //allows for variable audio block length
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 4;
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4; //allows for variable audio block length
	dma.TCD->DLASTSGA = 0;
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4; //allows for variable audio block length
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	dma.attachInterrupt(isr);
	
	// change the I2S frequencies to make the requested sample rate
	AudioOutputI2S_F32::setI2SFreq(AudioOutputI2SQuad_F32::sample_rate_Hz);
	
	enabled = 1;
#endif
}

void AudioOutputI2SQuad_F32::isr_shuffleDataBlocks(audio_block_f32_t *&block_1st, audio_block_f32_t *&block_2nd, uint32_t &ch_offset)
{
	if (block_1st) {
		if (ch_offset == 0) {
			ch_offset = audio_block_samples/2;
		} else {
			ch_offset = 0;
			AudioStream_F32::release(block_1st);
			block_1st = block_2nd;
			block_2nd = NULL;
		}
	}
}

void AudioOutputI2SQuad_F32::isr(void)
{
	int16_t *dest; //int16 is data type being sent to the audio codec

	//update the dma and get pointer for the destination tx buffer
	uint32_t saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	//if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) { //orig
	if (saddr < (uint32_t)i2s_tx_buffer + I2S_BUFFER_TO_USE_BYTES / 2) { //variable audio block length
		// DMA is transmitting the first half of the buffer so we must fill the second half
		//dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES]; //orig
		dest = (int16_t *)(&i2s_tx_buffer[audio_block_samples]); //new
		if (AudioOutputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
	} else {
		dest = (int16_t *)i2s_tx_buffer;
	}

	//get pointers for source data that we will copy into the tx buffer
	const float32_t *src1, *src2, *src3, *src4;
	const float32_t *zeros = (const float32_t *)zerodata;
	src1 = (block_ch1_1st) ? (&(block_ch1_1st->data[ch1_offset])) : zeros;
	src2 = (block_ch2_1st) ? (&(block_ch2_1st->data[ch2_offset])) : zeros;
	src3 = (block_ch3_1st) ? (&(block_ch3_1st->data[ch3_offset])) : zeros;
	src4 = (block_ch4_1st) ? (&(block_ch4_1st->data[ch4_offset])) : zeros;

	//Interleave and copy into txt buffer (note the unexpected order!!!: chan 1, chan 3, chan 2, chan 4)
	//for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++) {	//original
	for (int i=0; i < audio_block_samples/2; i++) {
		*dest++ = (int16_t) (*src1++); //hopefully the float32 src1 data was pre-scaled in the update() method
		*dest++ = (int16_t) (*src3++);
		*dest++ = (int16_t) (*src2++);
		*dest++ = (int16_t) (*src4++);
	}

	//now, shuffle the 1st and 2nd data block for each channel
	isr_shuffleDataBlocks(block_ch1_1st, block_ch1_2nd, ch1_offset);
	isr_shuffleDataBlocks(block_ch2_1st, block_ch2_2nd, ch2_offset);
	isr_shuffleDataBlocks(block_ch3_1st, block_ch3_2nd, ch3_offset);
	isr_shuffleDataBlocks(block_ch4_1st, block_ch4_2nd, ch4_offset);
	
}


void AudioOutputI2SQuad_F32::scale_f32_to_i16(float32_t *p_f32, float32_t *p_i16, int len) {
	for (int i=0; i<len; i++) { *p_i16++ = max(-32767,min(32767,(*p_f32++) * 32767.f)); }
}
#define F32_TO_I24_NORM_FACTOR (8388607)   //which is 2^23-1
void AudioOutputI2SQuad_F32::scale_f32_to_i24( float32_t *p_f32, float32_t *p_i24, int len) {
	for (int i=0; i<len; i++) { *p_i24++ = max(-F32_TO_I24_NORM_FACTOR,min(F32_TO_I24_NORM_FACTOR,(*p_f32++) * F32_TO_I24_NORM_FACTOR)); }
}
#define F32_TO_I32_NORM_FACTOR (2147483647)   //which is 2^31-1
//define F32_TO_I32_NORM_FACTOR (8388607)   //which is 2^23-1
void AudioOutputI2SQuad_F32::scale_f32_to_i32( float32_t *p_f32, float32_t *p_i32, int len) {
	for (int i=0; i<len; i++) { *p_i32++ = max(-F32_TO_I32_NORM_FACTOR,min(F32_TO_I32_NORM_FACTOR,(*p_f32++) * F32_TO_I32_NORM_FACTOR)); }
	//for (int i=0; i<len; i++) { *p_i32++ = (*p_f32++) * F32_TO_I32_NORM_FACTOR + 512.f*8388607.f; }
}

void AudioOutputI2SQuad_F32::update_1chan(const int chan,  //this is not changed upon return
		audio_block_f32_t *&block_1st, audio_block_f32_t *&block_2nd, uint32_t &ch_offset,
		audio_block_f32_t *&block_f32_scaled) //all three of these are changed upon return  
{
	//Receive the incoming audio blocks
	audio_block_f32_t *block_f32 = receiveReadOnly_f32(chan); // channel 1
	
	//is it valid?
	if (block_f32) {
		//it is valid.  now process it.
		
		if (chan == 0) {
			if (block_f32->length != audio_block_samples) {
				Serial.print("AudioOutputI2SQuad_F32: *** WARNING ***: audio_block says len = ");
				Serial.print(block_f32->length);
				Serial.print(", but I2S settings want it to be = ");
				Serial.println(audio_block_samples);
			}
		} 
	
		//scale F32 to Int16
		//audio_block_f32_t *block_f32_scaled = AudioStream_F32::allocate_f32();
		scale_f32_to_i16(block_f32->data, block_f32_scaled->data, audio_block_samples);
		
		//shuffle between the two buffers that the isr() routines looks for
		__disable_irq();
		if (block_1st == NULL) {
			block_1st = block_f32_scaled;
			ch_offset = 0;
			__enable_irq();
		} else if (block_2nd == NULL) {
			block_2nd = block_f32_scaled;
			__enable_irq();
		} else {
			audio_block_f32_t *tmp = block_1st;
			block_1st = block_2nd;
			block_2nd = block_f32_scaled;
			ch_offset = 0;
			__enable_irq();
			AudioStream_F32::release(tmp);
		}
		AudioStream_F32::transmit(block_f32,chan); AudioStream_F32::release(block_f32);	
	}

}

//This routine receives an audio block of F32 data from audio processing
//routines.  This routine will receive one block from each audio channel.
//This routine must receive that data, scale it (not convert it) to I16 fullscale
//from F32 fullscale, and stage it (via the correct pointers) so that the isr
// can find all four audio blocks, cast them to I16, interleave them, and put them
//into the DMA that the I2S peripheral pulls from.
void AudioOutputI2SQuad_F32::update(void)
{
	//pre-allocate working memory for each of the four channels...will be released
	//inside the update_1chan function when it is really free
	audio_block_f32_t *foo1_f32 = AudioStream_F32::allocate_f32();
	audio_block_f32_t *foo2_f32 = AudioStream_F32::allocate_f32();
	audio_block_f32_t *foo3_f32 = AudioStream_F32::allocate_f32();
	audio_block_f32_t *foo4_f32 = AudioStream_F32::allocate_f32();
	if ((!foo1_f32) || (!foo2_f32) || (!foo3_f32) || (!foo4_f32)) {
		//enough mem was not available!  clear out and return
		if (foo1_f32) AudioStream_F32::release(foo1_f32);
		if (foo2_f32) AudioStream_F32::release(foo2_f32);
		if (foo3_f32) AudioStream_F32::release(foo3_f32);
		if (foo4_f32) AudioStream_F32::release(foo4_f32);
		return;
	}
	
	//process (shuffle data) for each audio channel
	update_1chan(0, block_ch1_1st, block_ch1_2nd, ch1_offset, foo1_f32);
	update_1chan(1, block_ch2_1st, block_ch2_2nd, ch2_offset, foo2_f32);
	update_1chan(2, block_ch3_1st, block_ch3_2nd, ch3_offset, foo3_f32);
	update_1chan(3, block_ch4_1st, block_ch4_2nd, ch4_offset, foo4_f32);
}


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
  #define MCLK_MULT 8
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 4
  #define MCLK_DIV  85
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

void AudioOutputI2SQuad_F32::config_i2s(void)
{
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


#else // not __MK20DX256__


void AudioOutputI2SQuad_F32::begin(void)
{
}

void AudioOutputI2SQuad_F32::update(void)
{
	audio_block_f32_t *block_f32;

	block_f32 = receiveReadOnly_f32(0);
	if (block_f32) release(block_f32);
	block_f32 = receiveReadOnly_f32(1);
	if (block_f32) release(block_f32);
	block_f32 = receiveReadOnly_f32(2);
	if (block_f32) release(block_f32);
	block_f32 = receiveReadOnly_f32(3);
	if (block_f32) release(block_f32);
}

#endif
#endif