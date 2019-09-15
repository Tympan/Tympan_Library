
/*
 * AudioEffectFreqShiftFD_F32
 * 
 * Created: Chip Audette, Aug 2019
 * Purpose: Shift the frequency content of the audio up or down.  Performed in the frequency domain
 *          
 * This processes a single stream of audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioEffectFreqShiftFD_F32_h
#define _AudioEffectFreqShiftFD_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "FFT_Overlapped_F32.h"
#include <Arduino.h>


class AudioEffectFreqShiftFD_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName:freq_shift
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectFreqShiftFD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectFreqShiftFD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectFreqShiftFD_F32(const AudioSettings_F32 &settings) :
      AudioStream_F32(1, inputQueueArray_f32) {
      sample_rate_Hz = settings.sample_rate_Hz;
    }
    AudioEffectFreqShiftFD_F32(const AudioSettings_F32 &settings, const int _N_FFT) :
      AudioStream_F32(1, inputQueueArray_f32) {
      setup(settings, _N_FFT);
    }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectFreqShiftFD_F32(void) {
      if (complex_2N_buffer != NULL) delete complex_2N_buffer;
    }

    int setup(const AudioSettings_F32 &settings, const int _N_FFT) {
      sample_rate_Hz = settings.sample_rate_Hz;
      int N_FFT;

      //setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
      N_FFT = myFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
      if (N_FFT < 1) return N_FFT;
      N_FFT = myIFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
      if (N_FFT < 1) return N_FFT;

      //decide windowing
      //Serial.println("AudioEffectFreqShiftFD_F32: setting myFFT to use hanning...");
      (myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
      #if 1
        if (myIFFT.getNBuffBlocks() > 3) {
          Serial.println("AudioEffectFormantShiftFD_F32: setting myIFFT to use hanning...");
          (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
        }
      #endif

	  #if 0
      //print info about setup
      Serial.println("AudioEffectFreqShiftFD_F32: FFT parameters...");
      Serial.print("    : N_FFT = "); Serial.println(N_FFT);
      Serial.print("    : audio_block_samples = "); Serial.println(settings.audio_block_samples);
      Serial.print("    : FFT N_BUFF_BLOCKS = "); Serial.println(myFFT.getNBuffBlocks());
      Serial.print("    : IFFT N_BUFF_BLOCKS = "); Serial.println(myIFFT.getNBuffBlocks());
      Serial.print("    : FFT use window = "); Serial.println(myFFT.getFFTObject()->get_flagUseWindow());
      Serial.print("    : IFFT use window = "); Serial.println((myIFFT.getIFFTObject())->get_flagUseWindow());
	  #endif
	  
      //allocate memory to hold frequency domain data
      complex_2N_buffer = new float32_t[2 * N_FFT];

      //we're done.  return!
      enabled = 1;
      return N_FFT;
    }

    int setShift_bins(int _shift_bins) {
      return shift_bins = _shift_bins;
    }
    int getShift_bins(void) {
      return shift_bins;
    }

    virtual void update(void);

  private:
    int enabled = 0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;

    int shift_bins = 0; //how much to shift the frequency
};


#endif