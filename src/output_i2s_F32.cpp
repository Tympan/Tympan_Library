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
 
#include "output_i2s_F32.h"
#include <arm_math.h>

// Define the full TX buffer, big enough to support 32-bit or 16-bit transfers
#define NUM_CHAN_TRANSFER (2)         //this class is for stereo (2-channel)
#define MAX_BYTES_PER_SAMPLE (4)      //assume 32-bit transfers is the max supported by this class
#define LEN_I2S_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_I2S_BUFFER_ELEMENT)
DMAMEM __attribute__((aligned(32))) static I2S_BUFFER_TYPE i2s_default_tx_buffer[LEN_I2S_BUFFER];

#define LEFT (0)
#define RIGHT (1)

//taken from Teensy Audio utility/imxrt_hw.h and imxrt_hw.cpp...
#if defined(__IMXRT1062__)
	#ifndef imxr_hw_h_
	#define imxr_hw_h_
	#define IMXRT_CACHE_ENABLED 2 // 0=disabled, 1=WT, 2= WB
	#include <Arduino.h>
	#include <imxrt.h>
	PROGMEM
	void set_audioClock_tympan(int nfact, int32_t nmult, uint32_t ndiv, bool force = false) // sets PLL4
	{
		if (!force && (CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_ENABLE)) return;

		CCM_ANALOG_PLL_AUDIO = CCM_ANALOG_PLL_AUDIO_BYPASS | CCM_ANALOG_PLL_AUDIO_ENABLE
					 | CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2) // 2: 1/4; 1: 1/2; 0: 1/1
					 | CCM_ANALOG_PLL_AUDIO_DIV_SELECT(nfact);

		CCM_ANALOG_PLL_AUDIO_NUM   = nmult & CCM_ANALOG_PLL_AUDIO_NUM_MASK;
		CCM_ANALOG_PLL_AUDIO_DENOM = ndiv & CCM_ANALOG_PLL_AUDIO_DENOM_MASK;
		
		CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN;//Switch on PLL
		while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK)) {}; //Wait for pll-lock
		
		const int div_post_pll = 1; // other values: 2,4
		CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_DIV_MSB | CCM_ANALOG_MISC2_DIV_LSB);
		if(div_post_pll>1) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_LSB;
		if(div_post_pll>3) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_MSB;
		
		CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_BYPASS;//Disable Bypass
	}
	#endif
#else
	//No IMXRT
	#define IMXRT_CACHE_ENABLED 0
#endif 


////////////
//
// Changing the sample rate based on changing the I2S bus freuqency (just for Teensy 3.x??)
//
//Here's the function to change the sample rate of the system (via changing the clocking of the I2S bus)
//https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate?p=121365&viewfull=1#post121365
//
//And, a post on how to compute the frac and div portions?  I haven't checked the code presented in this post:
//https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate?p=188812&viewfull=1#post188812
//
//Finally, here is my own Matlab code for computing the mult and div values...(again, just for Teensy 3.x??)
/*

	%choose the sample rates that you are hoping to hit
	targ_fs_Hz =  [2000, 8000, 11025, 16000, 22050, 24000, 32000, 44100, floor(44117.64706) , ...
		48000, 2*41000, 88200, floor(44117.64706 * 2), (37000/256*662), 96000, 176400, floor(44117.64706 * 4), 192000];

	%choose the PLL (ie, clock speed) values
	F_PLL_all = [16e6 7236 96e6 120e6 144e6 180e6 192e6 216e6 240e6];  %choose the clock rate used for this calculation
	PLL_div = 256;
	all_out_str = {};
	for I_PLL=1:length(F_PLL_all)
		F_PLL=F_PLL_all(I_PLL);
		all_n=[];all_d=[];
		
		if (I_PLL == 1)
			out_str = ['#if (F_PLL==' num2str(F_PLL) ')'];
		else
			out_str = ['#elif (F_PLL==' num2str(F_PLL) ')'];
		end
		all_out_str{end+1}=out_str;
		
		out_str='  const tmclk clkArr[numfreqs]={';
		for Itarg=1:length(targ_fs_Hz)
			out_str(end+1)='{';
			if (0)
				[best_d,best_n]=rat((F_PLL/PLL_div)/targ_fs_Hz(Itarg));
			else
				best_n = 1; best_d = 1; best_err = 1e10;
				for n=1:255
					d = [1:4095];
					act_fs_Hz = F_PLL / PLL_div * n ./ d;
					[err,I] = min(abs(act_fs_Hz - targ_fs_Hz(Itarg)));
					if err < best_err
						best_n = n; best_d = d(I);
						best_err = err;
					end
				end
			end
			all_n(Itarg) = best_n;
			all_d(Itarg) = best_d;
			disp(['fs = ' num2str(targ_fs_Hz(Itarg)) ', n = ' num2str(best_n) ', d = ' num2str(best_d) ', true = ' num2str(F_PLL/PLL_div * best_n / best_d)])
			out_str=[out_str num2str(best_n) ', ' num2str(best_d) '}'];
			if Itarg < length(targ_fs_Hz)
				out_str=[out_str ', '];
			end
		end
		out_str=[out_str '};'];
		all_out_str{end+1}=out_str;
	end
	all_out_str{end+1} = '#endif';

	%print out the full code to past into output_i2s
	strvcat(all_out_str)

*/

