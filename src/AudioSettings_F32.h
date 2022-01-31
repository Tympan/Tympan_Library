
#ifndef _AudioSettings_F32_
#define _AudioSettings_F32_

#include "arm_math.h"  //simply to define float32_t

class AudioSettings_F32 {
	public:
		AudioSettings_F32(void) {
			sample_rate_Hz = 44100.f;
			audio_block_samples = 128;
		}
		AudioSettings_F32(const float fs_Hz, const int block_size) {
			sample_rate_Hz = fs_Hz;
			audio_block_samples = block_size;
		}
		float sample_rate_Hz;
		int audio_block_samples;
		
		float cpu_load_percent(const int n);
		float processorUsage(void);
		float processorUsageMax(void);
		void processorUsageMaxReset(void);
};

#endif