/*
 * iir_filterbank.cpp
 * 
 * Created: Chip Audette, Creare LLC, May 2020
 *   Primarly built upon CHAPRO "Generic Hearing Aid" from 
 *   Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro
 *   
 * License: MIT License.  Use at your own risk.
 * 
 */

#include "AudioConfigIIRFilterBank_F32.h"

#ifndef fmove
#define fmove(x,y,n)    memmove(x,y,(n)*sizeof(float))
#endif
#ifndef fcopy
#define fcopy(x,y,n)    memcpy(x,y,(n)*sizeof(float))
#endif
#ifndef fzero
#define fzero(x,n)      memset(x,0,(n)*sizeof(float))
#endif

#include "utility/BTNRH_iir_filterbank.h"

/*
  iir_filterbank : design a bank of IIR filters with a smooth transition between each
	b  = Output: filter "b" coefficients.  float[N_CHAN][N_IIR+1]
	a  = Output: filter "a" coefficients.  float[N_CHAN][N_IIR+1]
	d  = Output: filter delay (samples) to align impulse respsones.  int[N_CHAN]
	cv = Input: cross-over frequencies (Hz) between filters.  float[N_CHAN-1]
	nc = Input: number of channels (number of filters)
	n_iir = Input: order of each IIR filter (1 <= N <= 8)
	sr = Input: Sample rate (Hz)
*/
int AudioConfigIIRFilterBank_F32::iir_filterbank(float *b, float *a, int *d, float *cf, const int nc, const int n_iir, const float sr) {

	//Serial.print("AudioConfigIIRFilterBank: iir_filterbank: nw_orig = "); Serial.print(nw_orig);
	//Serial.print(", nw = "); Serial.println(nw);
	int n_iir=niir_orig;
	if (n_iir < 1) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be at least 1.");
		return -1;
	}
	if (n_iir > 8) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be <= 8.");
		return -1;
	}
	
	//allocate some memory
	float z[nc][n_iir+1];
	float p[nc][n_iir+1];
	float g[nc];

	//design filterbank, zeros and poles and gains
	//Serial.println("cha_iirfb_design: calling iirfb_zp...");delay(500);
	int nz = n_iir;  //the CHA code from BTNRH uses this notation
	iirfb_zp(z,   //filter zeros.  pointer to array float[64]
	  p, //filter poles.  pointer to array float[64]
	  g, //gain for each filter.  pointer to array float[8]
	  cf, //pointer to cutoff frequencies, float[7]
	  sr, //sample rate (Hz) 
	  nc, //number of channels (8) 
	  nz);  //filter order.  (4)


	//adjust filter to time-align the peaks of the fiter response
	//Serial.println("cha_iirfb_design: calling align_peak_fb...");delay(500);
	align_peak_fb(z, //filter zeros.  pointer to array float[64]
	  p, //filter poles.  pointer to array float[64]
	  g, //gain for each filter.  pointer to array float[8]
	  d, //delay for each filter (samples?).  pointer to array int[8]
	  td,  //max time delay (msec?) for the filters?
	  sr, //sample rate (Hz)
	  nc,  //number of channels
	  nz);  //filter order (4)


	//adjust gain of each filter (for flat response even during crossover?)
	//Serial.println("cha_iirfb_design: calling adjust_gain_fb...");delay(500);
	adjust_gain_fb(z,   //filter zeros.  pointer to array float[64]
	  p, //filter poles.  pointer to array float[64]
	  g, //gain for each filter.  pointer to array float[8] 
	  d, //delay for each filter (samples).  pointer to array int[8]
	  cf, //pointer to cutoff frequencies, float[7]
	  sr, //sample rate (Hz) 
	  nc, //number of channels (8) 
	  nz);  //filter order (4)
	  
	//change representation from pole-zero to transfer function
	int nb = nc;  //number of bands (bands and channels are synonymous)
	zp2ba_fb(z,p,nz,nc,b,a);
	  
	//apply the gain to the b coefficients
	for (int Ichan = 0; Ichan < nc; I++) {
		for (int Icoeff = 0; Icoeff < n_iir+1; Icoeff++) {
			b[Ichan*(nz+1)+Icoeff] *= g[Ichan];
		}
	}

	return 0;
}
			
