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

#include "utility/BTNRH_iir_filterbank.h"

#ifndef fmove
#define fmove(x,y,n)    memmove(x,y,(n)*sizeof(float))
#endif
#ifndef fcopy
#define fcopy(x,y,n)    memcpy(x,y,(n)*sizeof(float))
#endif
#ifndef fzero
#define fzero(x,n)      memset(x,0,(n)*sizeof(float))
#endif


/*
iir_filterbank_basic : design a bank of IIR filters with a smooth transition between each
	b  = Output: filter "b" coefficients.  float[N_CHAN][N_IIR+1]
	a  = Output: filter "a" coefficients.  float[N_CHAN][N_IIR+1]
	cv = Input: cross-over frequencies (Hz) between filters.  float[N_CHAN-1]
	nc = Input: number of channels (number of filters)
	n_iir = Input: order of each IIR filter (1 <= N <= 8)
	sr = Input: Sample rate (Hz)
*/
int AudioConfigIIRFilterBank_F32::iir_filterbank_basic(float *b, float *a, float *cf, const int nc, const int n_iir, const float sr) {

	//Serial.print("AudioConfigIIRFilterBank: iir_filterbank: nw_orig = "); Serial.print(nw_orig);
	//Serial.print(", nw = "); Serial.println(nw);
	if (n_iir < 1) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be at least 1.");
		return -1;
	}
	if (n_iir > 8) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be <= 8.");
		return -1;
	}
	
	//allocate some memory
	float *z = (float *) calloc(nc*(n_iir*2), sizeof(float)); //z is complex, so needs to be twice as long
	float *p = (float *) calloc(nc*(n_iir*2), sizeof(float)); //p is complex, so needs to be twice as long
	float *g = (float *) calloc(nc, sizeof(float));

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
	  
	//change representation from pole-zero to transfer function
	zp2ba_fb(z,p,nz,nc,b,a);
	  
	//apply the gain to the b coefficients
	for (int Ichan = 0; Ichan < nc; Ichan++) {
		for (int Icoeff = 0; Icoeff < n_iir+1; Icoeff++) {
			b[Ichan*(nz+1)+Icoeff] *= g[Ichan];
		}
	}  
	
	return 0;
}  
	  
