//
// AudioEffectFreqComp_FD_F32
//
// This is a mix of the frequency shifter and the formant shifter.
//   As of Oct 24, 2020, it is only the formant shifting, however.
//   I'm working on it.
//
// The frequency remapping follows this relationship:
//   D = Destiation Frequeny (ie, the new frequency, Hz)
//   S = Source Frequency (ie, the original frequency, Hz)
//   T = The starting frequency for the lowering (Hz)
//   dF = The amount of frequency shifting (Hz)
//   R = This is the frequency shift ratio (values below 1.0 compress; above 1.0 expand.  THIS IS ALSO 1.0/CompRatio!!!)
//
//   // NO!!! // D = T - dF + (S - T) * R    or    S = T + (D - T + dF) / R
//   // YES!! // D = T + dF + (S - T) * R    or    S = T + (D - T - dF) / R
//
// Created: Chip Audette, Open Audio, October 2020 (updated June 2021)
//
// MIT License.  Use at your own risk.
//

#ifndef _AudioEffectFreqComp_FD_F32_h
#define _AudioEffectFreqComp_FD_F32_h

#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor


//Create an audio processing class to do the compression in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT/IFFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.
class AudioEffectFreqComp_FD_F32 : public AudioFreqDomainBase_FD_F32
{
  public:
    // constructor
    AudioEffectFreqComp_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};
    
    // get/set methods specific to this particular frequency-domain algorithm
    float setScaleFactor(float scale_fac) {
      if (scale_fac < 0.00001) scale_fac = 0.00001; //limit the minimum scale factor
      return shift_scale_fac = scale_fac;
    }
    float getScaleFactor(void) {  return shift_scale_fac; }
    
    float setStartFreq_Hz(float freq_Hz) {
      freq_Hz = max(0.0f, freq_Hz); //prevent negative start frequencies
      int N_FFT = myFFT.getNFFT();
      float Hz_per_bin = sample_rate_Hz / ((float)N_FFT);
      int start_bin = round(freq_Hz / Hz_per_bin);
      return start_freq_Hz = Hz_per_bin * (float)start_bin; 
    }
    float getStartFreq_Hz(void) { return start_freq_Hz; }
    
    float setShift_Hz(float freq_Hz) { //only allow setting to an amount equal to one FFT bin
      //Serial.print("AudioEffectFreqLowering: setShift_Hz: input = "); Serial.print(freq_Hz);
      int N_FFT = myFFT.getNFFT();
      float Hz_per_bin = sample_rate_Hz / ((float)N_FFT);
      int shift_bins = round(freq_Hz / Hz_per_bin);
      //Serial.print(", output = "); Serial.println(Hz_per_bin * (float)shift_bins);
      return shift_Hz = Hz_per_bin * (float)shift_bins;
    }
    float getShift_Hz(void) { return shift_Hz; };

    int setShift_bins(int shift_bins) {
      int N_FFT = myFFT.getNFFT();
      float Hz_per_bin = sample_rate_Hz / ((float)N_FFT);
      setShift_Hz(Hz_per_bin * (float)shift_bins);
      return getShift_bins();       
    }
    int getShift_bins(void) {
      int N_FFT = myFFT.getNFFT();
      float Hz_per_bin = sample_rate_Hz / ((float)N_FFT);  
      return round(getShift_Hz() / Hz_per_bin);    
    }
    float setFreqCompRatio(float val) {  //1.0 is no compression.  Values larger than 1.0 squeeze the frequency range (so, a scale factor less than 1.0).
      setScaleFactor(1.0f / val);
      return getFreqCompRatio(); 
    }
    float getFreqCompRatio(void) { return (1.0f/shift_scale_fac); }
    
    bool setShiftOnlyTheMagnitude(bool _val) { return shiftOnlyTheMagnitude = _val; }; //if true, it's a vocoder.  Otherwise, it's a frequency shifter.
    bool getShiftOnlyTheMagnitude(void) { return shiftOnlyTheMagnitude; }; //if true, it's a vocoder.  Otherwise, it's a frequency shifter.

    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 

    //here are additional methods where we are going to put some details of the processing
    virtual void processAudioFD_NL_vocode(float *audio, int N_FFT);
    //virtual void processAudioFD_pitchShift(float *audio, int N_FFT);

  private:
    //create some data members specific to our processing
    float shift_scale_fac = 1.0; //how much to shift formants (frequency multiplier).  1.0 is no shift
    float start_freq_Hz = 0.0f; //what (original) frequency to start the shifting?. 
    float shift_Hz = 0.0f;  //set to zero to shift all of the audio up or down
    bool shiftOnlyTheMagnitude = true;
};