//Only used for Teensy 3.x (ie, Tympan Rev A through Rev D)
//Taken from: https://forum.pjrc.com/index.php?threads/discussion-about-a-simple-way-to-change-the-sample-rate.38753/#post-121365
float AudioOutputI2S_F32::setI2SFreq_T3(const float freq_Hz) {  
#if defined(KINETISK)   //for Teensy 3.x only!
	int freq = (int)(freq_Hz+0.5);
	typedef struct {
		uint8_t mult;
		uint16_t div;
	} __attribute__((__packed__)) tmclk;

	const int numfreqs = 19; //	           0     1      2      3      4      5      6      7                 8       9     10     11                     12                    13                    14      15      16                     17      18
	const int samplefreqs[numfreqs] = { 2000, 8000, 11025, 16000, 22050, 24000, 32000, 44100, (int)44117.64706 , 48000, 82000, 88200, (int)(44117.64706 * 2), (int)(95676.767+0.5),  (int)(95679.69+0.5), 96000, 176400, (int)(44117.64706 * 4), 192000};	//0.5 added to floats to force rounding

	#if (F_PLL==16000000)
		const tmclk clkArr[numfreqs]={{4, 125}, {16, 125}, {148, 839}, {32, 125}, {145, 411}, {48, 125}, {64, 125}, {151, 214}, {12, 17}, {96, 125}, {164, 125}, {151, 107}, {24, 17}, {124, 81}, {124, 81}, {192, 125}, {127, 45}, {48, 17}, {255, 83}};
	#elif (F_PLL==7236)
		const tmclk clkArr[numfreqs]={{212, 3}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}, {255, 1}};
	#elif (F_PLL==96000000)
		const tmclk clkArr[numfreqs]={{2, 375}, {8, 375}, {74, 2517}, {16, 375}, {147, 2500}, {8, 125}, {32, 375}, {147, 1250}, {2, 17}, {16, 125}, {82, 375}, {147, 625}, {4, 17}, {211, 827}, {62, 243}, {32, 125}, {151, 321}, {8, 17}, {64, 125}};
	#elif (F_PLL==120000000)
		const tmclk clkArr[numfreqs]={{8, 1875}, {32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {32, 625}, {128, 1875}, {205, 2179}, {8, 85}, {64, 625}, {194, 1109}, {89, 473}, {16, 85}, {149, 730}, {119, 583}, {128, 625}, {178, 473}, {32, 85}, {145, 354}};
	#elif (F_PLL==144000000)
		const tmclk clkArr[numfreqs]={{4, 1125}, {16, 1125}, {49, 2500}, {32, 1125}, {49, 1250}, {16, 375}, {64, 1125}, {49, 625}, {4, 51}, {32, 375}, {164, 1125}, {98, 625}, {8, 51}, {240, 1411}, {157, 923}, {64, 375}, {196, 625}, {16, 51}, {128, 375}};
	#elif (F_PLL==180000000)
		const tmclk clkArr[numfreqs]={{9, 3164}, {46, 4043}, {49, 3125}, {73, 3208}, {98, 3125}, {64, 1875}, {183, 4021}, {196, 3125}, {241, 3841}, {128, 1875}, {87, 746}, {107, 853}, {32, 255}, {192, 1411}, {238, 1749}, {219, 1604}, {214, 853}, {64, 255}, {219, 802}};
	#elif (F_PLL==192000000)
		const tmclk clkArr[numfreqs]={{1, 375}, {4, 375}, {37, 2517}, {8, 375}, {74, 2517}, {4, 125}, {16, 375}, {147, 2500}, {1, 17}, {8, 125}, {41, 375}, {147, 1250}, {2, 17}, {180, 1411}, {31, 243}, {16, 125}, {147, 625}, {4, 17}, {32, 125}};
	#elif (F_PLL==216000000)
		const tmclk clkArr[numfreqs]={{8, 3375}, {32, 3375}, {49, 3750}, {64, 3375}, {49, 1875}, {32, 1125}, {128, 3375}, {98, 1875}, {8, 153}, {64, 1125}, {183, 1883}, {196, 1875}, {16, 153}, {160, 1411}, {248, 2187}, {128, 1125}, {226, 1081}, {32, 153}, {147, 646}};
	#elif (F_PLL==240000000)
		const tmclk clkArr[numfreqs]={{4, 1875}, {16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {16, 625}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {164, 1875}, {205, 2179}, {8, 85}, {144, 1411}, {119, 1166}, {64, 625}, {89, 473}, {16, 85}, {128, 625}};
	#endif

	for (int f = 0; f < numfreqs; f++) {
		if (freq == samplefreqs[f]) {
			while (I2S0_MCR & I2S_MCR_DUF) {};
			I2S0_MDR = I2S_MDR_FRACT((clkArr[f].mult - 1)) | I2S_MDR_DIVIDE((clkArr[f].div - 1));
			return (float)(F_PLL / 256 * clkArr[f].mult / clkArr[f].div);
		}
	}
#endif
  return 0.0f;
} 


void AudioOutputI2S_F32::begin(void)
{
	if (i2s_buffer_was_given_by_user == false) i2s_tx_buffer = i2s_default_tx_buffer;
	dma.begin(true); // Allocate the DMA channel first

	initPointers(); //set all block_1st[] and block_2nd[] to nullptr

#if defined(KINETISK)
	transferUsing32bit = false;  //is this class ready for 32-bit yet?  Aug 5, 2025, I don't think it is.  So, force to false for now.
	AudioOutputI2S_F32::config_i2s(transferUsing32bit, sample_rate_Hz);

	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	//dma.TCD->SLAST = -sizeof(i2s_tx_buffer);//orig from Teensy Audio Library 2020-10-31
	dma.TCD->SLAST = -get_i2sBufferToUse_bytes();
	dma.TCD->DADDR = (void *)((uint32_t)&I2S0_TDR0 + 2);
	dma.TCD->DOFF = 0;
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2; //orig from Teensy Audio Library 2020-10-31
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel.
	dma.TCD->DLASTSGA = 0;
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;//orig from Teensy Audio Library 2020-10-31
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //The 2 is for stereo pair...because we're getting a stereo pair per I2S channel. 
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	dma.enable();  //newer location of this line in Teensy Audio library

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	
#elif defined(__IMXRT1062__)
	transferUsing32bit = true;
	AudioOutputI2S_F32::config_i2s(transferUsing32bit, sample_rate_Hz);

	//Setup the pins that we'll be using for the stereo I2S transfer
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0

	// DMA
	//   Each minor loop copies one audio sample destined for the CODEC (either 1 left or 1 right)
	//   Major loop repeats for all samples in i2s_tx_buffer
	#define DMA_TCD_ATTR_SSIZE_2BYTES         DMA_TCD_ATTR_SSIZE(1)
	#define DMA_TCD_ATTR_SSIZE_4BYTES         DMA_TCD_ATTR_SSIZE(2)
	#define DMA_TCD_ATTR_DSIZE_2BYTES         DMA_TCD_ATTR_DSIZE(1)
	#define DMA_TCD_ATTR_DSIZE_4BYTES         DMA_TCD_ATTR_DSIZE(2)
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = (transferUsing32bit ? 4 : 2); //4 for 32-bit, 2 for 16-bit
	if (transferUsing32bit) {
		// For 32-bit samples:
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_4BYTES | DMA_TCD_ATTR_DSIZE_4BYTES;
	} else {
		// For 16-bit samples:
		dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE_2BYTES | DMA_TCD_ATTR_DSIZE_2BYTES;
	}

	//settings that are common between 32-bit and 16-bit transfers
	dma.TCD->NBYTES_MLNO = (transferUsing32bit ? 4 : 2);
	dma.TCD->SLAST = -get_i2sBufferToUse_bytes();
	dma.TCD->DOFF = 0;     // 0 for stereo (because we're only using 1 i2s channel?).  This is the separation between sequential TDR registers
	dma.TCD->CITER_ELINKNO = audio_block_samples * 2; //allows for variable audio block length (2 is for stereo pair)
	dma.TCD->DLASTSGA = 0; // 0 for stereo (because we're only using 1 i2s channel?)
	dma.TCD->BITER_ELINKNO = audio_block_samples * 2; //must be set equal to CITER_ELINKNO
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	
	if (transferUsing32bit) {
		dma.TCD->DADDR = &I2S1_TDR0;
	}	else {
		dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0 + 2);  // "+ 2" to shift to start of high 16-bits in 32-bit register
	}
	
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);
	dma.enable();  //newer location of this line in Teensy Audio library
	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
	I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
