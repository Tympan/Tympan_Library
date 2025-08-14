

#include <AudioI2SBase.h> //tympan library


// define static data members...AudioI2SBase
float AudioI2SBase::sample_rate_Hz = AUDIO_SAMPLE_RATE;               //set to default value.  Will probably be overwritten by the constructor
int AudioI2SBase::audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;  //set to default value.  Will probably be overwritten by the constructor
static float32_t   default_zerodata[MAX_AUDIO_BLOCK_SAMPLES_F32] = {0}; //modified for F32.  Need zeros for half an audio block...do we need a whole block or just half a block?


// define static data members...AudioInputI2SBase_F32
audio_block_f32_t* AudioInputI2SBase_F32::block_ch[AUDIO_INPUT_I2S_MAX_CHAN] = {};
uint16_t           AudioInputI2SBase_F32::block_offset = 0;
int                AudioInputI2SBase_F32::n_chan = 0;  //overwritten in constructor
uint32_t*          AudioInputI2SBase_F32::i2s_rx_buffer = nullptr;
bool               AudioInputI2SBase_F32::update_responsibility = false;
DMAChannel         AudioInputI2SBase_F32::dma(false);
unsigned long      AudioInputI2SBase_F32::update_counter = 0;
int                AudioInputI2SBase_F32::flag_out_of_memory = 0;


// define static data members...AudioOutputI2SBase_F32
audio_block_f32_t* AudioOutputI2SBase_F32::block_1st[AUDIO_OUTPUT_I2S_MAX_CHAN] = {};
audio_block_f32_t* AudioOutputI2SBase_F32::block_2nd[AUDIO_OUTPUT_I2S_MAX_CHAN] = {};
uint16_t           AudioOutputI2SBase_F32::block_offset[AUDIO_OUTPUT_I2S_MAX_CHAN] = {};
int                AudioOutputI2SBase_F32::n_chan = 0;  //overwritten in constructor
uint32_t*          AudioOutputI2SBase_F32::i2s_tx_buffer = nullptr;
bool               AudioOutputI2SBase_F32::update_responsibility = false;
float32_t*         AudioOutputI2SBase_F32::zerodata = default_zerodata;
DMAChannel         AudioOutputI2SBase_F32::dma(false);




#ifdef __IMXRT1062__
  bool AudioI2SBase::transferUsing32bit = true;  //Our setup for I2S and DMA for Teensy 4.x defaults to 32-bit (I'm not sure all I2S classes can do 16-bit for Teensy 4.  Sorry)
#else
  bool AudioI2SBase::transferUsing32bit = false;  //Our setup for I2S and DMA for Teensy 3.x is only for 16 bits (I think)
#endif	

#define I16_TO_F32_NORM_FACTOR (3.051850947599719e-05)  //which is 1/32767 
void AudioI2SBase::scale_i16_to_f32( float32_t *p_i16, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i16++) * I16_TO_F32_NORM_FACTOR); }
}
#define I24_TO_F32_NORM_FACTOR (1.192093037616377e-07)   //which is 1/(2^23 - 1)
void AudioI2SBase::scale_i24_to_f32( float32_t *p_i24, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i24++) * I24_TO_F32_NORM_FACTOR); }
}
#define I32_TO_F32_NORM_FACTOR (4.656612875245797e-10)   //which is 1/(2^31 - 1)
void AudioI2SBase::scale_i32_to_f32( float32_t *p_i32, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) { *p_f32++ = ((*p_i32++) * I32_TO_F32_NORM_FACTOR); }
}

