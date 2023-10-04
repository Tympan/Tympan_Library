
#ifndef _AudioLoopBack_F32_h
#define _AudioLoopBack_F32_h

#include "AudioStream_F32.h"    //from Tympan_Library
#include "AudioSettings_F32.h"  //from Tympan_Library

// here's an interface to be used by AFC (or other classes) to allow audio to be handed
// to them by the AFC loopback class without the AFC loopback class needing to be rewritten
// for every variant of the AFC classes.  Instead, one's AFC class can inherit this interface
// the existing loopback class can target it. 
class AudioLoopBackInterface_F32 {
  public:
    virtual void receiveLoopBackAudio(audio_block_f32_t *in_block)=0;
};


class AudioLoopBack_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:0  //this line used for automatic generation of GUI node
//GUI: shortName: LoopBack
  public:
    //constructor
    AudioLoopBack_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { }
    AudioLoopBack_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { };

    void setTarget(AudioLoopBackInterface_F32 *_afc) {
      targ_obj = _afc;
    }

    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
      if (targ_obj == NULL) return;

      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;

      //do the work
      targ_obj->receiveLoopBackAudio(in_block);

      //release memory
      AudioStream_F32::release(in_block);
    }

  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    AudioLoopBackInterface_F32 *targ_obj;
};

#endif


