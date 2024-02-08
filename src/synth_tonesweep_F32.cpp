
/*
 * AudioSynthToneSweep_F32
 * 
 * Created: Chip Audette, February 2019 (based on code from Teensy Audio Library)
 * Purpose: Synthesize audio with a swept freuqency
 *
 * Additional Modifications:
 *    * Feb 2024: Added Exponential Frequency Sweep (Chip Audette, Open Audio)
 *          
 * Any code not covered by other license is covered by the MIT License.  Use at your own risk.
*/



/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Pete (El Supremo)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "synth_tonesweep_F32.h"
#include "arm_math.h"
#include "AudioEffectCompressor_F32.h" //for the log2f_approx (fast log2)



/******************************************************************/

//                A u d i o S y n t h T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014


boolean AudioSynthToneSweep_F32::play(float t_amp,float t_lo,float t_hi,float t_time) {

	float tone_tmp;
  
	if(0) {
	  Serial.print("AudioSynthToneSweep.begin(tone_amp = ");
	  Serial.print(t_amp);
	  Serial.print(", tone_lo = ");
	  Serial.print(t_lo);
	  Serial.print(", tone_hi = ");
	  Serial.print(t_hi);
	  Serial.print(", tone_time = ");
	  Serial.print(t_time,1);
	  Serial.println(")");
	}
	tone_amp = 0.f;
	if(t_amp < 0.0)return false;
	if(t_amp > 1.0)return false;
	if(t_lo < 1.e-6)return false;
	if(t_hi < 1.e-6)return false;
	if(t_hi >= (int) fs_Hz / 2.0)return false;
	if(t_lo >= (int) fs_Hz / 2.0)return false;
	if(t_time <= 0.0)return false;
	tone_lo = t_lo;
	tone_hi = t_hi;
	tone_phase = 0.0;

	//tone_amp = t_amp * 32767.0;
	tone_amp = t_amp;

	//tone_freq = tone_lo*0x100000000LL;
	tone_freq = tone_lo;
	if (tone_hi >= tone_lo) {
		tone_tmp = tone_hi - tone_lo;
		tone_sign = 1;
	} else {
		tone_sign = -1;
		tone_tmp = tone_lo - tone_hi;
	}
	tone_tmp = tone_tmp / t_time / fs_Hz;
	//tone_incr = (tone_tmp * 0x100000000LL);
	tone_incr = tone_tmp;
	sweep_busy = 1;
	return(true);
}

//fs_Hz is the sample rate
boolean AudioSynthToneSweep_F32::play(float t_amp,float t_lo,float t_hi,float t_time,float _fs_Hz)
{
  fs_Hz = _fs_Hz;
  return play(t_amp, t_lo, t_hi, t_time);
  
}



unsigned char AudioSynthToneSweep_F32::isPlaying(void)
{
  return(sweep_busy);
}


void AudioSynthToneSweep_F32::update_silence(void) {
	audio_block_f32_t *block;
	float *bp;
	int i;
	
	block = allocate_f32();
	if(block) {
		bp = block->data;
		for(i = 0;i < block->length;i++) bp[i] = 0.0f;
		
		AudioStream_F32::transmit(block);
		AudioStream_F32::release(block);
	}	
}

void AudioSynthToneSweep_F32::update(void)
{
  
  if(!sweep_busy) {
	  update_silence();
	  return;
  }

  audio_block_f32_t *block;
  float *bp;
  int i;

  //          L E F T  C H A N N E L  O N L Y
  block = allocate_f32();
  if(block) {
	float two_pi = 2.0*PI;  //PI is in CMSIS DSP library for ARM
    bp = block->data;
    //uint32_t tmp  = tone_freq >> 32; 
    //uint64_t tone_tmp = (0x400000000000LL * (int)(tmp&0x7fffffff)) / (int) AUDIO_SAMPLE_RATE_EXACT;
	//float tone_tmp = tone_freq / fs_Hz;
	const float two_pi_div_fs_Hz = two_pi / fs_Hz;
    // Generate the sweep
    for(i = 0;i < block->length;i++) {
      //*bp++ = (short)(( (short)(arm_sin_q31((uint32_t)((tone_phase >> 15)&0x7fffffff))>>16) *tone_amp) >> 15);
	  *bp++  = arm_sin_f32(tone_phase) * tone_amp;
	  
      //tone_phase +=  tone_tmp;
	  //if(tone_phase & 0x800000000000LL)tone_phase &= 0x7fffffffffffLL;

      if(tone_sign > 0) {
        if(tone_freq > tone_hi) {
          sweep_busy = 0;
          break;
        }
        tone_freq += tone_incr;
		tone_phase += (tone_freq * two_pi_div_fs_Hz);
      } else {
        if(tone_freq < tone_hi || tone_freq < tone_incr) {
          sweep_busy = 0;

          break;
        }
        tone_freq -= tone_incr;  
		tone_phase += (tone_freq * two_pi_div_fs_Hz);		
      }
	  if (tone_phase > two_pi) tone_phase -= two_pi;
    }
    while(i < block->length) {
      *bp++ = 0;
      i++;
    }    
    // send the samples to the left channel
    //transmit(block,0);
    //release(block);
	AudioStream_F32::transmit(block);
	AudioStream_F32::release(block);	
  }
}

