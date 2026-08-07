/*
 * AudioRateDecimator_F32
 * 
 * Created: Chip Audette (OpenAudio) Sept 2021
 * 
 * Purpose: Uses ARM CMSIS DSP functions to filter and then decimate
 *
 * MIT License.  Use at your own risk.  Have fun!
 * 
 */

#ifndef _AudioRateDecimator_F32_h
#define _AudioRateDecimator_F32_h

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "arm_math.h"

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define DEC_FIR_F32_PASSTHRU ((const float32_t *) 1)   //if you sete coeff_p to this, update() will simply 
#define DEC_FIR_MAX_COEFFS 200

class AudioRateDecimator_F32 : public AudioStream_F32 {
	public:
		AudioRateDecimator_F32(void) : AudioStream_F32(1,inputQueueArray) {}
		AudioRateDecimator_F32(const AudioSettings_F32 &settings): AudioStream_F32(1,inputQueueArray) {
			start_sample_rate_Hz = settings.sample_rate_Hz; 
			end_sample_rate_Hz = start_sample_rate_Hz;  //for now.  this will get replaced in begin()
			start_audio_block_samples = settings.audio_block_samples;
		}
		virtual ~AudioRateDecimator_F32() {
			if (local_coeff_p) delete[] local_coeff_p;
		}

		//dummy initialization (pass-thru)
		bool begin(void) { return begin(coeff_passthru, 1, 1, start_audio_block_samples); }

		//initialize the decimator filter by letting it generate its own filter coefficients
		bool begin(const uint16_t _n_coeffs, const uint8_t _dec_fac) { return begin(_n_coeffs, _dec_fac, start_audio_block_samples); }
		bool begin(const uint16_t _n_coeffs, const uint8_t _dec_fac, const int _in_block_size);	  // primary use case!	

		//initialize the decimator filter by giving it the filter coefficients
		bool begin(const float32_t *cp, const uint16_t _n_coeffs, const uint8_t _dec_fac) { return begin(cp, _n_coeffs, _dec_fac, start_audio_block_samples); } //assume that the block size is the maximum
		bool begin(const float32_t *cp, const uint16_t _n_coeffs, const uint8_t _dec_fac, const int _in_block_size);   //or, you can provide it with the block size
		void end(void) {  coeff_p = NULL; enable(false); }
		
		void update(void);
		int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update(); returns zero if OK

		bool enable(bool enable = true) { 
			if (enable == true) {
				if ((coeff_p != DEC_FIR_F32_PASSTHRU) && (is_armed)) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				}
			}
			is_enabled = false;
			return get_is_enabled();
		}
		bool get_is_enabled(void) { return is_enabled; }

		//void setBlockDC(void) {}	//helper function that sets this up for a first-order HP filter at 20Hz
		
		float set_startSampleRate_Hz(float fs_Hz) { start_sample_rate_Hz = fs_Hz;  end_sample_rate_Hz = start_sample_rate_Hz / dec_fac; return start_sample_rate_Hz; }
		float get_startSampleRate_Hz(void) { return start_sample_rate_Hz; }
		float get_endSampleRate_Hz(void) { return end_sample_rate_Hz; }
		
		void printCoeff(void) { printCoeff(0, n_coeffs); }
		void printCoeff(int start_ind, int end_ind);
	
	protected:
		audio_block_f32_t *inputQueueArray[1];
		float start_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		int start_audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;
		float end_sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT ;
		//int end_audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;
		
		bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
		bool is_enabled = false; //do you want this filter to execute?
	
		// pointer to current coefficients or NULL or FIR_PASSTHRU
		const float32_t coeff_passthru[1] = {1.0f}; //if you do begin() with this, the FIR filter will actually execute and update() will transmit the same values that you put in
		float32_t *local_coeff_p;
		const float32_t *coeff_p;
		uint16_t n_coeffs = 1;
		uint32_t dec_fac = 1;
		int configured_block_size;

		// ARM DSP Math library filter instance
		arm_fir_decimate_instance_f32 decimate_inst;
		const int fir_max_coeffs = DEC_FIR_MAX_COEFFS;
		float32_t StateF32[AUDIO_BLOCK_SAMPLES + DEC_FIR_MAX_COEFFS];
		
		// function to create the pre-filter
		static void generate_decimation_coeffs(const uint16_t numTaps, const uint8_t decimation_factor, float32_t *pCoeffs_out);
	
};


#endif