#define F32_TO_I16_NORM_FACTOR (32767)   //which is 2^15-1
void AudioI2SBase::scale_f32_to_i16(float32_t *p_f32, float32_t *p_i16, int len) {
	for (int i=0; i<len; i++) { *p_i16++ = max(-F32_TO_I16_NORM_FACTOR,min(F32_TO_I16_NORM_FACTOR,(*p_f32++) * F32_TO_I16_NORM_FACTOR)); }
}
#define F32_TO_I24_NORM_FACTOR (8388607)   //which is 2^23-1
void AudioI2SBase::scale_f32_to_i24( float32_t *p_f32, float32_t *p_i24, int len) {
	for (int i=0; i<len; i++) { *p_i24++ = max(-F32_TO_I24_NORM_FACTOR,min(F32_TO_I24_NORM_FACTOR,(*p_f32++) * F32_TO_I24_NORM_FACTOR)); }
}
#define F32_TO_I32_NORM_FACTOR (2147483647)   //which is 2^31-1
void AudioI2SBase::scale_f32_to_i32( float32_t *p_f32, float32_t *p_i32, int len) {
	for (int i=0; i<len; i++) { *p_i32++ = max(-F32_TO_I32_NORM_FACTOR,min(F32_TO_I32_NORM_FACTOR,(*p_f32++) * F32_TO_I32_NORM_FACTOR)); }
	//for (int i=0; i<len; i++) { *p_i32++ = (*p_f32++) * F32_TO_I32_NORM_FACTOR + 512.f*8388607.f; }
}


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Methods specific to I2S INPUT 
//
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void AudioInputI2SBase_F32::isr(void)
{
	uint32_t daddr;
	const int32_t *src32=nullptr;  //*end;
	const int16_t *src16=nullptr;  //*end;
	uint32_t offset;
	float32_t *dest_f32[n_chan];

	//update the dma 
	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();
	
	//get pointer for the destination rx buffer
	if (transferUsing32bit) {
		//32-bit transfers
		if (daddr < ((uint32_t)i2s_rx_buffer + get_i2sBufferMidPoint_index())) { //we're copying half an audio block times four channels. One address is already 4bytes, like our 32-bit audio sampels
			// DMA is receiving to the first half of the buffer.  Need to remove data from the second half
			src32 = (int32_t *)&i2s_rx_buffer[get_i2sBufferMidPoint_index()]; //For 32-bit transfers...we are transfering half an audio block and 4 channels each		
			if (update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer. Need to remove data from the first half
			src32 = (int32_t *)&i2s_rx_buffer[0];  //for 32-bit transfers
		}
		
	} else {
		//16-bit transfers
		if (daddr < ((uint32_t)i2s_rx_buffer + get_i2sBufferMidPoint_index())) { //we're copying half an audio block times four channels. But, one address is 4bytes whereas our data is only 2bytes, so divide by two
			// DMA is receiving to the first half of the buffer. Need to remove data from the second half
			src16 = (int16_t *)&i2s_rx_buffer[get_i2sBufferMidPoint_index()]; //For 16-bit transfers...we are transfering half an audio block and 4 channels each...divided by two because two samples fit in each 32-bit slot
			if (update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is receiving to the second half of the buffer. Need to remove data from the first half
			src16 = (int16_t *)&i2s_rx_buffer[0];  //for 16bit transfers
		}
	} 
	
	//check to see if data is available
	bool all_data_is_available = true;
	for (int Ichan=0; Ichan < n_chan; Ichan++) if (block_ch[Ichan]==nullptr) all_data_is_available = false;
	if (all_data_is_available) {
		//De-interleave and copy to destination audio buffers.  
		offset = block_offset;
		if (offset <= (uint32_t)(audio_block_samples/2)) {  //transfering half of an audio_block_samples at a time
			//This block of code only copies the data into F32 buffers but leaves 
			// the scaling at +/- 2^(32-1) scaling (or at +/- 2^(16-1) for 16-bit transfers)
			//which will then be scaled in the update() method instead of here
			block_offset = offset + audio_block_samples/2;
			if (n_chan == 2) {
				//channels are ordered as expected
				dest_f32[1-1] = &(block_ch[1-1]->data[offset]);
				dest_f32[2-1] = &(block_ch[2-1]->data[offset]);
			} else if (n_chan == 4) {
				//Note the unexpected order for 4-channel !!! Destination is chan 1, 3, 2, 4
				dest_f32[1-1] = &(block_ch[1-1]->data[offset]);
				dest_f32[2-1] = &(block_ch[3-1]->data[offset]); //unexpected order!
				dest_f32[3-1] = &(block_ch[2-1]->data[offset]); //unexpected order!
				dest_f32[4-1] = &(block_ch[4-1]->data[offset]);
			} else if (n_chan == 6) {
				//Note the unexpected order for 6-channel !!! Destination is chan 1, 3, 5, 2, 4, 6
				dest_f32[1-1] = &(block_ch[1-1]->data[offset]);
				dest_f32[2-1] = &(block_ch[3-1]->data[offset]); //unexpected order!
				dest_f32[3-1] = &(block_ch[5-1]->data[offset]); //unexpected order!
				dest_f32[4-1] = &(block_ch[2-1]->data[offset]);	//unexpected order!			
				dest_f32[5-1] = &(block_ch[4-1]->data[offset]);	//unexpected order!				
				dest_f32[6-1] = &(block_ch[6-1]->data[offset]);					
			} else {
				//should never be here
				Serial.println("AudioInputI2SBase_F32: ***ERROR***: n_chan = " + String(n_chan) + " which is unexpected");
				for (int Ichan=0; Ichan < n_chan; Ichan++) dest_f32[Ichan] = &(block_ch[Ichan]->data[offset]);
			}	

			//copy the data values! (32bit or 16bit, as specified)
			if ((transferUsing32bit) && (src32 != nullptr)) {
				//32-bits per audio sample
				arm_dcache_delete((void*)src32, get_i2sBufferToUse_bytes()/2); //ensure cache is up-to-date
				for (int i=0; i < audio_block_samples/2; i++) {
					for (int Ichan=0; Ichan < n_chan; Ichan++) { *(dest_f32[Ichan])++ = ((float32_t) *src32++); }
				}
			} else if ((transferUsing32bit==false) && (src16 != nullptr)) {
				//16-bits per audio sample
				arm_dcache_delete((void*)src16, get_i2sBufferToUse_bytes()/2); //ensure cache is up-to-date
				for (int i=0; i < audio_block_samples/2; i++) {
					for (int Ichan=0; Ichan < n_chan; Ichan++) { *(dest_f32[Ichan])++ = ((float32_t) *src16++); }
				}
			} else {	
				//should never be here..but let's be defensive just in case and copy in zeros
				for (int i=0; i < audio_block_samples/2; i++) { 
					for (int Ichan=0; Ichan < n_chan; Ichan++) { *(dest_f32[Ichan])++ = 0.0f; };
				}
			}
		}		
	}
}



void AudioInputI2SBase_F32::update(void)
{
	audio_block_f32_t *new_block[n_chan];
	audio_block_f32_t *out_block[n_chan];

	// allocate new blocks
	bool all_allocated = true;
	for (int Ichan=0; Ichan<n_chan; Ichan++) {
		new_block[Ichan] = AudioStream_F32::allocate_f32();
		if (new_block[Ichan] == nullptr) all_allocated = false;
	}
	if (all_allocated == false) {
		//Serial.println("AudioInputI2SBase_F32: out of memory.");
		flag_out_of_memory = 1;
		for (int Ichan=0; Ichan<n_chan; Ichan++) {
			AudioStream_F32::release(new_block[Ichan]);
			new_block[Ichan]=nullptr;
		}
	}
	
	// switch the data blocks being filled by the DMA
	__disable_irq();
	if (block_offset >= (uint32_t)audio_block_samples) {
		// the DMA filled 4 blocks, so grab them and get the
		// 4 new blocks to the DMA, as quickly as possible
		for (int Ichan=0; Ichan<n_chan; Ichan++) {
			out_block[Ichan] = block_ch[Ichan];
			block_ch[Ichan] = new_block[Ichan];
		}
		block_offset = 0;
		__enable_irq();
		
		//service the data that we just got out of the DMA
		update_counter++;
		for (int Ichan=0; Ichan<n_chan; Ichan++) update_1chan(Ichan, update_counter, out_block[Ichan]);
		
	} else if (new_block[0] != nullptr) {
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_ch[0] == nullptr) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			for (int Ichan=0; Ichan<n_chan; Ichan++) block_ch[Ichan]=new_block[Ichan];
			block_offset = 0;
			__enable_irq();
		} else {
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			for (int Ichan=0; Ichan<n_chan; Ichan++) AudioStream_F32::release(new_block[Ichan]);
		}
	} else {
		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
}


void AudioInputI2SBase_F32::update_1chan(int chan, unsigned long counter, audio_block_f32_t *&out_block) {
	if (!out_block) return;
		
	//incoming data is still scaled like int16 (so, +/-32767.).  Here we need to re-scale
	//the values so that the maximum possible audio values spans the F32 stadard of +/-1.0
	if (transferUsing32bit) {
		scale_i32_to_f32(out_block->data, out_block->data, audio_block_samples);   //for 32-bit transfers.  static method in AudiOI2SBase
	} else {
		scale_i16_to_f32(out_block->data, out_block->data, audio_block_samples); //for 16-bit transfers.  static method in AudiOI2SBase
	}
	
	//prepare to transmit by setting the update_counter (which helps tell if data is skipped or out-of-order)
	out_block->id = counter;
	out_block->length = audio_block_samples;

	// then transmit and release the DMA's former blocks
	AudioStream_F32::transmit(out_block, chan);
	AudioStream_F32::release(out_block);
}


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Methods specific to I2S OUTPUT 
//
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//define the isr that gets called every time the DMA is filling up
void AudioOutputI2SBase_F32::isr(void)
{
	uint32_t saddr;
	float32_t *src[n_chan];
	float32_t *zeros = (float32_t *)zerodata;
	int32_t *dest32=nullptr; //used for 32 bit transfers
	int16_t *dest16=nullptr; //used for 16 bit transfers
	
	//update the dma and get pointer for the destination tx buffer
	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	
	//prepare to transfer data to the buffer
	if (transferUsing32bit) {
		if (saddr < ((uint32_t)i2s_tx_buffer + get_i2sBufferMidPoint_index())) {  //we're copying half an audio block times four channels.  One address is already 4bytes, like our 32-bit audio sampels
			// DMA is transmitting the first half of the buffer so, here in this isr(), we must fill the second half
			dest32 = (int32_t *)&i2s_tx_buffer[get_i2sBufferMidPoint_index()]; 
			if (update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is transmitting the second half of the buffer so, here in this isr(), we must fill the first half
			dest32 = (int32_t *)i2s_tx_buffer;  //start of the TX buffer, 32-bit transfers
		}
		
	} else {	
		if (saddr < ((uint32_t)i2s_tx_buffer + get_i2sBufferMidPoint_index())) {  //we're copying half an audio block times four channels.  One address is 4bytes, but our samples are 2bytes, so divide by 2
			// DMA is transmitting the first half of the buffer so, here in this isr(), we must fill the second half
			dest16 = (int16_t *)&i2s_tx_buffer[get_i2sBufferMidPoint_index()]; 
			if (update_responsibility) AudioStream_F32::update_all();
		} else {
			// DMA is transmitting the second half of the buffer so, here in this isr(), we must fill the first half
			dest16 = (int16_t *)i2s_tx_buffer;  //start of the TX buffer, 16-bit transfers
		}
	}

	//get pointers for source data that we will copy into the tx buffer
	if (n_chan==2) {
		//left and right are in the order expected.  So, just copy the pointers over in the same order that we have them
		for (int Ichan=0; Ichan<n_chan; Ichan++) src[Ichan] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
	} else if (n_chan==4) {
		//The order is not the one expected! We need to copy the pointers over in a different order.
		//We need left 1 (src[0]) then left 2 (src[2]) then right 1 (src[1]) then right 2 (src[3])
		int Ichan;
		Ichan=1-1; src[0] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
		Ichan=3-1; src[1] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
		Ichan=2-1; src[2] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
		Ichan=4-1; src[3] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
	} else {
		//we should never be here
		Serial.println("AudioOutputI2SBase: *** ERROR ***: n_chan is " + String(n_chan) + " which is greater than the max allowed (4)");
		for (int Ichan=0; Ichan<n_chan; Ichan++) src[Ichan] = (block_1st[Ichan]) ? (&(block_1st[Ichan]->data[block_offset[Ichan]])) : zeros; //get pointer to data array (float32) from the audio_block that is destined for ch1
	}

	//This block of code assumes that the audio data HAS ALREADY been scaled to +/- float32(2**31 - 1)
	//interleave the given source data into the output array
	if (transferUsing32bit) {
		int32_t *d = dest32;  //32 bit transfers
		if (d != nullptr) {
			for (int i=0; i < audio_block_samples / 2; i++) { //copying half the buffer
  			//the src pointers have already been re-ordered earlier
				for (int Ichan=0; Ichan < n_chan; Ichan++) *d++ = (int32_t)(*(src[Ichan])++); //...retrieve scaled f32 value, cast to int32 type, copy to destination
			}				
			arm_dcache_flush_delete(dest32, get_i2sBufferToUse_bytes() / 2);  //flush the cache for all the bytes that we filled (the /2 should be correct...we filled half the buffer)
		}
	} else {
		int16_t *d = dest16;  //16 bit transfers
		if (d != nullptr) {
			for (int i=0; i < audio_block_samples / 2; i++) {
				//the src pointers have already been re-ordered earlier
				for (int Ichan=0; Ichan < n_chan; Ichan++) *d++ = (int16_t)(*(src[Ichan])++);  //...retrieve scaled f32 value, cast to int16 type, copy to destination
			}	
			arm_dcache_flush_delete(dest16, get_i2sBufferToUse_bytes() / 2);  //flush the cache for all the bytes that we filled (the /2 should be correct...we filled half the buffer)
		}
	}

	//now, shuffle the 1st and 2nd data block for each channel
	for (int i=0; i<n_chan; i++) isr_shuffleDataBlocks(block_1st[i], block_2nd[i], block_offset[i]);
	
}  

void AudioOutputI2SBase_F32::isr_shuffleDataBlocks(audio_block_f32_t *&block_1st, audio_block_f32_t *&block_2nd, uint16_t &ch_offset)
{
	if (block_1st) {
		if (ch_offset == 0) {
			ch_offset = audio_block_samples/2;
		} else {
			ch_offset = 0;
			AudioStream_F32::release(block_1st);
			block_1st = block_2nd;
			block_2nd = nullptr;
		}
	}
}


 
void AudioOutputI2SBase_F32::update_1chan(const int chan,  //this is not changed upon return
		audio_block_f32_t *&block_1, audio_block_f32_t *&block_2, uint16_t &ch_offset) //all three of these are changed upon return  
{

	//get some working memory
	audio_block_f32_t *block_f32_scaled = AudioStream_F32::allocate_f32(); //allocate for scaled data (from F32 scale of +/- 1.0 to int16 scale of +/- 32767)
	if (block_f32_scaled==NULL) return;  //fail
	
	//Receive the incoming audio blocks
	audio_block_f32_t *block_f32 = receiveReadOnly_f32(chan); // get one channel

	//Is there any data?
	if (block_f32 == NULL) {
		//Serial.print("Output_i2s_quad: update: did not receive chan " + String(chan));
		
		//fill with zeros
		for (int i=0; i<audio_block_samples; i++) block_f32_scaled->data[i] = 0.0f;

	} else {
		//Process the given data	
		if (chan == 0) {
			if (block_f32->length != audio_block_samples) {
				Serial.print("AudioOutputI2SBase_F32: *** WARNING ***: audio_block says len = ");
				Serial.print(block_f32->length);
				Serial.print(", but I2S settings want it to be = ");
				Serial.println(audio_block_samples);
			}
		} 
		
		//scale the F32 data (+/- 1.0) to fit within Int data type, though we're still float32 data type
		if (transferUsing32bit) {
			scale_f32_to_i32(block_f32->data, block_f32_scaled->data, audio_block_samples); //assume Int32 (static method inherited from AudioI2SBase)
		} else {
			scale_f32_to_i16(block_f32->data, block_f32_scaled->data, audio_block_samples); //assume Int16 (static method inherited from AudioI2SBase)
		}	
		block_f32_scaled->length = block_f32->length;
		block_f32_scaled->id = block_f32->id;
		block_f32_scaled->fs_Hz  = block_f32->fs_Hz;
		
		//transmit and release the original (not scaled) audio data
		AudioStream_F32::transmit(block_f32, chan);
		AudioStream_F32::release(block_f32);
	}
	
	//shuffle the scaled data between the two buffers that the isr() routines looks for
	__disable_irq();
	if (block_1 == nullptr) {
		block_1 = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		ch_offset = 0;
		__enable_irq();
	} else if (block_2 == nullptr) {
		block_2 = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		__enable_irq();
	} else {
		audio_block_f32_t *tmp = block_1;
		block_1 = block_2;
		block_2 = block_f32_scaled; //here were are temporarily holding onto the working memory for use by the isr()
		ch_offset = 0;
		__enable_irq();
		AudioStream_F32::release(tmp);  //here we are releaseing an older audio buffer used as working memory
	}

}

//This routine receives an audio block of F32 data from audio processing
//routines.  This routine will receive one block from each audio channel.
//This routine must receive that data, scale it (not convert it) to I16 fullscale
//from F32 fullscale, and stage it (via the correct pointers) so that the isr
// can find all four audio blocks, cast them to I16, interleave them, and put them
//into the DMA that the I2S peripheral pulls from.
void AudioOutputI2SBase_F32::update(void)
{
	//update each channel independently
	for (int Ichan = 0; Ichan < n_chan; Ichan++) {
		update_1chan(Ichan, block_1st[Ichan], block_2nd[Ichan], block_offset[Ichan]);
	}
}
