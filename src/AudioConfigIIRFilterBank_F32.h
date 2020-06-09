/*
 * iir_filterbank.h
 * 
 * Created: Chip Audette, Creare LLC, May 2020
 *   Primarly built upon CHAPRO "Generic Hearing Aid" from 
 *   Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro
 *   
 * License: MIT License.  Use at your own risk.
 * 
*/

#ifndef AudioConfigIIRFilterBank_F32_h
#define AudioConfigIIRFilterBank_F32_h

#include <Tympan_Library.h>


class AudioConfigIIRFilterBank_F32 {
  //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node  
  //GUI: shortName:config_FIRbank
  public:
    AudioConfigIIRFilterBank_F32(void) {}
	AudioConfigIIRFilterBank_F32(const AudioSettings_F32 &settings) {}
    AudioConfigIIRFilterBank_F32(const int n_chan, const int n_iir, const float sample_rate_Hz, float *crossover_freq, float *filter_bcoeff, float *filter_acoeff)
	{
      createFilterCoeff(n_chan, n_iir, sample_rate_Hz, crossover_freq, filter_bcoeff, filter_acoeff);
    }
    AudioConfigIIRFilterBank_F32(const int n_chan, const int n_iir, const float sample_rate_Hz, const float td, float *crossover_freq, float *filter_bcoeff, float *filter_acoeff, int *filter_delay)
	{
      createFilterCoeff(n_chan, n_iir, sample_rate_Hz, td, crossover_freq, filter_bcoeff, filter_acoeff, filter_delay);
    }

    //createFilterCoeff:
    //   Purpose: create all of the IIR filter coefficients for the IIR filterbank
    //   Syntax: createFilterCoeff(n_chan, n_fir, sample_rate_Hz, crossover_freq, filter_bcoeff, filter_acoeff)
    //      int n_chan (input): number of channels (number of filters) you desire.  Must be 2 or greater
    //      int n_iir (input): order of the IIR filter (should probably be 4 or less...but you can try higher)
    //      float sample_rate_Hz (input): sample rate of your system (used to scale the crossover_freq values)
    //      float *crossover_freq (input): array of frequencies (Hz) seperating each band in your filter bank.
    //          should contain n_chan-1 values because it should exclude the bottom (0 Hz) and the top
    //          (Nyquist) as those values are already assumed by this routine. An valid example is below:
    //          int n_chan = 8;  float cf[] = {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7}; 
    //      float *filter_bcoeff (output): array of "b" IIR filter coefficients (TF numerator, like Matlab) that
    //           are computed by this routine.  You must have pre-allocated the array such as: 
	//           float filter_bcoeff[N_CHAN][N_IIR+1];
    //      float *filter_acoeff (output): array of "a" IIR filter coefficients (TF denomenator, like Matlab) that
    //           are computed by this routine.  You must have pre-allocated the array such as: 
	//           float filter_acoeff[N_CHAN][N_IIR+1];
	int createFilterCoeff(const int n_chan, const int n_iir, const float sample_rate_Hz, float *crossover_freq, float *filter_bcoeff, float *filter_acoeff) {
		int filter_delay[n_chan]; //samples
		float td_msec = 0.000;  //assumed max delay (?) for the time-alignment process?
		return createFilterCoeff(n_chan, n_iir, sample_rate_Hz, td_msec, crossover_freq, filter_bcoeff, filter_acoeff, filter_delay);
	}
		
	//createFilterCoeff: same as above but adds an output "filter_delay" that tells you how many samples to delay
	//   each filter so that the impulse response lines up better (at the cross-over frequencies?)
	//       "td_msec" is an input setting the max time delay of all filters (?) in milliseconds
	//       "filter_delay" is the output with the best time delay (samples) for each filter.  It is ]
    int createFilterCoeff(const int n_chan, const int n_iir, const float sample_rate_Hz, const float td_msec, float *crossover_freq, float *filter_bcoeff, float *filter_acoeff, int *filter_delay) {
      float *cf = crossover_freq;
      int flag__free_cf = 0;
      if (cf == NULL) {
        //compute corner frequencies that are logarithmically spaced
        cf = (float *) calloc(n_chan, sizeof(float));
        flag__free_cf = 1;
        computeLogSpacedCornerFreqs(n_chan, sample_rate_Hz, cf);
      }

	  //Serial.println("AudioConfigIIRFilterBank_F32: createFilterCoeff: calling iir_filterbank...");
      int ret_val = iir_filterbank(filter_bcoeff, filter_acoeff, filter_delay, cf, n_chan, n_iir, sample_rate_Hz, td_msec);
      if (flag__free_cf) free(cf); 
	  return ret_val;
    }

    int createFilterCoeff_SOS(const int n_chan, const int n_iir, const float sample_rate_Hz, const float td_msec, float *crossover_freq, float *filter_sos, int *filter_delay) {
      float *cf = crossover_freq;
      int flag__free_cf = 0;
      if (cf == NULL) {
        //compute corner frequencies that are logarithmically spaced
        cf = (float *) calloc(n_chan, sizeof(float));
        flag__free_cf = 1;
        computeLogSpacedCornerFreqs(n_chan, sample_rate_Hz, cf);
      }

	  //Serial.println("AudioConfigIIRFilterBank_F32: createFilterCoeff: calling iir_filterbank...");
      int ret_val = iir_filterbank_sos(filter_sos, filter_delay, cf, n_chan, n_iir, sample_rate_Hz, td_msec);
      if (flag__free_cf) free(cf); 
	  return ret_val;
    }

    //compute frequencies that space zero to nyquist.  Leave zero off, because it is assumed to exist in the later code.
    //example of an *8* channel set of frequencies: cf = {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7}
    void computeLogSpacedCornerFreqs(const int n_chan, const float sample_rate_Hz, float *cf) {
      float cf_8_band[] = {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7};
      float scale_fac = expf(logf(cf_8_band[6]/cf_8_band[0]) / ((float)(n_chan-2)));
      //Serial.print("MakeFIRFilterBank: computeEvenlySpacedCornerFreqs: scale_fac = "); Serial.println(scale_fac);
      cf[0] = cf_8_band[0];
      //Serial.println("MakeIIRFilterBank: computeEvenlySpacedCornerFreqs: cf = ");Serial.print(cf[0]); Serial.print(", ");
      for (int i=1; i < n_chan-1; i++) {
        cf[i] = cf[i-1]*scale_fac;
        //Serial.print(cf[i]); Serial.print(", ");
      }
      //Serial.println();
    }
  private:

	int iir_filterbank_basic(float *bb,  float *aa, float *cf, const int nc, const int n_iir, const float sr); //no time alignment, no gain balancing
    int iir_filterbank(      float *bb,  float *aa, int *d,    float *cf,    const int nc,    const int n_iir, const float sr, const float td);
	int iir_filterbank_sos(  float *sos, int *d,    float *cf, const int nc, const int n_iir, const float sr,  const float td);
};
#endif
