
#ifndef _AudioEffectLowpass_FD_F32_h
#define _AudioEffectLowpass_FD_F32_h

#include "AudioStream_F32.h"
#include <arm_math.h>
#include "FFT_Overlapped_F32.h"

class AudioEffectLowpass_FD_F32 : public AudioStream_F32
{
  public:
    //constructors...a few different options.  The usual one should be: AudioEffectLowpassFD_F32(const AudioSettings_F32 &settings, const int _N_FFT)
    AudioEffectLowpass_FD_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {};
    AudioEffectLowpass_FD_F32(const AudioSettings_F32 &settings) :
        AudioStream_F32(1, inputQueueArray_f32) {  sample_rate_Hz = settings.sample_rate_Hz;   }
    AudioEffectLowpass_FD_F32(const AudioSettings_F32 &settings, const int _N_FFT) : 
        AudioStream_F32(1, inputQueueArray_f32) { setup(settings,_N_FFT);  }

    //destructor...release all of the memory that has been allocated
    ~AudioEffectLowpass_FD_F32(void) {  if (complex_2N_buffer != NULL) delete complex_2N_buffer;  }
       
    int setup(const AudioSettings_F32 &settings, const int _N_FFT);
    void update(void); //this is what gets called by the Tympan (Teensy) audio subsystem
    void processAudioFD(float32_t *complex_data, const int nfft);   //This is where our algorithm lives
    
    void setCutoff_Hz(float freq_Hz) { cutoff_Hz = freq_Hz;  }
    float getCutoff_Hz(void) {   return cutoff_Hz; }
    
  private:
    int enabled=0;
    float32_t *complex_2N_buffer;
    audio_block_f32_t *inputQueueArray_f32[1];
    FFT_Overlapped_F32 myFFT;
    IFFT_Overlapped_F32 myIFFT;
    float cutoff_Hz = 1000.f;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE;
    
};

int AudioEffectLowpass_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {
  sample_rate_Hz = settings.sample_rate_Hz;
  int N_FFT;
  
  //setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
  N_FFT = myFFT.setup(settings,_N_FFT); //hopefully, we got the same N_FFT that we asked for
  if (N_FFT < 1) return N_FFT;
  N_FFT = myIFFT.setup(settings,_N_FFT);  //hopefully, we got the same N_FFT that we asked for
  if (N_FFT < 1) return N_FFT;

  //decide windowing
  Serial.println("AudioEffectLowpassFD_F32: setting myFFT to use hanning...");
  (myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
  if (myIFFT.getNBuffBlocks() > 3) {
    Serial.println("AudioEffectLowpassFD_F32: setting myIFFT to use hanning...");
    (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
  }

  //print info about setup
  Serial.println("AudioEffectLowpassFD_F32: FFT parameters...");
  Serial.print("    : N_FFT = "); Serial.println(N_FFT);
  Serial.print("    : audio_block_samples = "); Serial.println(settings.audio_block_samples);
  Serial.print("    : FFT N_BUFF_BLOCKS = "); Serial.println(myFFT.getNBuffBlocks());
  Serial.print("    : IFFT N_BUFF_BLOCKS = "); Serial.println(myIFFT.getNBuffBlocks());
  Serial.print("    : FFT use window = "); Serial.println(myFFT.getFFTObject()->get_flagUseWindow());
  Serial.print("    : IFFT use window = "); Serial.println((myIFFT.getIFFTObject())->get_flagUseWindow());

  //allocate memory to hold frequency domain data
  complex_2N_buffer = new float32_t[2*N_FFT];

  //we're done.  return!
  enabled=1;
  return N_FFT;
}

void AudioEffectLowpass_FD_F32::update(void)
{
  //get a pointer to the latest data
  audio_block_f32_t *in_audio_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_audio_block) return;

  //simply return the audio if this class hasn't been enabled
  if (!enabled) { AudioStream_F32::transmit(in_audio_block); AudioStream_F32::release(in_audio_block); return; }

  //convert to frequency domain
  myFFT.execute(in_audio_block, complex_2N_buffer);
  AudioStream_F32::release(in_audio_block);  //We just passed ownership to myFFT, so release it here.
  
  // ////////////// Do your processing here!!!

  //here's where all of our lowpass filtering happens!
  processAudioFD(complex_2N_buffer,myFFT.getNFFT()); 

  //now, rebuild the frequency space above nyquist
  myFFT.rebuildNegativeFrequencySpace(complex_2N_buffer);

  // ///////////// End do your processing here

  //call the IFFT
  audio_block_f32_t *out_audio_block = myIFFT.execute(complex_2N_buffer); //out_block is pre-allocated in here.
 
  //send the returned audio block.  Don't issue the release command here because myIFFT will re-use it
  AudioStream_F32::transmit(out_audio_block); //don't release this buffer because myIFFT re-uses it within its own code
  return;
};

// Here is the function that actually does the lowpass filtering.
// It loops through the frequency bins and attenuates those bins that are above the cutoff frequency.
void AudioEffectLowpass_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {
  int nyquist_bin = NFFT/2 + 1;
  float bin_width_Hz = sample_rate_Hz / ((float)NFFT);
  
  int cutoff_bin = (int)(cutoff_Hz / bin_width_Hz + 0.5f); //the 0.5 is so that it rounds instead of truncates
  if (cutoff_bin >= nyquist_bin) return; 

  //start at the cutoff bin and loop upward, attenuating each one
  for (int i=cutoff_bin; i < nyquist_bin; i++) { //only do positive frequency space...will rebuild the neg freq space later
    //attenuate by 30 dB
    complex_2N_buffer[2*i]   = 0.03f * complex_2N_buffer[2*i];   //real
    complex_2N_buffer[2*i+1] = 0.03f * complex_2N_buffer[2*i+1]; //imaginary
  }
}

#endif
