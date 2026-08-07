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


bool AudioRateInterpolator_F32::begin_linearInterp(const uint8_t _upsamp_fac, const int in_block_size) {  //or, you can provide it with the block size
	coeff_p = coeff_passthru;
	n_coeffs = 1;
	upsamp_fac = _upsamp_fac;

	is_enabled = true;
	flag_useLinearInterp = true;
	
	start_audio_block_samples = in_block_size;
	end_audio_block_samples = upsamp_fac * start_audio_block_samples;
	end_sample_rate_Hz = start_sample_rate_Hz * upsamp_fac;

	return get_is_enabled();
}

/**
 * @brief Generates FIR interpolator low-pass filter coefficients using a windowed sinc method.
 * @param numTaps     Total number of filter coefficients (Must be a multiple of L_factor)
 * @param L_factor    The interpolation factor
 * @param pCoeffs_out Pointer to the output buffer where coefficients will be stored (Size: numTaps)
 */
void AudioRateInterpolator_F32::generate_interp_coeffs(uint16_t numTaps, uint8_t up_factor, float32_t *pCoeffs_out) {
	//check to see if we've been given the trival case
	if (numTaps == 1) { pCoeffs_out[0] = 1.0;	return; }  //return early

	// 1. Calculate the anti-aliasing cutoff frequency (normalized to fs = 1.0)
	const float32_t f_c = 0.5f / static_cast<float32_t>(up_factor);  //or should we use 0.45?
	const float32_t center = static_cast<float32_t>(numTaps - 1) / 2.0f;


	// 2. Compute Windowed Sinc Coefficients
	constexpr float32_t pi = static_cast<float32_t>(PI);  // PI should be in arm_math.h
	for (uint16_t n = 0; n < numTaps; n++) {
			const float32_t n_offset = (float32_t)n - center;
			float32_t sinc_val;

			// Handle the division-by-zero at the center tap
			if (fabsf(n_offset) < 1e-5f) {
					sinc_val = 2.0f * f_c;
			} else {
					sinc_val = sinf(2.0f * pi * f_c * n_offset) / (pi * n_offset);
			}

			// Apply Hamming Window to minimize spectral leakage (Gibbs phenomenon)
			float32_t window_val = 0.54f - 0.46f * cosf(2.0f * pi * (float32_t)n / (float32_t)(numTaps - 1));
			
			pCoeffs_out[n] = sinc_val * window_val;
	}
	
	// replace with simpler filter
	//warning "FOR DEBUGGING: forcing a dumb averaging filter. Remove this!"
	//for (uint16_t n = 0; n < numTaps; n++) { pCoeffs_out[n] = 1.0f; }

	// 3. Normalize coefficients to achieve unity DC gain (sum of all taps = 1.0)
	float32_t sum = 0.0f;
	for (uint16_t n = 0; n < numTaps; n++) { sum += pCoeffs_out[n]; }
	for (uint16_t n = 0; n < numTaps; n++) { pCoeffs_out[n] /= sum;	}
	
}



bool AudioRateInterpolator_F32::begin(const uint16_t _n_coeffs, const uint8_t _up_fac, const int _in_block_size) 
{
	// clear out the old filter coefficients (if present)
	enable(false);
	if (local_coeff_p) delete[] local_coeff_p;
	
	// allocate memory for the filter coefficients`
	const uint16_t requested_n_coeff = min(INTERP_FIR_MAX_COEFFS, _n_coeffs);
	local_coeff_p = new float32_t[requested_n_coeff];
	if (!local_coeff_p) {
		print_ptr->println("AudioRateInterpolator_F32: begin: *** ERROR ***: could not allocate memory for filter coefficients.");
		return get_is_enabled();
	}
	
	//generate the lowpass filter coefficients
	generate_interp_coeffs(requested_n_coeff, _up_fac, local_coeff_p);
	
	//call the full begin() method
	return begin(local_coeff_p, requested_n_coeff, _up_fac, _in_block_size);

}