#endif
	update_responsibility = update_setup();
	dma.attachInterrupt(AudioOutputI2S_F32::isr);
	//dma.enable(); //original location of this line in older Tympan_Library
	
	enabled = 1;

}



#if defined(KINETISK) || defined(KINETISL)
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
#endif


void AudioOutputI2S_F32::config_i2s(void) {	config_i2s(false, AudioOutputI2S_F32::sample_rate_Hz); }
void AudioOutputI2S_F32::config_i2s(bool _transferUsing32bit) {	config_i2s(_transferUsing32bit, AudioOutputI2S_F32::sample_rate_Hz); }
void AudioOutputI2S_F32::config_i2s(float fs_Hz) { config_i2s(false, fs_Hz); }
void AudioOutputI2S_F32::config_i2s(bool _transferUsing32bit, float fs_Hz)
{
	// TODO:
	//Set the universal static flag "transferUsing32bit" from the AudioI2SBase class that is common to all I2S classes
	//transferUsing32bit = __transferUsing32bit;  //looking through the code here, I'm not sure if 32-bits is relevant to this method.  Hmm. 
	
	bool only_bclk = false;
	
#if defined(KINETISK) || defined(KINETISL)
	//Tympan Revs A-D will be here (based on Teensy 3.6, which is Kinetis K)
	
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if ((I2S0_TCSR & I2S_TCSR_TE) != 0 || (I2S0_RCSR & I2S_RCSR_RE) != 0)
	{
	  if (!only_bclk) // if previous transmitter/receiver only activated BCLK, activate the other clock pins now
	  {
	    CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	    CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
	  }
	  return ;
	}

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(1);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(1);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	// configure pin mux for 3 clock signals
	if (!only_bclk)
	{
	  CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	  CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
	}
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK

	// change the I2S frequencies to make the requested sample rate
	setI2SFreq_T3(fs_Hz);  //for T3.x only!


#elif defined(__IMXRT1062__)
	//Tympan Revs E-F will be here (based on Teensy 4.1, which is IMXRT1062)
	
	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if ((I2S1_TCSR & I2S_TCSR_TE) != 0 || (I2S1_RCSR & I2S_RCSR_RE) != 0)
	{
	  if (!only_bclk) // if previous transmitter/receiver only activated BCLK, activate the other clock pins now
	  {
	    CORE_PIN23_CONFIG = 3;  //1:MCLK
	    CORE_PIN20_CONFIG = 3;  //1:RX_SYNC (LRCLK)
	  }
	  return ;
	}
	
	//PLL:
	//int fs = AUDIO_SAMPLE_RATE_EXACT; //original from Teensy Audio Library
	int fs = fs_Hz;
	
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	// Requirements: n2 must be 64 or below.  And, maybe, (n1*n2) = multiple of 4
	//    By default n1 = 4, which enables sample rates down to 9888 Hz
	//    This code will try n1 = 4, 8, 16, etc...but only 4 and 8 seem to result in a working system.
	//    I don't know why n1 = 16 does not work (try asking for a sample rate of 4000 Hz)
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4.  nIf we were to stay at n1 = 4, system would not support sample rates below 9888 Hz
	int n2;
	while ((n2 = 1 + (24000000.f * 27.f) / (fs * 256.f * n1)) > 64) { //do not allow n2 to be greater than 64?
		n1 *= 2;   //increase N1 (by factors of 4) to ensure n2 is 64 or less.  
	}
	
	double C = ((double)fs * 256 * n1 * n2) / 24000000.f;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	//set_audioClock(c0, c1, c2);
	set_audioClock_tympan(c0, c1, c2);

	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07 for n1 = 4
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	// Select MCLK
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1
		& ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
		| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));

	if (!only_bclk)
	{
	  CORE_PIN23_CONFIG = 3;  //1:MCLK
	  CORE_PIN20_CONFIG = 3;  //1:RX_SYNC  (LRCLK)
	}
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK

	int rsync = 0;
	int tsync = 1;

	I2S1_TMR = 0;
	//I2S1_TCSR = (1<<25); //Reset
	I2S1_TCR1 = I2S_TCR1_RFW(1);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP // sync=0; tx is async;
		    | (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
	I2S1_TCR3 = I2S_TCR3_TCE;
	I2S1_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((32-1)) | I2S_TCR4_MF
		    | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
	I2S1_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));

	I2S1_RMR = 0;
	//I2S1_RCSR = (1<<25); //Reset
	I2S1_RCR1 = I2S_RCR1_RFW(1);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP  // sync=0; rx is async;
		    | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1));
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_RCR4_FRSZ((2-1)) | I2S_RCR4_SYWD((32-1)) | I2S_RCR4_MF
		    | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S1_RCR5 = I2S_RCR5_WNW((32-1)) | I2S_RCR5_W0W((32-1)) | I2S_RCR5_FBT((32-1));

#endif
}


