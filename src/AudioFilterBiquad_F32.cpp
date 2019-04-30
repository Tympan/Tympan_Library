
/*
 * AudioFilterBiquad_F32.cpp
 *
 * Chip Audette, OpenAudio, Apr 2017
 *
 * MIT License,  Use at your own risk.
 *
*/


#include "AudioFilterBiquad_F32.h"

void AudioFilterBiquad_F32::update(void)
{
  audio_block_f32_t *block;

  block = AudioStream_F32::receiveWritable_f32();
  if (!block) return;

  // If there's no coefficient table, give up.  
  if (coeff_p == NULL) {
    AudioStream_F32::release(block);
    return;
  }

  // do passthru
  if (coeff_p == IIR_F32_PASSTHRU) {
    // Just passthrough
    AudioStream_F32::transmit(block);
    AudioStream_F32::release(block);
    return;
  }

  // do IIR
  arm_biquad_cascade_df1_f32(&iir_inst, block->data, block->data, block->length);
  
  //transmit the data
  AudioStream_F32::transmit(block); // send the IIR output
  AudioStream_F32::release(block);
}


//for all of these filters, a butterworth filter has q = 0.7017  (ie, 1/sqrt(2))
void AudioFilterBiquad_F32::calcLowpass(float32_t freq_Hz, float32_t q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	
	//int coeff[5];
	double w0 = freq_Hz * (2.0 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	//double scale = 1073741824.0 / (1.0 + alpha);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = ((1.0 - cosW0) / 2.0) * scale;
	/* b1 */ coeff[1] = (1.0 - cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;
	
}
void AudioFilterBiquad_F32::calcHighpass(float32_t freq_Hz, float32_t q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = ((1.0 + cosW0) / 2.0) * scale;
	/* b1 */ coeff[1] = -(1.0 + cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;

}
void AudioFilterBiquad_F32::calcBandpass(float32_t freq_Hz, float32_t q, float32_t *coeff) {
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = alpha * scale;
	/* b1 */ coeff[1] = 0;
	/* b2 */ coeff[2] = (-alpha) * scale;
	/* a0 = 1.0 in Matlab style */
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;

}
void AudioFilterBiquad_F32::calcNotch(float32_t freq_Hz, float32_t q, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
	
	//int coeff[5];
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	double alpha = sinW0 / ((double)q * 2.0);
	double cosW0 = cos(w0);
	double scale = 1.0 / (1.0+alpha); // which is equal to 1.0 / a0
	/* b0 */ coeff[0] = scale;
	/* b1 */ coeff[1] = (-2.0 * cosW0) * scale;
	/* b2 */ coeff[2] = coeff[0];
	/* a1 */ coeff[3] = (-2.0 * cosW0) * scale;
	/* a2 */ coeff[4] = (1.0 - alpha) * scale;

}
	
void AudioFilterBiquad_F32::calcLowShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
			
	//int coeff[5];
	double a = pow(10.0, gain/40.0);
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
	double cosW0 = cos(w0);
	//generate three helper-values (intermediate results):
	double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/slope-1.0)+2.0*a );
	double aMinus = (a-1.0)*cosW0;
	double aPlus = (a+1.0)*cosW0;
	double scale = 1.0 / ( (a+1.0) + aMinus + sinsq);
	/* b0 */ coeff[0] =		a *	( (a+1.0) - aMinus + sinsq	) * scale;
	/* b1 */ coeff[1] =  2.0*a * ( (a-1.0) - aPlus  			) * scale;
	/* b2 */ coeff[2] =		a * ( (a+1.0) - aMinus - sinsq 	) * scale;
	/* a1 */ coeff[3] = -2.0*	( (a-1.0) + aPlus			) * scale;
	/* a2 */ coeff[4] =  		( (a+1.0) + aMinus - sinsq	) * scale;

}

void AudioFilterBiquad_F32::calcHighShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *coeff) {
	cutoff_Hz = freq_Hz;
			
	//int coeff[5];
	double a = pow(10.0, gain/40.0);
	double w0 = freq_Hz * (2 * 3.141592654 / sampleRate_Hz);
	double sinW0 = sin(w0);
	//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
	double cosW0 = cos(w0);
	//generate three helper-values (intermediate results):
	double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/slope-1.0)+2.0*a );
	double aMinus = (a-1.0)*cosW0;
	double aPlus = (a+1.0)*cosW0;
	double scale = 1.0 / ( (a+1.0) - aMinus + sinsq);
	/* b0 */ coeff[0] =		a *	( (a+1.0) + aMinus + sinsq	) * scale;
	/* b1 */ coeff[1] = -2.0*a * ( (a-1.0) + aPlus  			) * scale;
	/* b2 */ coeff[2] =		a * ( (a+1.0) + aMinus - sinsq 	) * scale;
	/* a1 */ coeff[3] =  2.0*	( (a-1.0) - aPlus			) * scale;
	/* a2 */ coeff[4] =  		( (a+1.0) - aMinus - sinsq	) * scale;
}