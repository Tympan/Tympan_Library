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
class audio_block_f32_t {
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
		~audio_block_f32_t(void) 
		{
			if (data != NULL) delete [] data;
		}
		
		
		unsigned char ref_count;
		unsigned char memory_pool_index;
		unsigned char reserved1;
		unsigned char reserved2;
		float32_t *data; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
		int full_length = MAX_AUDIO_BLOCK_SAMPLES_F32; //MAX_AUDIO_BLOCK_SAMPLES_F32
		int length = MAX_AUDIO_BLOCK_SAMPLES_F32; // AUDIO_BLOCK_SAMPLES is 128, from AudioStream.h
		float fs_Hz = AUDIO_SAMPLE_RATE; // AUDIO_SAMPLE_RATE is 44117.64706 from AudioStream.h
		unsigned long id;
	private:
};

class AudioConnection_F32
{
  public:
    AudioConnection_F32(AudioStream_F32 &source, AudioStream_F32 &destination) :
      src(source), dst(destination), src_index(0), dest_index(0),
      next_dest(NULL)
      { connect(); }
    AudioConnection_F32(AudioStream_F32 &source, unsigned char sourceOutput,
      AudioStream_F32 &destination, unsigned char destinationInput) :
      src(source), dst(destination),
      src_index(sourceOutput), dest_index(destinationInput),
      next_dest(NULL)
      { connect(); }
    friend class AudioStream_F32;
  protected:
    void connect(void);
    AudioStream_F32 &src;
    AudioStream_F32 &dst;
    unsigned char src_index;
    unsigned char dest_index;
    AudioConnection_F32 *next_dest;
};


class AudioStream_F32 : public AudioStream {
  public:
    AudioStream_F32(unsigned char n_input_f32, audio_block_f32_t **iqueue) : 
			AudioStream(1, inputQueueArray_i16), num_inputs_f32(n_input_f32), inputQueue_f32(iqueue)
		{
      //active_f32 = false;
      destination_list_f32 = NULL;
      for (int i=0; i < n_input_f32; i++) {
        inputQueue_f32[i] = NULL;
      }
			if (numInstances < AudioStream_F32::maxInstanceCounting) allInstances[numInstances++] = this;
    };
    //static void initialize_f32_memory(audio_block_f32_t *data, unsigned int num);
    //static void initialize_f32_memory(audio_block_f32_t *data, unsigned int num, const AudioSettings_F32 &settings);
    static void initialize_f32_memory(const unsigned int num);
	  static void initialize_f32_memory(const unsigned int num, const AudioSettings_F32 &settings);
	
    //virtual void update(audio_block_f32_t *) = 0; 
    static uint8_t f32_memory_used;
    static uint8_t f32_memory_used_max;
    static audio_block_f32_t * allocate_f32(void);
    static void release(audio_block_f32_t * block);
	
		//Control the global update_all() process handled by the underlying AudioStream class.
		//These affect the *global* audio processing behavior, not the per-instance behavior.
		//The methods below should only be used with care...like in the I2S classes.
		static bool setIsAudioProcessing(bool enable) { if (enable) { return update_setup(); } else { return update_stop(); } };
		static bool getIsAudioProcessing(void) { return isAudioProcessing; }
		
		//Control the update process for a single instance of AudioStream_f32.  I am re-using
		//the "active" data member from AudioStream.h.  Typically, this is only set/cleared when
		//the AudioConnections are set/cleared.  During the global update_all() process, this
		//"active" flag is checked to see if update_all() should execute the instance's update()
		//method.  By given greater access here, perhaps I am expanding the use of "active" too far?
		//
		//bool active; //This is already in AudioStream.h as "protected"
		//bool isActive(void) { return active; }  //this is already in AudioStream.h as "public"
		virtual bool setActive(bool _active) { return active = _active; }  //added here in AudioStream_F32.h
		virtual bool setActive(bool _active, int flag_setup) { return active = _active; }  //default to ignore the setup flag.  Override in your derived class, if you wish
		
		
		//added for tracking and debugging how algorithms are called
		static AudioStream_F32* allInstances[]; 
		static int numInstances; 
		static const int maxInstanceCounting;
		//static void printNextUpdatePointers(void); 
		static void printAllInstances(void);
		String instanceName = String("NotNamed");
		
		static void reset_update_counter(void) { update_counter = 0; }
		static uint32_t update_counter;
		
		//added to enable AudioStreamComposite_F32 to put its inputs into another AudioStream_F32 inputs
		bool putBlockInInputQueue(audio_block_f32_t *block, unsigned int ind);
		
		//moved from protected to public to enable AudioForwarder_F32 to work
    void transmit(audio_block_f32_t *block, unsigned char index = 0);
		
  protected:
    //bool active_f32;
    unsigned char num_inputs_f32;
    audio_block_f32_t * receiveReadOnly_f32(unsigned int index = 0);
    audio_block_f32_t * receiveWritable_f32(unsigned int index = 0);  
    friend class AudioConnection_F32;
	
		//Control the global update_all() process handled by the underlying AudioStream class.
		//These affect the *global* audio processing behavior, not the per-instance behavior.
		//The methods below should only be used with care...like in the I2S classes.
		static bool update_setup(void) { return isAudioProcessing = AudioStream::update_setup(); }        //setup the global "update" process...not per instance, global! 
		static bool update_stop(void) { AudioStream::update_stop(); return isAudioProcessing = false; }   //stop the global "update" process...not per instance, global!
		static void update_all(void) { update_counter++; AudioStream::update_all(); }											//force th execution of the global "update" process...not per instance, global!
		static bool isAudioProcessing; //try to keep the same as AudioStream::update_scheduled, which is private and inaccessible to me :(
		
  private:
    AudioConnection_F32 *destination_list_f32;
    audio_block_f32_t **inputQueue_f32;
    virtual void update(void) = 0;
    audio_block_t *inputQueueArray_i16[1];  //two for stereo
    //static audio_block_f32_t *f32_memory_pool;
    static std::vector<audio_block_f32_t *> f32_memory_pool;
    static uint32_t f32_memory_pool_available_mask[6];
		static void allocate_f32_memory(const unsigned int num);
		static void allocate_f32_memory(const unsigned int num, const AudioSettings_F32 &settings);
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