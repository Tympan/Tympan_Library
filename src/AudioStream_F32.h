/*
 * AudioStream_F32
 * 
 * Created: Chip Audette, November 2016
 * Purpose; Extend the Teensy Audio Library's "AudioStream" to permit floating-point audio data.
 *          
 * I modeled it directly on the Teensy code in "AudioStream.h" and "AudioStream.cpp", which are 
 * available here: https://github.com/PaulStoffregen/cores/tree/master/teensy3
 * 
 * MIT License.  use at your own risk.
*/

#ifndef _AudioStream_F32_h
#define _AudioStream_F32_h

//include <Audio.h> //Teensy Audio Library
#include "arm_math.h" //simply to define float32_t
#include <core_pins.h> //for F_CPU_ACTUAL ??
#include <AudioStream.h> 
#include "AudioSettings_F32.h"
#include <Arduino.h>
#include <vector>

#if defined(__IMXRT1062__)   //for Teensy 4...this shouldn't be necessary
extern volatile uint32_t F_CPU_ACTUAL;  
#endif

// /////////////// class prototypes
class AudioStream_F32;
class AudioConnection_F32;

#define MAX_AUDIO_BLOCK_SAMPLES_F32  (AUDIO_BLOCK_SAMPLES) //hopefully, this macro is obsolete, but if not, you can set this bigger
//#define MAX_AUDIO_BLOCK_SAMPLES_F32  (1024)
#define MIN_AUDIO_BLOCK_SAMPLES_F32  (AUDIO_BLOCK_SAMPLES) //never go smaller than this (for historical reasons...classes might have been written assuming that this was the smallest length of audio_block->data[]

// ///////////// class definitions

//create a new structure to hold audio as floating point values.
//modeled on the existing teensy audio block struct, which uses Int16
//https://github.com/PaulStoffregen/cores/blob/268848cdb0121f26b7ef6b82b4fb54abbe465427/teensy3/AudioStream.h
class audio_block_f32_t : public audio_block_t {
	public:
		audio_block_f32_t(void)
		{
			full_length = max(MIN_AUDIO_BLOCK_SAMPLES_F32, MAX_AUDIO_BLOCK_SAMPLES_F32);
			data = new float32_t[full_length];
			length = full_length;
		}
		audio_block_f32_t(const AudioSettings_F32 &settings)
		{
			full_length = max(MIN_AUDIO_BLOCK_SAMPLES_F32, settings.audio_block_samples);
			data = new float32_t[full_length];
			fs_Hz = settings.sample_rate_Hz;
			length = settings.audio_block_samples;
		}
		~audio_block_f32_t(void) {
			if (data != NULL) delete data;
			audio_block_t::~audio_block_t();
		}
		
		//unsigned char ref_count_f32;  //we'll use ref_count in audio_block_t instead
		unsigned char memory_pool_index_f32;
		//unsigned char reserved1;
		//unsigned char reserved2;
		float32_t *data; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
		int full_length = MAX_AUDIO_BLOCK_SAMPLES_F32; //MAX_AUDIO_BLOCK_SAMPLES_F32
		int length = MAX_AUDIO_BLOCK_SAMPLES_F32; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
		float fs_Hz = AUDIO_SAMPLE_RATE; // AUDIO_SAMPLE_RATE is 44117.64706 from AudioStream.h
		unsigned long id;
	private:
};

#define AUDIOSTREAMF32_MAX_INT16_CONNECTIONS 8
class AudioStream_F32 : public AudioStream {
  public:
    AudioStream_F32(unsigned char n_inputs, audio_block_f32_t **iqueue) : 
		    AudioStream(min(n_inputs, AUDIOSTREAMF32_MAX_INT16_CONNECTIONS), inputQueueArray_i16), 
        inputQueue_f32(iqueue) {
      //active_f32 = false;
      //destination_list_f32 = NULL;
      for (int i=0; i < n_inputs; i++) {
        inputQueue_f32[i] = NULL;
      }
			// add to a simple list, for update_all
			// TODO: replace with a proper data flow analysis in update_all
			//if (first_update_f32 == NULL) {
			//	first_update_f32 = this;
			//} else {
			//	AudioStream_F32 *p;
			//	for (p=first_update_f32; p->next_update_f32; p = p->next_update_f32) ;
			//	p->next_update_f32 = this;
			//}
			//next_update_f32 = NULL;
			//cpu_cycles = 0;
			//cpu_cycles_max = 0;
			//numConnections = 0;			
			
			//added by WEA to track usage
			if (numInstances < AudioStream_F32::maxInstanceCounting) allInstances[numInstances++] = this;
    };
		virtual ~AudioStream_F32(void);

    //static void initialize_f32_memory(audio_block_f32_t *data, unsigned int num);
    //static void initialize_f32_memory(audio_block_f32_t *data, unsigned int num, const AudioSettings_F32 &settings);
    static void initialize_f32_memory(const unsigned int num);
		static void initialize_f32_memory(const unsigned int num, const AudioSettings_F32 &settings);
	
