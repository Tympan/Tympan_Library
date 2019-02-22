
/*
 * AudioSynthToneSweep_F32
 * 
 * Created: Chip Audette, February 2019 (based on code from Teensy Audio Library)
 * Purpose: Synthesize audio with a swept freuqency
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




/******************************************************************/

//                A u d i o S y n t h T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014


boolean AudioSynthToneSweep_F32::play(float t_amp,float t_lo,float t_hi,float t_time) {
	return play(t_amp, t_lo, t_hi, t_time, (float)AUDIO_SAMPLE_RATE_EXACT);
}

//fs_Hz is the sample rate
boolean AudioSynthToneSweep_F32::play(float t_amp,float t_lo,float t_hi,float t_time,float _fs_Hz)
{
  float tone_tmp;
  fs_Hz = _fs_Hz;
  
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
  if(t_lo < 1.0)return false;
  if(t_hi < 1.0)return false;
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



unsigned char AudioSynthToneSweep_F32::isPlaying(void)
{
  return(sweep_busy);
}


void AudioSynthToneSweep_F32::update(void)
{
  audio_block_f32_t *block;
  float *bp;
  int i;
  
  if(!sweep_busy)return;

  //          L E F T  C H A N N E L  O N L Y
  block = allocate_f32();
  if(block) {
	float two_pi = 2.0*PI;  //PI is in CMSIS DSP library for ARM
    bp = block->data;
    //uint32_t tmp  = tone_freq >> 32; 
    //uint64_t tone_tmp = (0x400000000000LL * (int)(tmp&0x7fffffff)) / (int) AUDIO_SAMPLE_RATE_EXACT;
	float tone_tmp = tone_freq / fs_Hz;
    // Generate the sweep
    for(i = 0;i < block->length;i++) {
      //*bp++ = (short)(( (short)(arm_sin_q31((uint32_t)((tone_phase >> 15)&0x7fffffff))>>16) *tone_amp) >> 15);
	  *bp++  = arm_sin_f32(tone_phase) * tone_amp;
	  
      tone_phase +=  tone_tmp;
	  //if(tone_phase & 0x800000000000LL)tone_phase &= 0x7fffffffffffLL;
	  if (tone_phase > two_pi) tone_phase -= two_pi;
      

      if(tone_sign > 0) {
        if(tone_freq > tone_hi) {
          sweep_busy = 0;
          break;
        }
        tone_freq += tone_incr;
      } else {
        if(tone_freq < tone_hi || tone_freq < tone_incr) {
          sweep_busy = 0;

          break;
        }
        tone_freq -= tone_incr;        
      }
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