
#ifndef _AudioFilterFreqWeighting_F32_h
#define _AudioFilterFreqWeighting_F32_h

#include "utility\FreqWeighting_IEC1672.h"

//Frequency weighting.  Defaults to A-Weighting
class AudioFilterFreqWeighting_F32: public AudioFilterBiquad_F32 {
	public:
		AudioFilterFreqWeighting_F32(void): AudioFilterBiquad_F32() {
			selectFilterCoeff();
		}		
		
		AudioFilterFreqWeighting_F32(const AudioSettings_F32 &settings):
			AudioFilterBiquad_F32(settings) { 
			selectFilterCoeff(); 
		}

		virtual void setSampleRate_Hz(float _fs_Hz) { 
			AudioFilterBiquad_F32::setSampleRate_Hz(_fs_Hz); 
			selectFilterCoeff();
		}
		virtual void setWeightingType(int type) {
			weightingType = type;
			selectFilterCoeff();
		}
		virtual int getWeightingType(void) { return weightingType; }
	
	protected:
		int weightingType = A_WEIGHT;
		FreqWeighting_IEC1672 IEC_coefficients;
		
		virtual void selectFilterCoeff(void) {
			int N_sos = IEC_coefficients.get_N_sos_per_filter(getWeightingType());
			float32_t *sos_matlab_coeff = IEC_coefficients.get_filter_matlab_sos(getWeightingType(),getSampleRate_Hz());
			setFilterCoeff_Matlab_sos(sos_matlab_coeff, N_sos); //calling AudioFilterBiquad_F32 method
		}
	
};

#endif
