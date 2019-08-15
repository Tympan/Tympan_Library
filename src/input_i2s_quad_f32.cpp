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
 
 #if defined(KINETISK)   //only include these for Teensy 3.x (and not Teensy 4)


#include <Arduino.h>
#include "input_i2s_quad_f32.h"
#include "output_i2s_quad_f32.h"

DMAMEM static uint32_t i2s_rx_buffer[AUDIO_BLOCK_SAMPLES/2*4];
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch1 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch2 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch3 = NULL;
audio_block_f32_t * AudioInputI2SQuad_F32::block_ch4 = NULL;
uint32_t AudioInputI2SQuad_F32::block_offset = 0;
bool AudioInputI2SQuad_F32::update_responsibility = false;
DMAChannel AudioInputI2SQuad_F32::dma(false);
int AudioInputI2SQuad_F32::flag_out_of_memory = 0;

float AudioInputI2SQuad_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;
int AudioInputI2SQuad_F32::audio_block_samples = AUDIO_BLOCK_SAMPLES;

#define I2S_BUFFER_TO_USE_BYTES ((AudioOutputI2SQuad_F32::audio_block_samples)*4*(sizeof(i2s_rx_buffer[0])/2))


#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

void AudioInputI2SQuad_F32::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	AudioOutputI2SQuad_F32::sample_rate_Hz = sample_rate_Hz;  //these were given in the AudioSettings in the Contructor
	AudioOutputI2SQuad_F32::audio_block_samples = audio_block_samples;//these were given in the AudioSettings in the Contructor
	
	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputI2SQuad_F32::config_i2s();

	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
#if defined(__MK20DX256__)
	CORE_PIN30_CONFIG = PORT_PCR_MUX(4); // pin 30, PTC11, I2S0_RXD1
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	CORE_PIN38_CONFIG = PORT_PCR_MUX(4); // pin 38, PTC11, I2S0_RXD1
#endif


#if defined(KINETISK)
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
	
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES/ 4; //new quad, enable diff len audio blocks
	dma.TCD->DLASTSGA = -I2S_BUFFER_TO_USE_BYTES;			//new quad, enable diff len audio blocks
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4;//new quad, enable diff len audio blocks
	
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
#endif
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
	dma.attachInterrupt(isr);
}

void AudioInputI2SQuad_F32::isr(void)
{
	uint32_t daddr, offset;
	const int16_t *src;  //*end;
	float32_t *dest1_f32, *dest2_f32, *dest3_f32, *dest4_f32;

	//digitalWriteFast(3, HIGH);
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	//if (daddr < (uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2) { //orig quad
	if (daddr < (uint32_t)i2s_rx_buffer + I2S_BUFFER_TO_USE_BYTES / 2) { //new quad, enable diff audio block lengths
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = (int16_t *)&i2s_rx_buffer[audio_block_samples];
		//end = (int16_t *)&i2s_rx_buffer[audio_block_samples*2];
		if (AudioInputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = (int16_t *)&i2s_rx_buffer[0];
		//end = (int16_t *)&i2s_rx_buffer[audio_block_samples];
	}
	
	//De-interleave and copy to destination audio buffers.  
	//Note the unexpected order!!! Chan 1, 3, 2, 4
	if (block_ch1 && block_ch2 && block_ch3 && block_ch4) {
		offset = AudioInputI2SQuad_F32::block_offset;
		if (offset <= (uint32_t)(audio_block_samples/2)) {
			AudioInputI2SQuad_F32::block_offset = offset + audio_block_samples/2;
			dest1_f32 = &(block_ch1->data[offset]);
			dest2_f32 = &(block_ch2->data[offset]);
			dest3_f32 = &(block_ch3->data[offset]);
			dest4_f32 = &(block_ch4->data[offset]);
			for (int i=0; i < audio_block_samples/2; i++) {
				*dest1_f32++ = (float32_t) *src++; //will need to scale this in update()
				*dest3_f32++ = (float32_t) *src++;//will need to scale this in update()
				*dest2_f32++ = (float32_t) *src++;//will need to scale this in update()
				*dest4_f32++ = (float32_t) *src++;//will need to scale this in update()
			}
		}
	} //else {
	//	Serial.println("Quad: ISR: do not have all data blocks.");
	//}
	//digitalWriteFast(3, LOW);
}

#define I16_TO_F32_NORM_FACTOR (3.051850947599719e-05)  //which is 1/32767 
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
}

void AudioInputI2SQuad_F32::update_1chan(int chan, audio_block_f32_t *&out_block) {
	if (!out_block) return;
		
	//scale the float values so that the maximum possible audio values span -1.0 to + 1.0
	scale_i16_to_f32(out_block->data, out_block->data, audio_block_samples);
	
	//prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	out_block->id = update_counter;

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
		update_1chan(0, out1); //uses audio_block_samples and update_counter
		update_1chan(1, out2); //uses audio_block_samples and update_counter
		update_1chan(2, out3); //uses audio_block_samples and update_counter
		update_1chan(3, out4); //uses audio_block_samples and update_counter
		
		
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

#endif