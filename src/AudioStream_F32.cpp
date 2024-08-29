
#include <Arduino.h>
#include "AudioStream_F32.h"
#include <arm_math.h> //ARM DSP extensions.  for speed!

//if defined(__MKL26Z64__)
  //define MAX_AUDIO_MEMORY 6144
//elif defined(__MK20DX128__)
  //define MAX_AUDIO_MEMORY 12288
//elif defined(__MK20DX256__)
  //define MAX_AUDIO_MEMORY 49152
//elif defined(__MK64FX512__)
  //define MAX_AUDIO_MEMORY 163840
//elif defined(__MK66FX1M0__)
  //define MAX_AUDIO_MEMORY 229376
//elif defined(__IMXRT1062__)
  //define MAX_AUDIO_MEMORY 229376
//endif

//define NUM_MASKS  (((MAX_AUDIO_MEMORY / AUDIO_BLOCK_SAMPLES / 2) + 31) / 32)

//audio_block_f32_t * AudioStream_F32::f32_memory_pool;
std::vector<audio_block_f32_t *> AudioStream_F32::f32_memory_pool;
uint32_t AudioStream_F32::f32_memory_pool_available_mask[6];

uint8_t AudioStream_F32::f32_memory_used = 0;
uint8_t AudioStream_F32::f32_memory_used_max = 0;
AudioConnection_F32* AudioStream_F32::unused_f32 = NULL; // linked list of unused but not destructed connections

//added 2021-02-17
const int AudioStream_F32::maxInstanceCounting = 120;
//bool AudioStream_F32::printUpdate = false;
//bool AudioStream_F32::enableUpdateAll = false;
int AudioStream_F32::numInstances = 0;
AudioStream_F32* AudioStream_F32::allInstances[AudioStream_F32::maxInstanceCounting];
bool AudioStream_F32::isAudioProcessing = false;

uint32_t AudioStream_F32::update_counter = 0;



void AudioMemory_F32(const unsigned int num) {
	//audio_block_f32_t *data_f32 = allocate_f32_memory(num);
	//if (data_f32 != NULL) AudioStream_F32::initialize_f32_memory(data_f32, num);
	AudioStream_F32::initialize_f32_memory(num);
}
void AudioMemory_F32(const unsigned int num, const AudioSettings_F32 &settings) {
	//audio_block_f32_t *data_f32 = allocate_f32_memory(num, settings);
	//if (data_f32 != NULL) AudioStream_F32::initialize_f32_memory(data_f32, num, settings);
	AudioStream_F32::initialize_f32_memory(num, settings);
}
void AudioMemory_F32(const int num) { 
	if (num < 0) return; 
	AudioMemory_F32( (unsigned int) num); 
}
void AudioMemory_F32(const int num, const AudioSettings_F32 &settings) { 
	if (num < 0) return; 
	AudioMemory_F32( (unsigned int) num, settings); 
}


void AudioStream_F32::allocate_f32_memory(const unsigned int num, const AudioSettings_F32 &settings) {

	int fail_count = 0;
	for (unsigned int i=0; i < num; i++) {
		if (i < f32_memory_pool.size()) {
			//there is already a memory block in the vector
			if (f32_memory_pool[i]->full_length >= settings.audio_block_samples) {
				//the existing block is not big enough, so delete and then re-allocate
				delete f32_memory_pool[i]; //delete the audio_block_f32_t that was here
				f32_memory_pool[i] = new audio_block_f32_t(settings); //create a new one
				if (f32_memory_pool[i]->data == NULL) fail_count++;
			} else {
				//the existing block is big enough, so there is nothing to do
			}
		} else {	
			//there is no existing memory block, so allocate a new one
			f32_memory_pool.push_back(new audio_block_f32_t(settings));
			if (f32_memory_pool[f32_memory_pool.size()-1]->data == NULL) fail_count++;
		}
	}
	if (fail_count>0) Serial.println("AudioStream_F32::allocate_f32_memory: *** ERROR ***: Failed to allocate " + String(fail_count) + " blocks of audio memory (out of " + String(num) + ").");
}

