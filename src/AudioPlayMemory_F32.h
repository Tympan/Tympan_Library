
#ifndef _AudioPlayMemory_F32_h
#define _AudioPlayMemory_F32_h

#include <Arduino.h>
#include <AudioSettings_F32.h>
#include <AudioStream_F32.h>

/*
 * AudioPlayMemory16_F32
 * 
 * Purpose: Play F32 audio from RAM memory.  Values are saved in memory as Int16 data type
 * 
 * Created: Chip Audette, OpenAudio, Aug 2023
 * 
 * Notes: This player will upsample or downsample the audio to handle different sample rates.
 *        But, as of writing this comment, it does not interpolate or filter; it only does 
 *        a very-simple zero-order-hold (ie, stair-step) based on the closest sample.  You
 *        might not like the audio artifacts that result.   Feel free to improve it!
 *
 * Notes: This player will play a single sample or it can seemlessly play a series of samples.
 *            - If you want a single sample, you can specify just that one sample as a data array
 *              plus its length and its sample rate.  Or, you can wrap up that same info in a
 *              "SampleInfo" object.
 *            - If you want to play a series of samples in a row, you must use an "AudioPlayMemoryQueue"
 *              object.  This object holds a bunch of "SampleInfo" instances within it.
 */
 
 //define a structure to hold the sample info...but NOT the sample values themselves, just the info
class SampleInfo {
	public:
		SampleInfo() {};
		SampleInfo(const int16_t *_data, const uint32_t _data_len, const float32_t _data_fs_Hz) {
			data_ptr = _data; length = _data_len; sample_rate_Hz = _data_fs_Hz;
		}
		SampleInfo& operator=(const SampleInfo& sample) { //ensure a proper (shallow) copy
			data_ptr = sample.data_ptr;
			length = sample.length;
			sample_rate_Hz = sample.sample_rate_Hz;
			return *this;
		}
		const int16_t *data_ptr;
		uint32_t length;
		float32_t sample_rate_Hz;
};

//if you want to queue up many samples, use the class below!
#define MAX_AUDIOPLAYMEMORYQUEUE_LEN 10
class AudioPlayMemoryQueue {
	public:
		AudioPlayMemoryQueue(void) { reset(); };
		AudioPlayMemoryQueue(const int16_t *data, const uint32_t data_len, const float32_t data_fs_Hz) {
			reset(); 
			addSample(data,data_len,data_fs_Hz);
		}
		AudioPlayMemoryQueue(SampleInfo sample) { addSample(sample); } 
		
		int addSample(const int16_t *data, const uint32_t data_len, const float32_t data_fs_Hz) {
			return addSample(SampleInfo(data,data_len,data_fs_Hz));
		}
		int addSample(SampleInfo sample);
		int getQueueLen(void) { return queue_len; };
		void reset(void) { queue_len = 0; }
		AudioPlayMemoryQueue& operator=(const AudioPlayMemoryQueue& queue) { //ensure a proper copy
			queue_len = queue.queue_len;
			for (int i=0; i<MAX_AUDIOPLAYMEMORYQUEUE_LEN; i++) all_sampleInfo[i] = queue.all_sampleInfo[i];
			return *this;
		}
		SampleInfo all_sampleInfo[MAX_AUDIOPLAYMEMORYQUEUE_LEN];
	private:
		int queue_len = 0;
};
 
class AudioPlayMemoryI16_F32 : public AudioStream_F32
{
//GUI: inputs:0, outputs:1  //this line used for automatic generation of GUI nodes 
  public:
    AudioPlayMemoryI16_F32(void) : AudioStream_F32(0, NULL) { }
    AudioPlayMemoryI16_F32(const AudioSettings_F32 &settings) : AudioStream_F32(0, NULL)
    { 
      setSampleRate_Hz(settings.sample_rate_Hz);  
	  setDataSampleRate_Hz(settings.sample_rate_Hz); 
    }
	
	// here are the play methods.  Here's where you tell it to start playing (and what to play)
	virtual bool play(AudioPlayMemoryQueue _playQueue) { 
		stop();	                       //stop any playback that might be already happening
		playQueue = _playQueue;        //make a local copy of the new queue
		setCurrentSampleFromQueue(0);  //start at queue index zero
		state = PLAYING;               //set the flag to allow the update() method to start playing
		return state;
	}
    virtual bool play(SampleInfo &sample) { 
		return play(AudioPlayMemoryQueue(sample));  //wrap the sample info into a queue
	}
	virtual bool play(const int16_t *data, const uint32_t data_len, const float32_t data_fs_Hz) {
		return play(AudioPlayMemoryQueue(SampleInfo(data,data_len,data_fs_Hz)));  //wrap the sample info into a queue
    }
	
	//here's how you check in on whether it is playing
	enum STATE {STOPPED=0, PLAYING};
    virtual bool isPlaying(void) { if (state == PLAYING) { return true; } else { return false; } }

	//here's how you force it to stop playing
    virtual void stop(void) { state = STOPPED; }	
	
	//utility stuff
    virtual void update(void); //here is what plays out the audio to the audio subsystem
    float setSampleRate_Hz(float fs_Hz) { 
      sample_rate_Hz = fs_Hz; 
      data_phase_incr = data_sample_rate_Hz / sample_rate_Hz;
      return sample_rate_Hz;
    }
    float setDataSampleRate_Hz(float data_fs_Hz) {
      data_sample_rate_Hz = data_fs_Hz;
      data_phase_incr = data_sample_rate_Hz / sample_rate_Hz;
      //Serial.println("AudioPlayMemoryI16_F32: data_fs_Hz = " + String(data_fs_Hz) + ", data_phase_incr = " + String(data_phase_incr));
      return data_sample_rate_Hz;
    }
	int setCurrentSampleFromQueue(int ind);


    private:
      int state = STOPPED;
      const int16_t *data_ptr = NULL;
      uint32_t data_ind = 0;
      float32_t data_phase = 0.0f;
      float32_t data_phase_incr = 1.0f;
      uint32_t data_len = 0;
      float32_t data_sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
      float sample_rate_Hz = ((float)AUDIO_SAMPLE_RATE_EXACT);
      //int audio_block_samples = AUDIO_BLOCK_SAMPLES;
	  
	  //related to the queue of audio samples
	  AudioPlayMemoryQueue playQueue;
	  float32_t getNextAudioValue(void);
	  int queue_ind;
};


#endif