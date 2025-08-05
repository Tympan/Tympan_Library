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
#include "output_i2s_F32.h"  //for config_i2s() and setI2Sfreq_T3()


#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__) || defined(__IMXRT1062__)

//if defined(__IMXRT1062__)
//include "utility/imxrt_hw.h"
//endif


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
//q15_t AudioOutputI2SQuad_F32::tmp_src1[AUDIO_BLOCK_SAMPLES/2];
//q15_t AudioOutputI2SQuad_F32::tmp_src2[AUDIO_BLOCK_SAMPLES/2];
//q15_t AudioOutputI2SQuad_F32::tmp_src3[AUDIO_BLOCK_SAMPLES/2];
//q15_t AudioOutputI2SQuad_F32::tmp_src4[AUDIO_BLOCK_SAMPLES/2];

bool AudioOutputI2SQuad_F32::update_responsibility = false;
//DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[MAX_AUDIO_BLOCK_SAMPLES_F32/2*4];  //pack 2 int16s into 1 int32 to make dense, so that 4 channels = 4*(audio_block_samples/2)
//DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*4];  //pack 1 int32 into 1 int32 to make dense, so that 4 channels = 4*(audio_block_samples)
//DMAMEM __attribute__((aligned(32))) static uint32_t i2s_default_tx_buffer[MAX_AUDIO_BLOCK_SAMPLES_F32/2*4];  //pack 2 int16s into 1 int32 to make dense, so that 4 channels = 4*(audio_block_samples/2)
DMAMEM __attribute__((aligned(32))) static uint32_t i2s_default_tx_buffer[MAX_AUDIO_BLOCK_SAMPLES_F32*4];  //for 32-bit data.  Simple...pack 1 int32 into 1 int32, so that 4 channels = 4*audio_block_samples
uint32_t * AudioOutputI2SQuad_F32::i2s_tx_buffer = i2s_default_tx_buffer;
DMAChannel AudioOutputI2SQuad_F32::dma(false);

//static const uint32_t zerodata[AUDIO_BLOCK_SAMPLES/4] = {0}; //original from Teensy Audio Library
//static const float32_t zerodata[MAX_AUDIO_BLOCK_SAMPLES_F32/2] = {0}; //modified for F32.  Need zeros for half an audio block
//static float32_t default_zerodata[MAX_AUDIO_BLOCK_SAMPLES_F32/2] = {0}; //modified for F32.  Need zeros for half an audio block
static float32_t default_zerodata[MAX_AUDIO_BLOCK_SAMPLES_F32] = {0}; //modified for F32.  Need zeros for half an audio block
float32_t * AudioOutputI2SQuad_F32::zerodata = default_zerodata;

//initialize some static variables.  Likely get overwritten by constructor.
float AudioOutputI2SQuad_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;
int AudioOutputI2SQuad_F32::audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;

//for 16-bit transfers (we're packing two int6 values packed into a 32-bit data type) multiplied by 4 channels...later, you'll see that we'll be transfering half of audio_bock_samples at a time
// #define I2S_BUFFER_TO_USE_BYTES ((AudioOutputI2SQuad_F32::audio_block_samples)*4*sizeof(i2s_tx_buffer[0])/2)

//for 32-bit transfers (into a 32-bit data type) multiplied by 4 channels...later, you'll see that we'll be transfering half of audio_bock_samples at a time
#define I2S_BUFFER_TO_USE_BYTES ((AudioOutputI2SQuad_F32::audio_block_samples)*4*sizeof(i2s_tx_buffer[0]))

