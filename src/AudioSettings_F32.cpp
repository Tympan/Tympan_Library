

#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

float AudioSettings_F32::cpu_load_percent(const int n) {
	#if defined(KINETISK)
	//teensy 3.6...modified from Teensy3/AudioStream.h
	
	//n is the number of cycles
	//#define CYCLE_COUNTER_APPROX_PERCENT(n) (((n) + (F_CPU / 32 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / (F_CPU / 16 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))
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