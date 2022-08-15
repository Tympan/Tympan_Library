/*
   AudioConfigFIRFilter_F32

   Created: Chip Audette, OpenAudio, Sept 2021
     Primarly built upon CHAPRO "Generic Hearing Aid" from
     Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

   License: MIT License.  Use at your own risk.

*/

#ifndef _AudioConfigFIRFilter_F32_h
#define _AudioConfigFIRFilter_F32_h

#include <Arduino.h>


class AudioConfigFIRFilter_F32 {
    //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
    //GUI: shortName:config_FIR
  public:
    AudioConfigFIRFilter_F32(void) {}
    AudioConfigFIRFilter_F32(const AudioSettings_F32 &settings) {}

    int designLowpass(int n_fir, float freq_Hz, float sample_rate_Hz, float *filter_coeff);
    int designBandpass(int n_fir, float freq1_Hz, float freq2_Hz,  float sample_rate_Hz, float *filter_coeff);
    int designBandstop(int n_fir, float freq1_Hz, float freq2_Hz, float sample_rate_Hz, float *filter_coeff);
    int designHighpass(int n_fir, float freq_Hz, float sample_rate_Hz, float *filter_coeff);
    int designPassOrStop(int is_bandpass, int n_fir, float freq1_Hz, float freq2_Hz, float sample_rate_Hz, float *filter_coeff);

    void createWindow(int window_type, int n_window, float *window_coeff) {
      createWindow(window_type, n_window, n_window, window_coeff);
    }
    void createWindow(int window_type, int n_window_orig, int n_window, float *window_coeff);
    
  protected:

    //used for nextPowerOfTwo
    const int n_out_vals = 10;
    int out_vals[10] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    int nextPowerOfTwo(int n) {
      if (n < out_vals[0]) return out_vals[0];
      for (int i = 1; i < n_out_vals; i++) {
        if ((n > out_vals[i - 1]) & (n <= out_vals[i])) {
          return out_vals[i];
        }
      }
      return n;
    }
};

#ifndef fmove
#define fmove(x,y,n)    memmove(x,y,(n)*sizeof(float))
#endif
#ifndef fcopy
#define fcopy(x,y,n)    memcpy(x,y,(n)*sizeof(float))
#endif
#ifndef fzero
#define fzero(x,n)      memset(x,0,(n)*sizeof(float))
#endif

#include "utility/BTNRH_rfft.h"

inline int AudioConfigFIRFilter_F32::designLowpass(int n_fir, float freq_Hz, float sample_rate_Hz, float *filter_coeff) {
  float freq1_Hz = 0.0;     //low edge of bandpass
  float freq2_Hz = freq_Hz; //high edge of bandpass
  return designBandpass(n_fir, freq1_Hz, freq2_Hz, sample_rate_Hz, filter_coeff);
}
inline int AudioConfigFIRFilter_F32::designHighpass(int n_fir, float freq_Hz, float sample_rate_Hz, float *filter_coeff) {
  float freq1_Hz = freq_Hz;            //low edge of bandpass
  float freq2_Hz = sample_rate_Hz/2.0; //high edge of bandpass
  return designBandpass(n_fir, freq1_Hz, freq2_Hz, sample_rate_Hz, filter_coeff);
}
inline int AudioConfigFIRFilter_F32::designBandpass(int n_fir, float freq1_Hz, float freq2_Hz, float sample_rate_Hz, float *filter_coeff) {
  int is_bandpass = 1; //one for bandpass
  return designPassOrStop(is_bandpass, n_fir, freq1_Hz, freq2_Hz, sample_rate_Hz, filter_coeff);
}
inline int AudioConfigFIRFilter_F32::designBandstop(int n_fir, float freq1_Hz, float freq2_Hz, float sample_rate_Hz, float *filter_coeff) {
  int is_bandpass = 0; //zero for bandstop
  return designPassOrStop(is_bandpass, n_fir, freq1_Hz, freq2_Hz, sample_rate_Hz, filter_coeff);
}

//wt = window type.  0 = Hamming, 1 = blackman, else = Hanning
//nw_orig = length of the window
//nw = number of coefficients (could be larger than nw_orig
//ww = array of window coefficients
inline void AudioConfigFIRFilter_F32::createWindow(int wt, int nw_orig, int nw, float *ww) {
  //float sm = 0.0;
  float w, p;
  float a = 0.16f;  //for blackman window
  
  for (int j = 0; j < nw; j++) ww[j] = 0.0f; //clear
  for (int j = 0; j < nw_orig; j++) {  //create window
    p = M_PI * (2.0 * j - nw_orig) / nw_orig;
    if (wt == 0) {
      w = 0.54 + 0.46 * cos(p);                   // Hamming
    } else if (wt == 1) {
      w = (1.0 - a + cos(p) + a * cos(2.0 * p)) / 2.0;  // Blackman
    } else {
      //win = (1 - cos(2*pi*[1:N]/(N+1)))/2;  //WEA's matlab call, indexing starts from 1, not zero
      w = (1.0 - cosf(2.0 * M_PI * ((float)(j)) / ((float)(nw_orig - 1)))) / 2.0;
    }
    //sm += w;
    ww[j] = (float) w;
  }  
}

