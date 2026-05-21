

#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

float AudioSettings_F32::get_cpu_load_divide_fac(void) { // return devide_fac for: CPU_percent = n_cycles / divide_fac
	float foo2 = 0.0f;
	#if defined(KINETISK)
		//teensy 3.6...modified from Teensy3/AudioStream.h
		//n is the number of cycles
		//#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU / 32 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU / 16 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))
		foo2 = (float)( F_CPU/16 ) / sample_rate_Hz;
		foo2 *= ((float)audio_block_samples);
		foo2 /= 100.f;
	#else
		//teensy 4...modified from Teensy4/AudioStream.h
		//n is the number of cycles
		//#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU_ACTUAL / 128 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU_ACTUAL / 64 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))
		foo2 = (float)( F_CPU_ACTUAL/64 ) / sample_rate_Hz;
		foo2 *= ((float)audio_block_samples);
		foo2 /= 100.f;
	#endif
	return foo2;
}

float AudioSettings_F32::cpu_load_percent(const int n) { //n is the number of cycles
/*
	#if defined(KINETISK)
	//teensy 3.6...modified from Teensy3/AudioStream.h
	//#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU / 32 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU / 16 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))
	//
	// Re-expressed, the equation above looks like:  CPU_percent = n / divide_fac + 0.5
	// The "+ 0.5" at the end is so that (in the historical code) it would *round* to the nearest integer (not truncate).
	// We're returning a float, so we don't care about this rounding.  We'll leave it out.
	// https://forum.pjrc.com/index.php?threads/cpu-cycle-counter-formula-in-audiostream.63898/
	
	float foo1 = 0;
	//foo1 += ((float)(F_CPU / 32))/sample_rate_Hz;
	//foo1 *= ((float)audio_block_samples);
	//foo1 /= 100.f;
	foo1 += (float)n;
	float foo2 = (float)(F_CPU / 16)/sample_rate_Hz;
	foo2 *= ((float)audio_block_samples);
	foo2 /= 100.f;
	return  foo1 / foo2;

	//return (((n) + (F_CPU / 32 / sample_rate_Hz * audio_block_samples / 100)) / (F_CPU / 16 / sample_rate_Hz * audio_block_samples / 100));

	#else
	//teensy 4...modified from Teensy4/AudioStream.h
	//n is the number of cycles
	//#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU_ACTUAL / 128 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU_ACTUAL / 64 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))
	//
	// Re-expressed, the equation above looks like:  CPU_percent = n / divide_fac + 0.5
	// The "+ 0.5" at the end is so that (in the historical code) it would *round* to the nearest integer (not truncate).
	// We're returning a float, so we don't care about this rounding.  We'll leave it out.
	// https://forum.pjrc.com/index.php?threads/cpu-cycle-counter-formula-in-audiostream.63898/
	
	float foo1 = 0;
	//foo1 +=((float)(F_CPU_ACTUAL / 128))/sample_rate_Hz;
	//foo1 *= ((float)audio_block_samples);
	//foo1 /= 100.f;
	foo1 += (float)n;
	float foo2 = (float)(F_CPU_ACTUAL / 64)/sample_rate_Hz;
	foo2 *= ((float)audio_block_samples);
	foo2 /= 100.f;
	return  foo1 / foo2;	
	#endif
*/

	return (float(n) / get_cpu_load_divide_fac());
}

float AudioSettings_F32::processorUsage(void) { 
	return cpu_load_percent(AudioStream::cpu_cycles_total); 
};
float AudioSettings_F32::processorUsageMax(void) { 
	return cpu_load_percent(AudioStream::cpu_cycles_total_max); 
}
void AudioSettings_F32::processorUsageMaxReset(void) { 
	AudioStream::cpu_cycles_total_max = AudioStream::cpu_cycles_total; 
}

unsigned long AudioSettings_F32::millis(void) {
	uint32_t n_audio_blocks = AudioStream_F32::update_counter;
	double millis_per_block = ((double)audio_block_samples / (double)sample_rate_Hz) * 1000.0;
	return (unsigned long)((n_audio_blocks * millis_per_block)+0.5);
}