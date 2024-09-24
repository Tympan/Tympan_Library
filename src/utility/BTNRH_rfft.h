
#ifndef _BTNRH_FFT_H
#define _BTNRH_FFT_H

#include <math.h>
//#include "chapro.h"
//#include "cha_ff.h"

/***********************************************************/
// FFT functions adapted from G. D. Bergland, "Subroutines FAST and FSST," (1979).
// In IEEE Acoustics, Speech, and Signal Processing Society.
// "Programs for Digital Signal Processing," IEEE Press, New York,
//
// From BTNRH chapro library: https://github.com/BoysTownOrg/chapro
// See docs: https://github.com/BoysTownorg/chapro/blob/master/chapro.pdf

namespace BTNRH_FFT {
	
	/* 
	void cha_fft_rc(float *x, int n): 
		Fourier transform real signal into complex frequency components.
		Function arguments
			x Real-valued signal is replaced by complex frequency components
			n Number of points in the signal.
		Return Value
			None
		Remarks
			The input array must be dimensioned to accommodate n+2 float values. The number of
			complex frequency components is (n+2)/2.
	*/
	void cha_fft_cr(float *, int);  //FFT
	
	
	/*
	void cha_fft_cr(float *x, int n)
		Inverse Fourier transform complex frequency components into real signal.
		Function arguments
			x Complex frequency components are replaced by real-valued signal.
			n Number of points in the signal.
		Return Value
			None
		Remarks
			The input array must be dimensioned to accommodate n+2 float values. The number of
			complex frequency components is (n+2)/2.
	*/
	void cha_fft_rc(float *, int);  //Inverse FFT
}

#endif