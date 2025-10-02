/*
 * AudioMathOther_F32
 * 
 * Created: Chip Audette, Open Audio, Sept 2025
 * Purpose: Contains a bunch of classes for math operations on audio blocks
 * Assumes floating-point data.
 *          
 * These classes generally process a single stream fo audio data (ie, they are mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioMathOther_F32_h
#define _AudioMathother_F32_h

#include <arm_math.h>
#include "AudioStream_F32.h"

// Absolute value of each audio sample
class AudioMathAbs_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  public:
    AudioMathAbs_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
	  AudioMathAbs_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {};
		virtual ~AudioMathAbs_F32(void) {};
	
    void update(void) override;
  
	protected:
    audio_block_f32_t *inputQueueArray_f32[1];
		
};


// Squares each audio sample
class AudioMathSquare_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  public:
    AudioMathSquare_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
	  AudioMathSquare_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {};
		virtual ~AudioMathSquare_F32(void) {};
	
    void update(void) override;
  
	protected:
    audio_block_f32_t *inputQueueArray_f32[1];
		
};

// Square-root each audio sample
class AudioMathSqrt_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  public:
    AudioMathSqrt_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
	  AudioMathSqrt_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {};
		virtual ~AudioMathSqrt_F32(void) {};
	
    void update(void) override;
  
	protected:
    audio_block_f32_t *inputQueueArray_f32[1];
		
};



#endif