// /////////////////////////////////////////////////////////////////////////////////

//t_amp is the amplitude of the tone, 0.0->1.0
//f_start is the starting frequency of the chirp in Hz
//f_end is the ending frequecy of the chirp in Hz
//t_time is the duration of he chirp in seconds 
boolean AudioSynthToneSweepExp_F32::play(float t_amp,float f_start_Hz, float f_end_Hz,float t_time) {

	// Check the inputs to make sure that they're OK
	if(0) {
	  Serial.print("AudioSynthToneSweepExp_F32.begin(tone_amp = ");
	  Serial.print(t_amp);
	  Serial.print(", f_start_Hz = ");
	  Serial.print(f_start_Hz);
	  Serial.print(", f_end_Hz = ");
	  Serial.print(f_end_Hz);
	  Serial.print(", tone_time = ");
	  Serial.print(t_time,1);
	  Serial.println(")");
	}
	tone_amp = 0.f;
	if(t_amp < 0.0)return false;
	if(t_amp > 1.0)return false;
	if(f_start_Hz < 1.e-4)return false;
	if(f_end_Hz < 1.e-4)return false;
	if(f_start_Hz >= (int) fs_Hz / 2.0)return false;
	if(f_end_Hz >= (int) fs_Hz / 2.0)return false;
	if(t_time <= 0.0)return false;

	// save the parameters
	tone_amp = t_amp;
	tone_lo = f_start_Hz;
	tone_hi = f_end_Hz;
	
	// compute the increment factor
	float foo = powf(tone_hi / tone_lo,1.0/t_time);
	float samp_time_sec = 1.0/fs_Hz;
	tone_incr = logf(foo) * samp_time_sec;

	// initialize the current state
	tone_freq = tone_lo;  //here is the intial (ie, current) frequency
	log_tone_freq = logf(tone_freq);
	tone_phase = 0.0;
	sweep_busy = 1;
	
	return(true);
}


//fs_Hz is the sample rate
boolean AudioSynthToneSweepExp_F32::play(float t_amp,float t_lo,float t_hi,float t_time,float _fs_Hz)
{
  fs_Hz = _fs_Hz;
  return AudioSynthToneSweepExp_F32::play(t_amp, t_lo, t_hi, t_time);

}

void AudioSynthToneSweepExp_F32::update(void)
{
  if(!sweep_busy) {
	  update_silence();
	  return;
  }

  audio_block_f32_t *block;
  float *bp;
  int i;
  block = allocate_f32();
  if(block) {
	float two_pi = 2.0f*PI;  //PI is in CMSIS DSP library for ARM
    bp = block->data;
	const float two_pi_div_fs_Hz = two_pi / fs_Hz;
    
	// Generate the sweep
    for(i = 0;i < block->length;i++) {
		//compute the current audio sample
		*bp++  = arm_sin_f32(tone_phase) * tone_amp;
	  
		//are we done with computing the waveform?
        if(tone_freq > tone_hi) {
          sweep_busy = 0;  //yes, we're done
          break;  //break out of the for-loop
        }
		
		//otherwise, let's increment the phase
		tone_phase += (tone_freq * two_pi_div_fs_Hz);
		while (tone_phase > two_pi) tone_phase -= two_pi;
		
		//and increment the frequency
		log_tone_freq += tone_incr;
		tone_freq = expf(log_tone_freq); //here is the computational expensive operation
    }
	
	//in case we bailed out early (because we were done), fill out the audio block with zeros
    while(i < block->length) {
      *bp++ = 0;
      i++;
    }    
	
    // send the samples to the left channel
	AudioStream_F32::transmit(block);
	AudioStream_F32::release(block);	
  }
}