    //virtual void update(audio_block_f32_t *) = 0; 
    static uint8_t f32_memory_used;
    static uint8_t f32_memory_used_max;
    static audio_block_f32_t * allocate_f32(void);
    static void release(audio_block_f32_t * block);
	
		//added for controlling whether calculations are done or not
		static bool setIsAudioProcessing(bool enable) { if (enable) { return update_setup(); } else { return update_stop(); } };
		static bool getIsAudioProcessing(void) { return isAudioProcessing; }
		
		//added for tracking and debugging how algorithms are called
		static AudioStream_F32* allInstances[]; 
		static int numInstances; 
		static const int maxInstanceCounting;
		//static void printNextUpdatePointers(void); 
		static void printAllInstances(void);
		String instanceName = String("NotNamed");
		
		static void reset_update_counter(void) { update_counter = 0; }
		static uint32_t update_counter;
	
  protected:
    //bool active_f32;
    //unsigned char num_inputs_f32;
    void transmit(audio_block_f32_t *block, unsigned char index = 0);
    audio_block_f32_t * receiveReadOnly_f32(unsigned int index = 0);
    audio_block_f32_t * receiveWritable_f32(unsigned int index = 0);  
    friend class AudioConnection_F32;
		static bool update_setup(void) { return isAudioProcessing = AudioStream::update_setup(); }
		static bool update_stop(void) { AudioStream::update_stop(); return isAudioProcessing = false; }
		static void update_all(void) { update_counter++; AudioStream::update_all(); }
		static bool isAudioProcessing; //try to keep the same as AudioStream::update_scheduled, which is private and inaccessible to me :(
		
  private:
		//static AudioConnection_F32* unused_f32; // linked list of unused but not destructed connections
    //AudioConnection_F32 *destination_list_f32;
    audio_block_f32_t **inputQueue_f32;
    virtual void update(void) = 0;
		//static AudioStream_F32 *first_update_f32; // don't conflict with AudioStream::update_all, but it's private in AudioStream, so I need a version here so that AudioConnection_F32 can use something like it.
    //AudioStream_F32 *next_update_f32; // don't conflict with AudioStream::update_all, but it's private in AudioStream, so I need a version here so that AudioConnection_F32 can use something like it.
		audio_block_t *inputQueueArray_i16[AUDIOSTREAMF32_MAX_INT16_CONNECTIONS];  //assume max of 8 possible int16 connections (nearly all instances will actually use zero)
    //static audio_block_f32_t *f32_memory_pool;
    static std::vector<audio_block_f32_t *> f32_memory_pool;
    static uint32_t f32_memory_pool_available_mask[6];
		static void allocate_f32_memory(const unsigned int num);
		static void allocate_f32_memory(const unsigned int num, const AudioSettings_F32 &settings);
};


class AudioConnection_F32 : public AudioConnection
{
  public:
		AudioConnection_F32() : AudioConnection() {};
    //AudioConnection_F32(AudioStream_F32 &source, AudioStream_F32 &destination)
    //  : AudioConnection(source, (unsigned char)0, destination, (unsigned char)0) {}
    AudioConnection_F32(AudioStream_F32 &source, unsigned char sourceOutput,
      AudioStream_F32 &destination, unsigned char destinationInput)
			: AudioConnection(source, sourceOutput, destination, destinationInput) {}
		virtual ~AudioConnection_F32() {
			AudioConnection_F32::disconnect();
			AudioConnection::~AudioConnection();
		}
    friend class AudioStream_F32;
		//virtual ~AudioConnection_F32(); 
		int disconnect(void);
		//int connect(void);
		//int connect(AudioStream_F32 &source, AudioStream_F32 &destination) {return connect(source,0,destination,0);};
		//int connect(AudioStream_F32 &source, unsigned char sourceOutput,
		//	AudioStream_F32 &destination, unsigned char destinationInput);
  protected:
    //AudioStream_F32 *src;
    //AudioStream_F32 *dst;
    //unsigned char src_index;
    //unsigned char dest_index;
    //AudioConnection_F32 *next_dest;
		//bool isConnected;
};




/*
#define AudioMemory_F32(num) ({ \
  static audio_block_f32_t data_f32[num]; \
  AudioStream_F32::initialize_f32_memory(data_f32, num); \
})
*/

void AudioMemory_F32(const int num);
void AudioMemory_F32(const int num, const AudioSettings_F32 &settings);
void AudioMemory_F32(const unsigned int num);
void AudioMemory_F32(const unsigned int num, const AudioSettings_F32 &settings);
#define AudioMemory_F32_wSettings(num,settings) (AudioMemory_F32(num,settings))   //for historical compatibility


#define AudioMemoryUsage_F32() (AudioStream_F32::f32_memory_used)
#define AudioMemoryUsageMax_F32() (AudioStream_F32::f32_memory_used_max)
#define AudioMemoryUsageMaxReset_F32() (AudioStream_F32::f32_memory_used_max = AudioStream_F32::f32_memory_used)


#endif