#ifndef _FreqWeighting_IEC1672_h
#define _FreqWeighting_IEC1672_h
//These filter coefficients were written by the Matlab code "make_A_C_weighting_sos.m
 
#define A_WEIGHT 1
#define C_WEIGHT 3
 
class FreqWeighting_IEC1672 {
  public:
	FreqWeighting_IEC1672(void) {}; //empty constructor
	
	//include all of the coefficients
	#include "FreqWeighting_IEC1672_coeff.h"
	
	int get_N_sos_per_filter(int type) {
		int out_val = Aweight_N_SOS_PER_FILTER;
		switch (type) {
			case A_WEIGHT:
				out_val = Aweight_N_SOS_PER_FILTER;
				break;
			case C_WEIGHT:
				out_val = Cweight_N_SOS_PER_FILTER;
				break;	
		
		}
		return out_val;
	}
	virtual int findIndexForSampleRate(float targ_fs_Hz) {
		int index = 0;
		bool done = false;
		float32_t val;
		while ( (!done) & (index < (N_all_fs_Hz-1))) {
			val = 0.5*(all_fs_Hz[index] + all_fs_Hz[index+1]);
			if (targ_fs_Hz > val) {
				index++;
			} else {
				done = 1;
			}
		}
		return index;
	}
	float32_t* get_filter_matlab_sos(int type, float32_t targ_fs_Hz) {
		int ind = findIndexForSampleRate(targ_fs_Hz);
		
		float32_t* coeff = (all_A_matlab_sos[ind]);
		switch (type) {
			case A_WEIGHT:
				coeff = (all_A_matlab_sos[ind]);
				break;
			case C_WEIGHT:
				coeff = (all_C_matlab_sos[ind]);
				break;
		}
		return coeff;
	}
 
 
};
 
#endif