inline int AudioConfigFIRFilter_F32::designPassOrStop(int is_bandpass, int n_fir, float freq1_Hz, float freq2_Hz, float sample_rate_Hz, float *filter_coeff) {

  //remap the names to match BTNRH example
  int nw_orig = n_fir;       //use BTNRH name
  float sr = sample_rate_Hz; //use BTNRH name
  const int wt = 0;          //window type (BTNRH name): 0 = Hamming, 1=Blackmann, 2 = Hanning

  //make sure that the requested n_fir fits within what we know how to do
  if ((nw_orig < 1) || (nw_orig > out_vals[n_out_vals - 1])) return -1;

  int      nt, nf, ns;
  int      bp_inds[2];
  int      nw = nextPowerOfTwo(nw_orig);
  
  //Serial.print("AudioConfigFIRFilter_F32: designLowpass: nw_orig = "); Serial.print(nw_orig);
  //Serial.print(", nw = "); Serial.println(nw);

  //we're going to design this FIR filter using an FFT

  nt = nw * 2;   //we're going to do an fft that's twice as long (zero padded)
  nf = nt/2 + 1; //number of bins to nyquist in the zero-padded FFT.
  ns = nf * 2;   //hold complex data

  float ww[nw];
  float xx[ns];
  float yy[ns];
  
  //initilize (clear) the arrays
  fzero(xx, ns); fzero(yy, ns);
  
  // create windowing function
  createWindow(wt, nw_orig, nw, ww);

  // define the frequency bounds (in FFT indices) that we care about
  bp_inds[0] = max(0,min(nf, round(nf * freq1_Hz * (2.0 / sr))));
  bp_inds[1] = max(0,min(nf, round(nf * freq2_Hz * (2.0 / sr))));

  // begin building the desired frequency response
  xx[nw_orig / 2] = 1.0; //make a single-sample impulse centered on our eventual window
  BTNRH_FFT::cha_fft_rc(xx, nt); //convert to frequency domain

  //check for special cases of the bandstop
  if (is_bandpass == 0) { //look for if it is a bandstop
    if ((bp_inds[0]==0) && (bp_inds[1] < nf)) {
      //if the bandstop starts at 0, this is actually a highpass
      bp_inds[0] = bp_inds[1]; 
      bp_inds[1] = nf;
      is_bandpass = 1; //it's not a bandstop anymore, it's a bandpass (as a highpass)
    } else if ((bp_inds[0]>0) && (bp_inds[1] == nf)) {
      //if bandpass ends at nyquist, this is actually a lowpass
      bp_inds[1] = bp_inds[0];
      bp_inds[0] = 0;
      is_bandpass = 1;//it's not a bandstop anymore, it's a bandpass (as a lowpass)
    }
  }
  

  // create the desired response from the spectrum by copying just the desired regions of the impulse
  // int nbins = (be[k + 1] - be[k]) * 2;  Serial.print("fir_filterbank: chan ");Serial.print(k); Serial.print(", nbins = ");Serial.println(nbins);
  if (is_bandpass==1) {
    //copy the values between the two limits
    fcopy(yy + bp_inds[0] * 2, xx + bp_inds[0] * 2, (bp_inds[1] - bp_inds[0]) * 2); //copy just our passband
  } else {
    //copy the low coefficients
    fcopy(yy + bp_inds[1] * 2, xx + bp_inds[1] * 2, (nf - bp_inds[1]) * 2); //copy the low end below the bandstop
    
    //copy the high coefficients
    fcopy(yy + bp_inds[1] * 2, xx + bp_inds[1] * 2, (nf - bp_inds[1]) * 2); //copy the high end above the bandstop
  }
  
  // convert back to time domain
  BTNRH_FFT::cha_fft_cr(yy, nt); //IFFT back into the time domain

  // apply window to iFFT of bandpass
  for (int j = 0; j < nw; j++) yy[j] *= ww[j];
  
  //copy the filter coefficients to the output array
  fcopy(filter_coeff, yy, nw_orig);

  return 0;  //OK
}

#endif