bool AudioRateInterpolator_F32::begin(const float32_t *cp, const uint16_t _n_coeffs, const uint8_t _upsamp_fac, const int in_block_size) {  //or, you can provide it with the block size
	coeff_p = cp;
	n_coeffs = _n_coeffs;
	upsamp_fac = _upsamp_fac;
	flag_useLinearInterp = false;
	
	if (_upsamp_fac == 1) {
		//just do pass-thru
		coeff_p = coeff_passthru;
		n_coeffs = 1;
		upsamp_fac = 1;		
	}
		
	if (in_block_size != start_audio_block_samples) {
			print_ptr->println(F("AudioRateInterpolator_F32: begin: *** WARNING ***: given in_block_size does not match expectation."));
			print_ptr->println("    : given in_block_size = " + String(in_block_size));
			print_ptr->println("    : expected start_audio_block_sampls = " + String(start_audio_block_samples));
			print_ptr->println("    : continuing with out_block_size...");
	}

	// Initialize Interpolator instance (ARM DSP Math Library)
	if (coeff_p && (coeff_p != INTERP_FIR_F32_PASSTHRU) && n_coeffs <= fir_max_coeffs) {
		//initialize the ARM FIR module
		arm_status status = arm_fir_interpolate_init_f32(&interp_inst, upsamp_fac, n_coeffs, (float32_t *)coeff_p,  &StateF32[0], in_block_size);
		if (status != ARM_MATH_SUCCESS) {
			print_ptr->println(F("AudioRateInterpolator_F32: begin: *** ERROR ***: arm_fir_interpolate_init_f32 failed."));
			print_ptr->flush();
			delay(500); while (1);
		}
		configured_block_size = in_block_size;
		
		is_armed = true;
		is_enabled = true;
		flag_useLinearInterp = false;
	} else {
		is_enabled = false;
		flag_useLinearInterp = true;
	}
	
	start_audio_block_samples = in_block_size;
	end_audio_block_samples = upsamp_fac * start_audio_block_samples;
	end_sample_rate_Hz = start_sample_rate_Hz * upsamp_fac;

	//print_ptr->println("AudioRateInterpolator_F32: begin complete " + String(is_armed) + " " + String(is_enabled) + " " + String(get_is_enabled()));
	
	return get_is_enabled();
}

void AudioRateInterpolator_F32::update(void)
{
	audio_block_f32_t *block, *block_new;

	if (!is_enabled) return;

	//print_ptr->println("AudioRateInterpolator_F32: update: starting...");

	block = AudioStream_F32::receiveReadOnly_f32();
	if (!block) return;  //no data to get

	// If there's no coefficient table, give up.  
	if (coeff_p == NULL) {
		AudioStream_F32::release(block);
		return;
	}

	// do passthru
	if ((upsamp_fac == 1) || (coeff_p == INTERP_FIR_F32_PASSTHRU)) {
		// Just pass through
		AudioStream_F32::transmit(block);
		AudioStream_F32::release(block);
		//print_ptr->println("AudioRateInterpolator_F32: update(): PASSTHRU.");
		return;
	}

	// get a block for the output
	block_new = AudioStream_F32::allocate_f32();
	if (block_new == NULL) { AudioStream_F32::release(block); return; } //failed to allocate
	
	//apply the interpolation process
	if (flag_useLinearInterp) {
		processAudioBlock_linearInterp(block, block_new);
	} else {
		processAudioBlock(block,block_new);
	}

	//transmit the data and release the memory blocks
	AudioStream_F32::transmit(block_new); // send the output
	AudioStream_F32::release(block_new);  // release the memory
	AudioStream_F32::release(block);	    // release the memory
	
}


int AudioRateInterpolator_F32::processAudioBlock_linearInterp(audio_block_f32_t *block, audio_block_f32_t *block_new)
{
	if ((is_enabled == false) || (block==NULL) || (block_new==NULL)) return -1;
	
	int Idest = 0;
	float32_t interp_scale_fac = 1.0f/upsamp_fac;
	for (int Isource=0; Isource < start_audio_block_samples; Isource++) {
		const float32_t interp_fac = (block->data[Isource] - prev_sample) * interp_scale_fac;
		float32_t interp_val = prev_sample; 
		for (int Isubstep=0; Isubstep < upsamp_fac; Isubstep++) {
			block_new->data[Idest++] = interp_val;
			interp_val += interp_fac;
		}
		prev_sample = block->data[Isource];
	}
	
	//set metadata
	block_new->id = block->id;
	block_new->fs_Hz = end_sample_rate_Hz;
	block_new->length = end_audio_block_samples;
	
	return 0;
}



int AudioRateInterpolator_F32::processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *block_new) {
	if ((is_enabled == false) || (block==NULL) || (block_new==NULL)) return -1;
	
	//check to make sure our Interpolator instance has the right size
//	if (block->length != configured_block_size) {
//		//doesn't match.  re-initialize
//		print_ptr->println("AudioRateInterpolator_F32: block size (" + String(block->length) + ") doesn't match expectation (" + String(configured_block_size) + ").  Re-initializing Interpolator.");
//		begin(coeff_p, n_coeffs, upsamp_fac, block->length);  //initialize with same coefficients, just a new block length
//	}
	
	//apply the Interpolator
	arm_fir_interpolate_f32(&interp_inst, block->data, block_new->data, configured_block_size);
	
	//copy info about the block
	block_new->length = end_audio_block_samples;
	block_new->id = block->id;	
	block_new->fs_Hz = end_sample_rate_Hz;
	
	return 0;
}


void AudioRateInterpolator_F32::printCoeff(int start_ind, int end_ind) {
	start_ind = min(n_coeffs-1,max(0,start_ind));
	end_ind = min(n_coeffs,max(0,end_ind));
	print_ptr->print("AudioRateInterpolator_F32: printCoeff [" + String(start_ind) + ", " + String(end_ind) + "): ");
	for (int i=start_ind; i<end_ind; i++) {
		print_ptr->print(coeff_p[i],4); 
		print_ptr->print(", ");
	}
	print_ptr->println();				
}



#endif