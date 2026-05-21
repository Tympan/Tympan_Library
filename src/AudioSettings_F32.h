
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
		
		float get_cpu_load_divide_fac(void);   // return devide_fac for: CPU_percent = n_cycles / divide_fac
		float cpu_load_percent(const int n);   // convert any CPU load counter into % of total CPU time available
		float processorUsage(void);            // return current CPU usage by update_all()...ie, all audio processing blocks
		float processorUsageMax(void);         // return maximum CPU usage by update_all()...ie, all audio processing blocks
		void processorUsageMaxReset(void);     // reset the maximum tracking
		unsigned long millis(void); //return milliseconds as counted by the number of calls to AudioStream_F32::update_all() instead of the regular millis() that is based on the CPU clock
};

#endif