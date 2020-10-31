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

#ifndef synth_tonesweep_f32_h_
#define synth_tonesweep_f32_h_

#include <Arduino.h>
#include "AudioStream_F32.h"

//                A u d i o S y n t h T o n e S w e e p
// Written by Pete (El Supremo) Feb 2014

class AudioSynthToneSweep_F32 : public AudioStream_F32
{
public:
  AudioSynthToneSweep_F32(void) : AudioStream_F32(0,NULL), sweep_busy(0)
  { 
	fs_Hz = (float)AUDIO_SAMPLE_RATE_EXACT;
  }
  AudioSynthToneSweep_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0,NULL), sweep_busy(0)  
  { 
	fs_Hz = settings.sample_rate_Hz;
  }

  boolean play(float t_amp,float t_lo,float t_hi,float t_time);
  boolean play(float t_amp,float t_lo,float t_hi,float t_time, float fs_Hz);
  virtual void update(void);
  unsigned char isPlaying(void);
  float read(void) {
    __disable_irq();
    uint64_t freq = tone_freq;
    unsigned char busy = sweep_busy;
    __enable_irq();
    if (!busy) return 0.0f;
    return (float)(freq >> 32);
  }

private:
  float tone_amp;
  float tone_lo;
  float tone_hi;
  float tone_freq;
  float tone_phase;
  float tone_incr;
  int tone_sign;
  unsigned char sweep_busy;
  float fs_Hz = (float)AUDIO_SAMPLE_RATE_EXACT;
};

#endif