void AudioOutputI2SQuad_F32::begin(void)
{
	//Serial.println("AudioOutputI2SQuad_F32: begin: starting...");
	dma.begin(true); // Allocate the DMA channel first

	block_ch1_1st = NULL;
	block_ch2_1st = NULL;
	block_ch3_1st = NULL;
	block_ch4_1st = NULL;

	#if defined(KINETISK) //this code is for Teensy 3.x, not Teensy 4
		transferUsing32bit=false;
		i2s_buffer_to_use_bytes = (I2S_BUFFER_TO_USE_BYTES)/2
	
		// TODO: can we call normal config_i2s, and then just enable the extra output?
		config_i2s();  //this is the local config_i2s(), not the one in the non-quad version of output_i2s.  does it know about block size?  (sample rate is defined is set ~25 rows below here)
		CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0 -> ch1 & ch2
		CORE_PIN15_CONFIG = PORT_PCR_MUX(6); // pin 15, PTC0, I2S0_TXD1 -> ch3 & ch4

		dma.TCD->SADDR = i2s_tx_buffer;
		dma.TCD->SOFF = 2;
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1) | DMA_TCD_ATTR_DMOD(3);
		dma.TCD->NBYTES_MLNO = 4;
		//dma.TCD->SLAST = -sizeof(i2s_tx_buffer); //orig
		dma.TCD->SLAST = -i2s_buffer_to_use_bytes; //allows for variable audio block length
		dma.TCD->DADDR = &I2S0_TDR0;
		dma.TCD->DOFF = 4;
		//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
		dma.TCD->CITER_ELINKNO = i2s_buffer_to_use_bytes / 4; //allows for variable audio block length
		dma.TCD->DLASTSGA = 0;
		//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 4; //orig
		dma.TCD->BITER_ELINKNO = i2s_buffer_to_use_bytes / 4; //allows for variable audio block length
		dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
		update_responsibility = update_setup();
		dma.enable();

		I2S0_TCSR = I2S_TCSR_SR;
		I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
		dma.attachInterrupt(isr);
		
		// change the I2S frequencies to make the requested sample rate
		AudioOutputI2S_F32::setI2SFreq_T3(AudioOutputI2SQuad_F32::sample_rate_Hz);
	
	#elif defined(__IMXRT1062__) //this is for Teensy 4
		transferUsing32bit = true;  //32 bit from AIC
		i2s_buffer_to_use_bytes = I2S_BUFFER_TO_USE_BYTES
	
		const int pinoffset = 0; // TODO: make this configurable...
		//memset(i2s_tx_buffer, 0, sizeof(i2s_tx_buffer)); //WEA 2023-09-28: commented out because we've already initialized the array to zero when we created the array 
		AudioOutputI2S_F32::config_i2s(transferUsing32bit,AudioOutputI2SQuad_F32::sample_rate_Hz);
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

		dma.TCD->SADDR = i2s_tx_buffer;

		#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
		#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
		#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
		#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)

		if (transferUsing32bit) {
			// For 32-bit samples:
			dma.TCD->SOFF = 4;  // Number of bytes per sample in i2s_tx_buffer
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
			dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE |
				DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) |  // Restore DADDR after each minor loop (= 2 * DOFF)
				DMA_TCD_NBYTES_MLOFFYES_NBYTES(8);   // Minor loop is 2 samples @ 4 bytes each
			//dma.TCD->SLAST = -sizeof(i2s_tx_buffer); //original from Teensy Audio Library	
			dma.TCD->SLAST = -i2s_buffer_to_use_bytes; //allows for variable audio block length		
			#define I2S1_TDR                (IMXRT_SAI1.TDR)
			dma.TCD->DADDR = &(I2S1_TDR[pinoffset]);
			
		} else {
			// For 16-bit samples:
			dma.TCD->SOFF = 2;  //is 2 in Teensy 
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
			dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE |
				DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) |
				DMA_TCD_NBYTES_MLOFFYES_NBYTES(4);
			//dma.TCD->SLAST = -sizeof(i2s_tx_buffer); //original from Teensy Audio Library
			dma.TCD->SLAST = -i2s_buffer_to_use_bytes; //allows for variable audio block length
			dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2 + pinoffset * 4);

		}
		
		//settings that are common between 32-bit and 16-bit transfers
		dma.TCD->DOFF = 4;  // This is the separation between sequential TDR registers
		//dma.TCD->CITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2; //original from Teensy Audio Library (16-bit)
		dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //allows for variable audio block length (2 is for stereo pair)
		dma.TCD->DLASTSGA = -8;
		//dma.TCD->BITER_ELINKNO = AUDIO_BLOCK_SAMPLES * 2; //original from Teensy Audio Library (16-bit)
		dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //allows for variable audio block length (assumes 16-bit?)
		
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

