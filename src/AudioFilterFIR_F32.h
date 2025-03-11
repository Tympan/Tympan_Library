/*
 * AudioFilterFIR_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 *    - Building from AudioFilterFIR from Teensy Audio Library (AudioFilterFIR credited to Pete (El Supremo))
 * 
 */

#ifndef _filter_fir_f32_h
#define _filter_fir_f32_h

#include <Arduino.h>
#include "AudioStream_F32.h"
#include "AudioFilterBiquad_F32.h" //for AudioFilterBase_F32
#include "arm_math.h"

// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define FIR_F32_PASSTHRU ((const float32_t *) 1)   //if you sete coeff_p to this, update() will simply 
#define FIR_MAX_COEFFS 200

class AudioFilterFIR_F32 : public AudioFilterBase_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node  
//GUI: shortName:filter_FIR
	public:
		AudioFilterFIR_F32(void): AudioFilterBase_F32(), 
			coeff_p(FIR_F32_PASSTHRU), n_coeffs(1), configured_block_size(0) {	}
		AudioFilterFIR_F32(const AudioSettings_F32 &settings): AudioFilterBase_F32(settings),
			coeff_p(FIR_F32_PASSTHRU), n_coeffs(1), configured_block_size(0) {	}
			
		//initialize the FIR filter by giving it the filter coefficients
		bool begin(void) { return begin(coeff_passthru, 1, AUDIO_BLOCK_SAMPLES); }
		bool begin(const float32_t *cp, const int _n_coeffs) { return begin(cp, _n_coeffs, AUDIO_BLOCK_SAMPLES); } //assume that the block size is the maximum
		bool begin(const float32_t *cp, const int _n_coeffs, const int block_size);   //or, you can provide it with the block size
		void end(void) {  coeff_p = NULL; enable(false); }
		void update(void);
		int processAudioBlock(const audio_block_f32_t *block, audio_block_f32_t *block_new); //called by update(); returns zero if OK

 		bool enable(bool enable = true) { 
			if (enable == true) {
				if ((coeff_p != FIR_F32_PASSTHRU) && (is_armed)) {  //don't allow it to enable if it can't actually run the filters
					is_enabled = enable;
					return get_is_enabled();
				}
			}
			is_enabled = false;
			return get_is_enabled();
		}
		//bool get_is_enabled(void) { return is_enabled; }

		//void setBlockDC(void) {}	//helper function that sets this up for a first-order HP filter at 20Hz
		
		void printCoeff(int start_ind, int end_ind);
		
	protected:
		audio_block_f32_t *inputQueueArray[1];
		bool is_armed = false;   //has the ARM_MATH filter class been initialized ever?
		//bool is_enabled = false; //do you want this filter to execute?

		// pointer to current coefficients or NULL or FIR_PASSTHRU
		const float32_t coeff_passthru[1] = {1.0f}; //if you do begin() with this, the FIR filter will actually execute and update() will transmit the same values that you put in
		const float32_t *coeff_p;
		int n_coeffs;
		int configured_block_size;

		// ARM DSP Math library filter instance
		arm_fir_instance_f32 fir_inst;
		float32_t StateF32[AUDIO_BLOCK_SAMPLES + FIR_MAX_COEFFS];
};


#endif


