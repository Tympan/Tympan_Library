
#include "AudioPlayMemory_F32.h"


int AudioPlayMemoryQueue::addSample(SampleInfo sample) {
	if (queue_len >= MAX_AUDIOPLAYMEMORYQUEUE_LEN) {
		Serial.println("AudioPlayMemoryQueue: addSample: *** ERROR ***: Queue already has " + String(queue_len) + " samples");
		Serial.println("                               : Cannot add another sample.");
		return -1;
	} else {
		int cur_write_ind = queue_len;
		all_sampleInfo[cur_write_ind] = sample; //I want this to make a local copy.  Hopefully it does!
		queue_len++;  //queue is now one longer
	}
	//Serial.println("AudioPlayMemoryQueue:addSample: queue length = " + String(queue_len)); 
	return queue_len;
}

int AudioPlayMemoryI16_F32::setCurrentSampleFromQueue(int ind) {
	if (ind < 0) return -1;
	const int queueLen = playQueue.getQueueLen();
	if (queueLen == 0) return -1;
	if (ind >= queueLen) return -1;
	
	//copy the sample information
	queue_ind = ind;
	data_ptr = playQueue.all_sampleInfo[queue_ind].data_ptr;
	data_len = playQueue.all_sampleInfo[queue_ind].length;
	setDataSampleRate_Hz(playQueue.all_sampleInfo[queue_ind].sample_rate_Hz);

	//reset the playback counters
	data_ind = 0;
	data_phase = 0.0f;
	
	//Serial.println("AudioPlayMemoryI16_F32: setCurrentSampleFromQueue: queue ind = " + String(queue_ind)
	//    + ", data_ptr = " + String((int)data_ptr) 
	//	+ ", data_len = " + String(data_len)
	//	+ ", sample_rate = " + String(sample_rate_Hz));
	return queue_ind;
}
		
#define CONVERT_I16_TO_F32(x) (((float32_t)x)/(16384.0f))

void AudioPlayMemoryI16_F32::update(void) {
  if (state != PLAYING) return;

  //get memory for the output
  audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if (out_block == NULL) return; //no memory available.

  //fill the out_block with audio
  for (uint32_t dest_ind = 0; dest_ind < ((uint32_t)out_block->length); dest_ind++) {
	 out_block->data[dest_ind] = getNextAudioValue(); //in this method, it will also alter the play/stop state
  }

  //transmit and release
  AudioStream_F32::transmit(out_block);
  AudioStream_F32::release(out_block);
}

float32_t AudioPlayMemoryI16_F32::getNextAudioValue(void) {
	//Serial.println("AudioPlayMemoryI16_F32: update: data_phase = " + String(data_phase) + ", data_ind = " + String(data_ind));
	
	
	if (data_ind >= data_len) { //have we reached the end of the current data buffer?
		//yes, we've reached the end of the current buffer. 
      
		//is there another buffer of audio in the queue?
		int candidate_queue_ind = queue_ind + 1;
		if (candidate_queue_ind >= playQueue.getQueueLen()) {
			//there are no more queue'd data buffers, so we're done
			state = STOPPED;
			return 0.0;  //give a zero value return
		} else {
			//there are more buffers in the queue, so let's use them!
			setCurrentSampleFromQueue(candidate_queue_ind);  //switch to the new audio buffer
		}
	}
	
	//catch a degenerate case: if the current data buffer has zero length.  In this weird case
	//let's just issue a zero and return, without changing the play state.  The next time
	//this function gets called, it'll try to increment the buffers, or it'll stop playing.
	//It would have been great to do that incrementing now, but I don't have time.  Sorry.
	if (data_ind >= data_len) return 0.0;
	
	//if we're here, we should have data to work with.  So, let's go!
	
	// get the current data value from the buffer
	float ret_val = CONVERT_I16_TO_F32(data_ptr[data_ind]);   //get the next piece of data

	//compute the data index for next time this gets called
	data_phase += data_phase_incr;                  //increment the phase
	uint32_t whole_step = (unsigned int)data_phase; //have we increased a whole step yet?
	data_ind += whole_step;                         //increment the data index by any whole steps
	data_phase -= (float32_t)whole_step;            //remove the whole step from the data_phase to keep it well bounded
	
	return ret_val;
  }
	
