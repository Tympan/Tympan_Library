/*
 * AudioRateInterpolator_F32
 * 
 * Created: Chip Audette (OpenAudio) Sept 2021
 * 
 * Purpose: Uses ARM CMSIS DSP functions to interpolate then filter
 *
 * MIT License.  Use at your own risk.  Have fun!
 * 
 */

#ifndef _AudioRateInterpolator_F32_h
#define _AudioRateInterpolator_F32_h

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "arm_math.h"
#include <math.h>

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define INTERP_FIR_F32_PASSTHRU ((const float32_t *) 1)   //if you sete coeff_p to this, update() will simply 
#define INTERP_FIR_MAX_COEFFS 200

class AudioRateInterpolator_F32 : public AudioStream_F32 {
	public:
		AudioRateInterpolator_F32(void) : AudioStream_F32(1,inputQueueArray) {}
		AudioRateInterpolator_F32(const AudioSettings_F32 &settings): AudioStream_F32(1,inputQueueArray) { 
			start_sample_rate_Hz = settings.sample_rate_Hz;
			start_audio_block_samples = settings.audio_block_samples;

			end_sample_rate_Hz = start_sample_rate_Hz;  // will get overwritten when the up-factor is specified
			end_audio_block_samples = start_audio_block_samples;  // will get overwritten when the up-factor is specified
		}
		~AudioRateInterpolator_F32() {
			if (local_coeff_p) delete[] local_coeff_p;
		}
		
		//dummy initialization (pass-thru)
		bool begin(void) { return begin(coeff_passthru, 1, 1, start_audio_block_samples); }
		
		//initialize to use linear interpolation
		bool begin_linearInterp(const uint8_t _up_fac) { return begin_linearInterp(_up_fac, start_audio_block_samples); }
		bool begin_linearInterp(const uint8_t _up_fac, const int _in_block_size); //primary use case 1
		
		//initilize the interpolation using a auto-generated higher-order interpolation filter
		bool begin(const uint16_t _n_coeffs, const uint8_t _up_fac) { return begin(_n_coeffs, _up_fac, start_audio_block_samples); }
		bool begin(const uint16_t _n_coeffs, const uint8_t _up_fac, const int _in_block_size); //primary use case 2
		
		//initialize the interpolation (lowpass) filter by giving it the filter coefficients
		bool begin(const float32_t *cp, const uint16_t _n_coeffs, const uint8_t _upsamp_fac) { return begin(cp, _n_coeffs, _upsamp_fac, start_audio_block_samples); } //assume that the block size is the maximum
		bool begin(const float32_t *cp, const uint16_t _n_coeffs, const uint8_t _upsamp_fac, const int in_block_size);   //or, you can provide it with the block size
		void end(void) {  coeff_p = NULL; enable(false); }
		
		void update(void);
		int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update(); returns zero if OK
		int processAudioBlock_linearInterp(audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update(); returns zero if OK

		bool enable(bool enable = true) { 
			if (enable == true) {
				if ((coeff_p != INTERP_FIR_F32_PASSTHRU) && (is_armed)) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				}
			}
			is_enabled = false;
			return get_is_enabled();
		}
		bool get_is_enabled(void) { return is_enabled; }

		//void setBlockDC(void) {}	//helper function that sets this up for a first-order HP filter at 20Hz
		
		float set_startSampleRate_Hz(float fs_Hz) { start_sample_rate_Hz = fs_Hz;  end_sample_rate_Hz = start_sample_rate_Hz * upsamp_fac; return start_sample_rate_Hz; }
		float get_startSampleRate_Hz(void) { return start_sample_rate_Hz; }
		float get_endSampleRate_Hz(void) { return end_sample_rate_Hz; }
		
		void printCoeff(void) { printCoeff(0,n_coeffs); }
		void printCoeff(int start_ind, int end_ind);
	
	protected:
		audio_block_f32_t *inputQueueArray[1];
		float start_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		int start_audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;
		float end_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		int end_audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;
		bool flag_useLinearInterp = true;  //this gets overridden depending upon how the user calls begin
		float32_t prev_sample = 0.0;  //only used for linear interpolation
		
		bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
		bool is_enabled = false; //do you want this filter to execute?
	
		// pointer to current coefficients or NULL or FIR_PASSTHRU
		const float32_t coeff_passthru[1] = {1.0f}; //if you do begin() with this, the FIR filter will actually execute and update() will transmit the same values that you put in
		float32_t *local_coeff_p;
		const float32_t *coeff_p;
		int n_coeffs = 1;
		int upsamp_fac = 1;
		int configured_block_size = 0;

		// ARM DSP Math library filter instance
		arm_fir_interpolate_instance_f32 interp_inst;
		const int fir_max_coeffs = INTERP_FIR_MAX_COEFFS;
		float32_t StateF32[AUDIO_BLOCK_SAMPLES + INTERP_FIR_MAX_COEFFS];
		
		static void generate_interp_coeffs(uint16_t numTaps, uint8_t up_factor, float32_t *pCoeffs_out);
	
};


#endif