/*
  iir_filterbank : design a bank of IIR filters with a smooth transition between each.
    Also, it computes the best delay for each filter to time-align the inpulse responses.
	Also, it computes the best gain per band to equlibrate their responses, depsite overlaping coverage.

  Arguments:
	b  = Output: filter "b" coefficients.  float[N_CHAN][N_IIR+1]
	a  = Output: filter "a" coefficients.  float[N_CHAN][N_IIR+1]
	d  = Output: filter delay (samples) to align impulse respsones.  int[N_CHAN]
	cv = Input: cross-over frequencies (Hz) between filters.  float[N_CHAN-1]
	nc = Input: number of channels (number of filters)
	n_iir = Input: order of each IIR filter (1 <= N <= 8)
	sr = Input: Sample rate (Hz)
	td = Input: max time delay (mseconds) allowed for aligning the impulse response of the filters
*/
int AudioConfigIIRFilterBank_F32::iir_filterbank(float *b, float *a, int *d, float *cf, const int nc, const int n_iir, const float sr, const float td) {

	if (n_iir < 1) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be at least 1.");
		return -1;
	}
	if (n_iir > 8) {
		Serial.println("AudioConfigIIRFilterBank: iir_filterbank: *** ERROR *** filter order must be <= 8.");
		return -1;
	}
	
	//allocate some memory
	float *z = (float *) calloc(nc*n_iir*2,sizeof(float)); //z is complex, so needs to be twice as long
	float *p = (float *) calloc(nc*n_iir*2,sizeof(float)); //p i scomplex, so needs to be twice as long
	float *g = (float *) calloc(nc, sizeof(float));  // gain is real

	if (g==NULL) {
		Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: *** ERROR *** could not allocate memory.");
	}
	//Serial.print("AudioConfigIIRFilterBank_F32: iir_filterbank: allocated memory...FreeRAM(B) = ");
    //Serial.println(FreeRam());

	//design filterbank, zeros and poles and gains
	int nz = n_iir;  //the CHA code from BTNRH uses this notation
	iirfb_zp(z,   //filter zeros.  pointer to array float[64]
	  p, //filter poles.  pointer to array float[64]
	  g, //gain for each filter.  pointer to array float[8]
	  cf, //pointer to cutoff frequencies, float[7]
	  sr, //sample rate (Hz) 
	  nc, //number of channels (8) 
	  nz);  //filter order.  (4)

	#if 0
		//plot zeros and poles and gains
		Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: orig zero-pole");
		for (int Iband=0; Iband < nc; Iband++) {
			Serial.print("  : Band "); Serial.print(Iband); Serial.print(", Gain = "); Serial.println(g[Iband],8);
			Serial.print("    : Z: ");for (int i=0; i < 2*n_iir; i++) { Serial.print(z[(Iband*2*n_iir)+i],4); Serial.print(", ");}; Serial.println();
			Serial.print("    : P: ");for (int i=0; i < 2*n_iir; i++) { Serial.print(p[(Iband*2*n_iir)+i],4); Serial.print(", ");}; Serial.println();
		}
	#endif

	//adjust filter to time-align the peaks of the fiter response
	align_peak_fb(z, //filter zeros.  pointer to array float[64]
	  p, //filter poles.  pointer to array float[64]
	  g, //gain for each filter.  pointer to array float[8]
	  d, //delay for each filter (samples).  pointer to array int[8]
	  td,  //max time delay (msec?) for the filters?
	  sr, //sample rate (Hz)
	  nc,  //number of channels
	  nz);  //filter order (4)


	int ret_val = 0;
	if (0) {
		//adjust gain of each filter (for flat response even during crossover?)
		//WARNING: This operation takes a lot of RAM.  If ret_val is -1, it failed.
		ret_val = adjust_gain_fb(z,   //filter zeros.  pointer to array float[64]
		  p, //filter poles.  pointer to array float[64]
		  g, //gain for each filter.  pointer to array float[8] 
		  d, //delay for each filter (samples).  pointer to array int[8]
		  cf, //pointer to cutoff frequencies, float[7]
		  sr, //sample rate (Hz) 
		  nc, //number of channels (8) 
		  nz);  //filter order (4)
		if (ret_val < 0) {
			Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: *** ERROR *** failed (in adjust_gain_fb.");
		}
	}
	
	#if 0
		//plot zeros and poles and gains
		Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: prior to b-a conversion");
		for (int Iband=0; Iband < nc; Iband++) {
			Serial.print("  : Band "); Serial.print(Iband); Serial.print(", Gain = "); Serial.println(g[Iband],8);
			Serial.print("    : Z: ");for (int i=0; i < 2*n_iir; i++) { Serial.print(z[(Iband*2*n_iir)+i],4); Serial.print(", ");}; Serial.println();
			Serial.print("    : P: ");for (int i=0; i < 2*n_iir; i++) { Serial.print(p[(Iband*2*n_iir)+i],4); Serial.print(", ");}; Serial.println();
		}
	#endif
	
	//change representation from pole-zero to transfer function
	zp2ba_fb(z,p,nz,nc,b,a);
	
	//print the coeff for debugging
	//Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: ");
	//for (int Ichan = 0; Ichan < nc; Ichan++) {
	//	Serial.print("Band "); Serial.print(Ichan);Serial.print(", g = "); Serial.print(g[Ichan],5); Serial.print(", b"); 
	//	for (int Icoeff = 0; Icoeff < n_iir+1; Icoeff++) {
	//		Serial.print(", ");
	//		Serial.print(b[Ichan*(nz+1)+Icoeff],5);
	//	}
	//	Serial.println();
	//}

	  
	//apply the gain to the b coefficients
	for (int Ichan = 0; Ichan < nc; Ichan++) {
		for (int Icoeff = 0; Icoeff < n_iir+1; Icoeff++) {
			//if ((Ichan == 0) || (Ichan == nc-1)) { //really?  only do it for the first and last?  WEA 6/8/2020
				b[Ichan*(nz+1)+Icoeff] *= g[Ichan];
			//}
		}
	}

	//Serial.println("AudioConfigIIRFilterBank_F32: iir_filterbank: post g*b: ");
	//for (int Ichan = 0; Ichan < nc; Ichan++) {
	//	Serial.print("Band "); Serial.print(Ichan);Serial.print(", b"); 
	//	for (int Icoeff = 0; Icoeff < n_iir+1; Icoeff++) {
	//		Serial.print(", ");
	//		Serial.print(b[Ichan*(nz+1)+Icoeff],5);
	//	}
	//	Serial.println();
	//}
	
	//release the allocated memory
	free(g);
	free(p);
	free(z);

	return ret_val;
}
			