// Allocate and set up the pool of audio data blocks
// placing them all onto the free list
//void AudioStream_F32::initialize_f32_memory(audio_block_f32_t *data, unsigned int num)
void AudioStream_F32::initialize_f32_memory(const unsigned int num)
{
	AudioSettings_F32 foo_settings;
	AudioStream_F32::initialize_f32_memory(num, foo_settings);
}
void AudioStream_F32::initialize_f32_memory(const unsigned int _num, const AudioSettings_F32 &settings)
{
	unsigned int num = _num;
	if (num > 192) num = 192;
	__disable_irq();
	
	allocate_f32_memory(num, settings);
  
	unsigned int i;

	//f32_memory_pool = data;  //already dealt with via allocated_f32_memory() method
	for (i=0; i < 6; i++) {
		f32_memory_pool_available_mask[i] = 0;
	}
	for (i=0; i < num; i++) {
		f32_memory_pool_available_mask[i >> 5] |= (1 << (i & 0x1F));
	}
	for (i=0; i < num; i++) {
		//data[i].memory_pool_index = i;
		f32_memory_pool[i]->memory_pool_index = i;
	}
	__enable_irq();
}



// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_block_f32_t * AudioStream_F32::allocate_f32(void)
{
  uint32_t n, index, avail;
  uint32_t *p;
  audio_block_f32_t *block;
  uint8_t used;

  p = f32_memory_pool_available_mask;
  __disable_irq();
  do {
    avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    p++; avail = *p; if (avail) break;
    __enable_irq();
    //Serial.println("alloc_f32:null");
    return NULL;
  } while (0);
  n = __builtin_clz(avail);
  *p = avail & ~(0x80000000 >> n);
  used = f32_memory_used + 1;
  f32_memory_used = used;
  __enable_irq();
  index = p - f32_memory_pool_available_mask;
  //block = f32_memory_pool + ((index << 5) + (31 - n));
  block = f32_memory_pool[(index << 5) + (31 - n)];
  block->ref_count = 1;
  if (used > f32_memory_used_max) f32_memory_used_max = used;
  //Serial.print("alloc_f32:");
  //Serial.println((uint32_t)block, HEX);
  return block;
}


// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream_F32::release(audio_block_f32_t *block)
{
  if (!block) return;  //return if block is NULL
  uint32_t mask = (0x80000000 >> (31 - (block->memory_pool_index & 0x1F)));
  uint32_t index = block->memory_pool_index >> 5;

  __disable_irq();
  if (block->ref_count > 1) {
    block->ref_count--;
  } else {
    //Serial.print("release_f32:");
    //Serial.println((uint32_t)block, HEX);
    f32_memory_pool_available_mask[index] |= mask;
    f32_memory_used--;
  }
  __enable_irq();
}

// Transmit an audio data block
// to all streams that connect to an output.  The block
// becomes owned by all the recepients, but also is still
// owned by this object.  Normally, a block must be released
// by the caller after it's transmitted.  This allows the
// caller to transmit to same block to more than 1 output,
// and then release it once after all transmit calls.
void AudioStream_F32::transmit(audio_block_f32_t *block, unsigned char index)
{
  //Serial.print("AudioStream_F32: transmit().  start...index = ");Serial.println(index);
  for (AudioConnection_F32 *c = destination_list_f32; c != NULL; c = c->next_dest) {
		//Serial.print("  : loop1, c->src_index = ");Serial.println(c->src_index);
    if (c->src_index == index) {
      if (c->dst->inputQueue_f32[c->dest_index] == NULL) {
        c->dst->inputQueue_f32[c->dest_index] = block;
        block->ref_count++;
        //Serial.print("  : block->ref_count = "); Serial.println(block->ref_count);
      }
    }
  } 
  //Serial.println("AudioStream_F32: transmit(). finished.");
}

// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_block_f32_t * AudioStream_F32::receiveReadOnly_f32(unsigned int index)
{
  audio_block_f32_t *in;

  if (index >= num_inputs_f32) return NULL;
  in = inputQueue_f32[index];
  inputQueue_f32[index] = NULL;
  return in;
}


// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_block_f32_t * AudioStream_F32::receiveWritable_f32(unsigned int index)
{
  audio_block_f32_t *in, *p;

  if (index >= num_inputs_f32) return NULL;
  in = inputQueue_f32[index];
  inputQueue_f32[index] = NULL;
  if (in && in->ref_count > 1) {
    p = allocate_f32();
    //if (p) memcpy(p->data, in->data, sizeof(p->data));
	if (p) memcpy(p->data, in->data, (p->full_length)*sizeof(p->data[0])); //revised 9/27/2023 as p->data is now allocated at runtime
	p->id = in->id; //copy over ID so that the new one is the same as the one on the original block.  added 1/13/2020
    in->ref_count--;
    in = p;
  }
  return in;
}

/**************************************************************************************/
// Constructor with no parameters: leave unconnected
AudioConnection_F32::AudioConnection_F32() 
	: src(NULL), dst(NULL),
	  src_index(0), dest_index(0),
	  isConnected(false)

{
	// we are unused right now, so
	// link ourselves at the start of the unused list
	next_dest = AudioStream_F32::unused_f32;
	AudioStream_F32::unused_f32 = this;
}


// Destructor
AudioConnection_F32::~AudioConnection_F32()
{
	AudioConnection_F32** pp;
	
	disconnect(); // disconnect ourselves: puts us on the unused list
	// Remove ourselves from the unused list
	pp = &AudioStream_F32::unused_f32;
	while (*pp && *pp != this)
		pp = &((*pp)->next_dest);
	if (*pp) // found ourselves
		*pp = next_dest; // remove ourselves from the unused list
}

/**************************************************************************************/


int AudioConnection_F32::connect(void) {
	int result = 1;
	AudioConnection_F32 *p;
	AudioConnection_F32 **pp;
	AudioStream_F32* s;

	do 
	{
		if (isConnected) // already connected
		{
			break;
		}
		
		if (!src || !dst) // NULL src or dst - [old] Stream object destroyed
		{
			result = 3;
			break;
		}
			
		if (dest_index >= dst->num_inputs_f32) // input number too high
		{
			result = 2;
			break;
		}
			
		__disable_irq();
		
		// First check the destination's input isn't already in use
		s = AudioStream_F32::first_update_f32; // first AudioStream in the stream list
		while (s) // go through all AudioStream objects
		{
			p = s->destination_list_f32;	// first patchCord in this stream's list
			while (p)
			{
				if (p->dst == dst && p->dest_index == dest_index) // same destination - it's in use!
				{
					__enable_irq();
					return 4;
				}
				p = p->next_dest;
			}
			s = s->next_update_f32;
		}
		
		// Check we're on the unused list
		pp = &AudioStream_F32::unused_f32;
		while (*pp && *pp != this)
		{
			pp = &((*pp)->next_dest);
		}
		if (!*pp) // never found ourselves - fail
		{
			result = 5;
			break;
		}
			
		// Now try to add this connection to the source's destination list
		p = src->destination_list_f32; // first AudioConnection
		if (p == NULL) 
		{
			src->destination_list_f32 = this;
		} 
		else 
		{
			while (p->next_dest)  // scan source Stream's connection list for duplicates
			{
				
				if (&p->src == &this->src && &p->dst == &this->dst
					&& p->src_index == this->src_index && p->dest_index == this->dest_index) 
				{
					//Source and destination already connected through another connection, abort
					__enable_irq();
					return 6;
				}
				p = p->next_dest;
			}			
			
			p->next_dest = this; // end of list, can link ourselves in
		}
		
		*pp = next_dest;  // remove ourselves from the unused list
		next_dest = NULL; // we're last in the source's destination list
		
		src->numConnections++;
		src->active = true;

		dst->numConnections++;
		dst->active = true;

		isConnected = true;
		
		result = 0;
	} while (0);
	
	__enable_irq();
	
	return result;
}