void AudioEffectFreqComp_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT)
{

  //if (shiftOnlyTheMagnitude) {
    processAudioFD_NL_vocode(complex_2N_buffer, NFFT);
  //} else {
  //  processAudioFD_pitchShift(complex_2N_buffer, NFFT);
  //}

}
 

//shift the audio by vocoding, which is the shifting of the FFT amplitudes but leaving the FFT phases in place
void AudioEffectFreqComp_FD_F32::processAudioFD_NL_vocode(float32_t *comples_2N_buffer, int fftSize) { //define some variables
  int N_2 = fftSize / 2 + 1;
  float Hz_per_bin = sample_rate_Hz / fftSize;
  float inv_shift_scale_fac = 1.0f / shift_scale_fac;
  int source_ind; // neg_dest_ind;
  float dest_freq_Hz, source_freq_Hz, interp_fac;
  float new_mag, scale;
  float orig_mag[N_2];
 
  #if 0
    float max_source_Hz = 10000.0; //highest frequency to use as source data
    int max_source_ind = min(int(max_source_Hz / sample_rate_Hz * fftSize + 0.5),N_2);
  #else
    int max_source_ind = N_2;  //this line causes this feature to be defeated
  #endif
  
  //get the magnitude for each FFT bin and store somewhere safes
  arm_cmplx_mag_f32(complex_2N_buffer, orig_mag, N_2);

  //now, loop over each bin and compute the new magnitude based on shifting the formants
  for (int dest_ind = 1; dest_ind < N_2; dest_ind++) { //don't start at zero bin, keep it at its original
    
    //what is the source bin for the new magnitude for this current destination bin
    dest_freq_Hz = dest_ind * Hz_per_bin;  //convert from bin # to frequency
    source_freq_Hz = start_freq_Hz + (dest_freq_Hz - start_freq_Hz - shift_Hz) * inv_shift_scale_fac;  

    //is the source index high enough to be above the start frequency for the shifting
    if (source_freq_Hz >= start_freq_Hz) {
      
      //source_ind = (int)(source_ind_float+0.5);  //no interpolation but round to the neariest index
      //source_ind = min(max(source_ind,1),N_2-1);
      float source_ind_float = source_freq_Hz / Hz_per_bin;
      if (source_ind_float > 0.0) {
        source_ind = min(max(1, (int)source_ind_float), N_2 - 2); //Chip: why -2 and not -1?  Because later, for for the interpolation, we do a +1 and we want to stay within nyquist
        interp_fac = source_ind_float - (float)source_ind;
        interp_fac = max(0.0, interp_fac);  //this will be used in the interpolation in a few lines
    
        //what is the new magnitude...interpolate
        new_mag = 0.0; scale = 0.0;
        if (source_ind < max_source_ind) {
      
          //interpolate in the original magnitude vector to find the new magnitude that we want
          //new_mag=orig_mag[source_ind];  //the magnitude that we desire
          //scale = new_mag / orig_mag[dest_ind];//compute the scale factor
          new_mag = orig_mag[source_ind];
          new_mag += interp_fac * (orig_mag[source_ind] - orig_mag[source_ind + 1]);
          scale = new_mag / orig_mag[dest_ind];
      
          //apply scale factor
          complex_2N_buffer[2 * dest_ind] *= scale; //real
          complex_2N_buffer[2 * dest_ind + 1] *= scale; //imaginary
        
        } else {  //if the source index is off the top of the frequency space, what to do?
        
          #if 0
            //set to zero
            complex_2N_buffer[2 * dest_ind] = 0.0; //real
            complex_2N_buffer[2 * dest_ind + 1] = 0.0; //imaginary
          #else
            //leave the original audio in that bin unchanged
          #endif
        
        }
        
      } else { //if the source index is below zero, what to do?
        //should neve get here due to the earlier check on start_freq_Hz
        
        //set to zero
        //complex_2N_buffer[2 * dest_ind] = 0.0; //real
        //complex_2N_buffer[2 * dest_ind + 1] = 0.0; //imaginary
      
      }
    } else { //if the source index is not above the minimum required
      //leave the original audio in that bin unchanged
    }
  }
  //zero out the lowest bin
  complex_2N_buffer[0] = 0.0; //real
  complex_2N_buffer[1] = 0.0; //imaginary

}
#endif
