/*
 * AudioFilterIIR_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 *    - Building from AudioFilterFIR from Teensy Audio Library (AudioFilterFIR credited to Pete (El Supremo))
 * 
 */

#ifndef _filter_iir_f32_h
#define _filter_iir_f32_h

#include "Arduino.h"
#include "AudioStream_F32.h"
//include "arm_math.h"

#define IRR_MAX_ORDER 8
#define IIR_MAX_N_COEFF (IRR_MAX_ORDER+1)

class AudioFilterIIR_F32 : public AudioStream_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node  
//GUI: shortName:filter_IIR
	public:
		AudioFilterIIR_F32(void): AudioStream_F32(1,inputQueueArray) { initCoefficientsToPassthru();  }
		AudioFilterIIR_F32(const AudioSettings_F32 &settings): AudioStream_F32(1,inputQueueArray) { initCoefficientsToPassthru(); }
		
		//initialize the coefficients to simply pass the audio through unaffected
		void initCoefficientsToPassthru(void) {
			for (int i=0; i<IIR_MAX_N_COEFF; i++) {
				b_coeff[i] = 0.0;
				a_coeff[i] = 0.0;
				filter_states[i]=0.0;
			}
			n_coeff = 1;
			b_coeff[0] = 1.0;
			a_coeff[0] = 1.0;
			
			is_armed = true;

		}
		void disable(void) {  n_coeff=0; enable(false); is_armed = false;}
			
		//initialize this IIR filter by giving it the filter coefficients (which are copied locally)
		int begin(void) { initCoefficientsToPassthru();	return 0; }
		int begin(const float32_t *_b_coeff, const float32_t *_a_coeff, const int _n_coeff) { 
			if ((n_coeff < 0) | (n_coeff > IIR_MAX_N_COEFF)) {
				enable(false);
				return -1;
			}
			if (n_coeff == 0) { 
				initCoefficientsToPassthru(); 
				enable(true);
				return 0;
			} 
			
			copyCoeff(_b_coeff, b_coeff, _n_coeff);
			copyCoeff(_a_coeff, a_coeff, _n_coeff);
			n_coeff = _n_coeff;
			resetFilterStates();
			is_armed=true;
			enable(true);
			return 0;
		}
		void end(void) {  initCoefficientsToPassthru(); disable(); }
		void update(void);
		int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update();  returns 0 if OK
		void resetFilterStates(void) { for (int i=0; i<IIR_MAX_N_COEFF; i++) filter_states[i]=0.0; }

		bool enable(bool enable = true) { 
			if (enable == true) {
				if (is_armed) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				}
			}
			is_enabled = false;
			return get_is_enabled();
		}
		bool get_is_enabled(void) { return is_enabled; }

		//void setBlockDC(void) {}	//helper function that sets this up for a first-order HP filter at 20Hz
		
	private:
		audio_block_f32_t *inputQueueArray[1];
		bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
		bool is_enabled = false; //do you want this filter to execute?
		
		int copyCoeff(const float *in, float *out, int n_coeff) {
			if ((n_coeff > 0) & (n_coeff <= IIR_MAX_N_COEFF)) {
				for (int i=0; i < n_coeff; i++) {
					out[i]=in[i];
				}
			} else {
				return -1;
			}
			return 0;
		}

		// pointer to current coefficients or NULL or FIR_PASSTHRU
		float32_t b_coeff[IIR_MAX_N_COEFF],a_coeff[IIR_MAX_N_COEFF];
		float32_t filter_states[IIR_MAX_N_COEFF];
		int n_coeff=0;

};


#endif