void AudioOutputI2SQuad_F32::isr_shuffleDataBlocks(audio_block_f32_t *&block_1st, audio_block_f32_t *&block_2nd, uint32_t &ch_offset)
{
	if (block_1st) {
		if (ch_offset == 0) {
			//ch_offset = AUDIO_BLOCK_SAMPLES / 2;
			ch_offset = audio_block_samples/2;
			
		} else {
			ch_offset = 0;
			AudioStream_F32::release(block_1st);
			block_1st = block_2nd;
			block_2nd = NULL;
		}
	}
}

 
 //#define SCALE_AND_SATURATE_F32_TO_INT16(x) (min(32767.0f,max(-32767.0f, (x) * 32767.0f)))

 void AudioOutputI2SQuad_F32::isr(void)
{
	uint32_t saddr;
	float32_t *src1, *src2, *src3, *src4;
	float32_t *zeros = (float32_t *)zerodata;
	int32_t *dest32; //32 bit transfers
	int16_t *dest32; //32 bit transfers
	
	if (transferUsing32bit) {
	
		//update the dma and get pointer for the destination tx buffer
		saddr = (uint32_t)(dma.TCD->SADDR);
		dma.clearInterrupt();
		if (saddr < (uint32_t)i2s_tx_buffer + ((audio_block_samples/2)*4)) {  //we're copying half an audio block times four channels.  One address is already 4bytes, like our 32-bit audio sampels
			// DMA is transmitting the first half of the buffer so, here in this isr(), we must fill the second half
			dest32 = (int32_t *)&i2s_tx_buffer[audio_block_samples/2*4]; //For 32-bit transfers...we are transfering half an audio block and 4 channels each
			if (AudioOutputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is transmitting the second half of the buffer so, here in this isr(), we must fill the first half
			dest32 = (int32_t *)i2s_tx_buffer;  //start of the TX buffer, 32-bit transfers
		}
		
	} else {	
	
		//update the dma and get pointer for the destination tx buffer
		saddr = (uint32_t)(dma.TCD->SADDR);
		dma.clearInterrupt();
		if (saddr < (uint32_t)i2s_tx_buffer + ((audio_block_samples/2)*4)/2) {  //we're copying half an audio block times four channels.  One address is 4bytes, but our samples are 2bytes, so divide by 2
			// DMA is transmitting the first half of the buffer so, here in this isr(), we must fill the second half
			dest16 = (int16_t *)&i2s_tx_buffer[((audio_block_samples/2)*4)/2]; //For 16-bit transfers
			if (AudioOutputI2SQuad_F32::update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is transmitting the second half of the buffer so, here in this isr(), we must fill the first half
			dest16 = (int16_t *)i2s_tx_buffer;  //start of the TX buffer, 16-bit transfers
		}
		
	}

	//get pointers for source data that we will copy into the tx buffer
	//const float32_t *zeros = (const float32_t *)zerodata; //for 32-bit data??
	src1 = (block_ch1_1st) ? (&(block_ch1_1st->data[ch1_offset])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
	src2 = (block_ch2_1st) ? (&(block_ch2_1st->data[ch2_offset])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch2
	src3 = (block_ch3_1st) ? (&(block_ch3_1st->data[ch3_offset])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch3
	src4 = (block_ch4_1st) ? (&(block_ch4_1st->data[ch4_offset])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch4


	//This block of code assumes that the audio data HAS ALREADY been scaled to +/- float32(2**31 - 1)
	//interleave the given source data into the output array
	if (transferUsing32bit) {
		int32_t *d = dest32;  //32 bit transfers
		//for (int i=0; i < audio_block_samples / 2; i++) {  //words for int16 data transfers?...why divided by 2?
		for (int i=0; i < audio_block_samples / 2; i++) { //copying half the buffer
			*d++ = (int32_t)(*src1++); //left 1...retrieve scaled f32 value, cast to int32 type, copy to destination
			*d++ = (int32_t)(*src3++); //left 2  (note it is src3, not src2!!!)  ...retrieve scaled f32 value, cast to int32 type, copy to destination
			*d++ = (int32_t)(*src2++); //right 1 (note it is src2, not src3!!!  ...retrieve scaled f32 value, cast to int32 type, copy to destination
			*d++ = (int32_t)(*src4++); //right 2  ...retrieve scaled f32 value, cast to int32 type, copy to destination
		}	
		
		arm_dcache_flush_delete(dest32, i2s_buffer_to_use_bytes / 2);  //flush the cache for all the bytes that we filled (the /2 should be correct...we filled half the buffer)

	} else {
		int16_t *d = dest16;  //16 bit transfers
		for (int i=0; i < audio_block_samples / 2; i++) {
			*d++ = (int16_t)(*src1++); //left 1
			*d++ = (int16_t)(*src3++); //left 2  (note it is src3, not src2!!!)
			*d++ = (int16_t)(*src2++); //right 1 (note it is src2, not src3!!!
			*d++ = (int16_t)(*src4++); //right 2
		}
		
		arm_dcache_flush_delete(dest16, i2s_buffer_to_use_bytes / 2);  //flush the cache for all the bytes that we filled (the /2 should be correct...we filled half the buffer)

	}

	//now, shuffle the 1st and 2nd data block for each channel
	isr_shuffleDataBlocks(block_ch1_1st, block_ch1_2nd, ch1_offset);
	isr_shuffleDataBlocks(block_ch2_1st, block_ch2_2nd, ch2_offset);
	isr_shuffleDataBlocks(block_ch3_1st, block_ch3_2nd, ch3_offset);
	isr_shuffleDataBlocks(block_ch4_1st, block_ch4_2nd, ch4_offset);
	
}  


//scale with saturation
/* #define F32_TO_I16_NORM_FACTOR (32767)   //which is 2^15-1
void AudioOutputI2SQuad_F32::scale_f32_to_i16(float32_t *p_f32, float32_t *p_i16, int len) {
	for (int i=0; i<len; i++) { *p_i16++ = max(-F32_TO_I16_NORM_FACTOR,min(F32_TO_I16_NORM_FACTOR,(*p_f32++) * F32_TO_I16_NORM_FACTOR)); }
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
} */

 
void AudioOutputI2SQuad_F32::update_1chan(const int chan,  //this is not changed upon return
		audio_block_f32_t *&block_1st, audio_block_f32_t *&block_2nd, uint32_t &ch_offset) //all three of these are changed upon return  
{

	//get some working memory
	audio_block_f32_t *block_f32_scaled = AudioStream_F32::allocate_f32(); //allocate for scaled data (from F32 scale of +/- 1.0 to int16 scale of +/- 32767)
	if (block_f32_scaled==NULL) return;  //fail
	
	//Receive the incoming audio blocks
	audio_block_f32_t *block_f32 = receiveReadOnly_f32(chan); // get one channel

	//Is there any data?
	if (block_f32 == NULL) {
		//Serial.print("Output_i2s_quad: update: did not receive chan " + String(chan));
		
		//fill with zeros
		for (int i=0; i<audio_block_samples; i++) block_f32_scaled->data[i] = 0.0f;

	} else {
		//Process the given data	
		if (chan == 0) {
			if (block_f32->length != audio_block_samples) {
				Serial.print("AudioOutputI2SQuad_F32: *** WARNING ***: audio_block says len = ");
				Serial.print(block_f32->length);
				Serial.print(", but I2S settings want it to be = ");
				Serial.println(audio_block_samples);
			}
		} 
		
		//scale the F32 data (+/- 1.0) to fit within Int data type, though we're still float32 data type
		if (transferUsing32bit) {
			AudioOutputI2S_F32::scale_f32_to_i32(block_f32->data, block_f32_scaled->data, audio_block_samples); //assume Int32
		} else {
			AudioOutputI2S_F32::scale_f32_to_i16(block_f32->data, block_f32_scaled->data, audio_block_samples); //assume Int16
		}	
		block_f32_scaled->length = block_f32->length;
		block_f32_scaled->id = block_f32->id;
		block_f32_scaled->fs_Hz  = block_f32->fs_Hz;
		
		//transmit and release the original (not scaled) audio data
		AudioStream_F32::transmit(block_f32, chan);
		AudioStream_F32::release(block_f32);
	}
	
	//shuffle the scaled data between the two buffers that the isr() routines looks for
	__disable_irq();
	if (block_1st == NULL) {
		block_1st = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		ch_offset = 0;
		__enable_irq();
	} else if (block_2nd == NULL) {
		block_2nd = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		__enable_irq();
	} else {
		audio_block_f32_t *tmp = block_1st;
		block_1st = block_2nd;
		block_2nd = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		ch_offset = 0;
		__enable_irq();
		AudioStream_F32::release(tmp);  //here we are releaseing an older audio buffer used as working memory
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
	update_1chan(0, block_ch1_1st, block_ch1_2nd, ch1_offset);
	update_1chan(1, block_ch2_1st, block_ch2_2nd, ch2_offset);
	update_1chan(2, block_ch3_1st, block_ch3_2nd, ch3_offset);
	update_1chan(3, block_ch4_1st, block_ch4_2nd, ch4_offset);
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