int AudioConnection_F32::connect(AudioStream_F32 &source, unsigned char sourceOutput,
		AudioStream_F32 &destination, unsigned char destinationInput)
{
	int result = 1;
	
	if (!isConnected)
	{
		src = &source;
		dst = &destination;
		src_index = sourceOutput;
		dest_index = destinationInput;
		
		result = connect();
	}
	return result;
}

int AudioConnection_F32::disconnect(void)
{
	AudioConnection_F32 *p;

	if (!isConnected) return 1;
	if (dest_index >= dst->num_inputs_f32) return 2; // should never happen!
	__disable_irq();
	
	// Remove destination from source list
	p = src->destination_list_f32;
	if (p == NULL) {
//>>> PAH re-enable the IRQ
		__enable_irq();
		return 3;
	} else if (p == this) {
		if (p->next_dest) {
			src->destination_list_f32 = next_dest;
		} else {
			src->destination_list_f32 = NULL;
		}
	} else {
		while (p)
		{
			if (p->next_dest == this) // found the parent of the disconnecting object
			{
				p-> next_dest = this->next_dest; // skip parent's link past us
				break;
			}
			else
				p = p->next_dest; // carry on down the list
		}
	}
//>>> PAH release the audio buffer properly
	//Remove possible pending src block from destination
	if(dst->inputQueue_f32[dest_index] != NULL) {
		AudioStream_F32::release(dst->inputQueue_f32[dest_index]);
		// release() re-enables the IRQ. Need it to be disabled a little longer
		__disable_irq();
		dst->inputQueue_f32[dest_index] = NULL;
	}

	//Check if the disconnected AudioStream objects should still be active
	src->numConnections--;
	if (src->numConnections == 0) {
		src->active = false;
	}

	dst->numConnections--;
	if (dst->numConnections == 0) {
		dst->active = false;
	}
	
	isConnected = false;
	next_dest = dst->unused_f32;
	dst->unused_f32 = this;

	__enable_irq();
	
	return 0;
}

AudioStream_F32 * AudioStream_F32::first_update_f32 = NULL;


/* void AudioStream_F32::printNextUpdatePointers(void) {
	AudioStream_F32 *p;

	int count=0;
	Serial.println("AudioStream_F32: printNextUpdatePointers:");
	
	for (p = first_update; p; p = p->next_update) {
		Serial.print("  : "); 
		Serial.print(count++); 
		Serial.print(", ");
		Serial.print(p->instanceName);
		if (p->active) {
			Serial.print(", Active. Next p = ");
			Serial.print((uint32_t)(p->next_update));
			Serial.println(", Done.");
		} else {			
			Serial.print(", Not Active, ");
			Serial.print(p->instanceName);
			Serial.print(", Next p = "); 
			Serial.print((uint32_t)(p->next_update));
			Serial.println(", Done.");					
		}
	}
	Serial.println("    : Done."); Serial.flush();
}	
 */

void AudioStream_F32::printAllInstances(void) {
	AudioStream_F32 *p;
	//AudioStream_F32 *p_next;
	
	Serial.print("AudioStream_F32: printAllInstances: "); Serial.print(numInstances); Serial.println("...");
	
	for (int i=0; i < numInstances; i++) {
		p = allInstances[i];
		Serial.print("    : ");
		Serial.print(i);
		Serial.print(", ");
		Serial.print((uint32_t)p);
		
		if ((uint32_t)p != 0) { 
			Serial.print(", ");
			Serial.print(p->instanceName);
			if (p->active) {
				Serial.print(", Active");
			} else {		
				Serial.print(", Not Active");
			}
			//Serial.print(", Next p = ");
			//p_next = p->next_update;
			//Serial.print((uint32_t)p_next);
			//if ((uint32_t)p_next != 0) {
			//	Serial.print(" = ");
			//	Serial.print(p_next->instanceName);
			//}
		} else {	
			Serial.print(", null p.");
		}
		Serial.println();
	}
	Serial.println("    : Done.");Serial.flush();
}