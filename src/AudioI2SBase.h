/*
	AudioI2SBase

	Created: Chip Audette, Open Audio
	Purpose: Base class to be inherited by all I2S classes.  
	    Holds config values that must always be the same across
			I2S input/output classes.

	License: MIT License.  Use at your own risk.
 */

#ifndef audioI2SBase_h_
#define audioI2SBase_h_

#include <Arduino.h>
#include <arm_math.h>
#include <DMAChannel.h>
#include <AudioStream_F32.h>   //tympan library

#define AUDIO_INPUT_I2S_MAX_CHAN (6)
#define AUDIO_OUTPUT_I2S_MAX_CHAN (4)

//Base class to hold items that should be common to all I2S classes
class AudioI2SBase {
	//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
	public:
		AudioI2SBase(void) {};
		AudioI2SBase(const AudioSettings_F32 &settings) 
		{
			sample_rate_Hz = settings.sample_rate_Hz;
			audio_block_samples = settings.audio_block_samples;
		}
		virtual ~AudioI2SBase() {};
		
		//helper functions to convert number formats
		static void scale_i16_to_f32( float32_t *p_i16, float32_t *p_f32, int len);
		static void scale_i24_to_f32( float32_t *p_i24, float32_t *p_f32, int len);
		static void scale_i32_to_f32( float32_t *p_i32, float32_t *p_f32, int len);
		static void scale_f32_to_i16( float32_t *p_f32, float32_t *p_i16, int len) ;
		static void scale_f32_to_i24( float32_t *p_f32, float32_t *p_i16, int len) ;
		static void scale_f32_to_i32( float32_t *p_f32, float32_t *p_i32, int len) ;
		
		//other methods common to all I2S classes
		static uint32_t get_i2sBytesPerSample(void) { return (transferUsing32bit ? 4 : 2); }
		
	protected:
		static bool transferUsing32bit;  //I2S transfers using 32 bit values (instead of 16-bit).  See instantiation in the cpp file  for the default and see begin() and config_i2s() for changes
		static float sample_rate_Hz;    
		static int audio_block_samples; 
		volatile uint8_t enabled = 1; //should not be static, each instance should have its own

	private:

};

// Some constants for this header
#define I2S_BUFFER_TYPE uint32_t
#define BYTES_PER_I2S_BUFFER_ELEMENT (sizeof(I2S_BUFFER_TYPE)) //assumes the rx buffer is made up of uint32_t
//#define LEN_BIG_BUFFER (MAX_AUDIO_BLOCK_SAMPLES_F32 * NUM_CHAN_TRANSFER * MAX_BYTES_PER_SAMPLE / BYTES_PER_BIG_BUFF_ELEMENT)

// ///////////////////////////////////////////////////
class AudioInputI2SBase_F32 : public AudioI2SBase, public AudioStream_F32 {
	//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
	public:
		AudioInputI2SBase_F32(int _n_chan) : AudioI2SBase(), AudioStream_F32(0, nullptr) 
		{ 
			set_n_chan(_n_chan);
			initPointers(); 
		};
		AudioInputI2SBase_F32(int _n_chan, const AudioSettings_F32 &settings) :	AudioI2SBase(settings), AudioStream_F32(0, nullptr)
		{ 
			set_n_chan(_n_chan);
			initPointers(); 
		};
		virtual ~AudioInputI2SBase_F32(void) {};
		
		static void initPointers(void) {
			for (int i=0; i<AUDIO_INPUT_I2S_MAX_CHAN; i++) block_ch[i]=nullptr;  
			block_offset = 0;
		}
		
		virtual void begin(void) = 0;
		virtual void update(void) override;

		static int set_n_chan(int _n_chan) { return n_chan = max(0,min(AUDIO_INPUT_I2S_MAX_CHAN,_n_chan)); }
		static int get_isOutOfMemory(void) { return flag_out_of_memory; }
		static void clear_isOutOfMemory(void) { flag_out_of_memory = 0; }
	
	protected:
		static int flag_out_of_memory;
		static unsigned long update_counter;
		static int n_chan;
		static bool update_responsibility;
		static DMAChannel dma;
		static uint32_t *i2s_rx_buffer; 
		//
		static audio_block_f32_t *block_ch[AUDIO_INPUT_I2S_MAX_CHAN];
		static uint16_t block_offset;
		//
		static void isr(void);
		virtual void update_1chan(int chan, unsigned long counter, audio_block_f32_t *&out_block);
	
		//Compute how much of the i2s buffer we're actually using, which depends upon whether we're doing 32-bit or 16-bit transfers
		static uint32_t get_i2sBufferToUse_bytes(void) { return (audio_block_samples * n_chan * get_i2sBytesPerSample()); }
		static uint32_t get_i2sBufferMidPoint_index(void) { return (get_i2sBufferToUse_bytes() / 2 / BYTES_PER_I2S_BUFFER_ELEMENT); }
	
	private:
};



// ///////////////////////////////////////////
class AudioOutputI2SBase_F32 : public AudioI2SBase, public AudioStream_F32 {
	//GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
	public:
		AudioOutputI2SBase_F32(int _n_chan) : 
				AudioI2SBase(), 
				AudioStream_F32(AUDIO_OUTPUT_I2S_MAX_CHAN, inputQueueArray)
		{ 
			set_n_chan(_n_chan);
			initPointers(); 
		};
		AudioOutputI2SBase_F32(int _n_chan, const AudioSettings_F32 &settings) :	
				AudioI2SBase(settings), 
				AudioStream_F32(AUDIO_OUTPUT_I2S_MAX_CHAN, inputQueueArray)
		{	
			set_n_chan(_n_chan);
			initPointers(); 
		};

		virtual ~AudioOutputI2SBase_F32(void) {};
		
		static void initPointers(void) {
			for (int i=0; i<AUDIO_OUTPUT_I2S_MAX_CHAN; i++) {	
				block_1st[i]=nullptr;  block_2nd[i]=nullptr; block_offset[i] = 0;
			}
		}

		virtual void begin(void) = 0;
		void update(void) override;
		
		static int set_n_chan(int _n_chan) { return n_chan = max(0,min(AUDIO_OUTPUT_I2S_MAX_CHAN,_n_chan)); }
		
	protected:
		audio_block_f32_t *inputQueueArray[AUDIO_OUTPUT_I2S_MAX_CHAN];
		static int n_chan;
		static bool update_responsibility;
		static DMAChannel dma;
		static I2S_BUFFER_TYPE *i2s_tx_buffer;
		static float32_t *zerodata;
		//
		static audio_block_f32_t *block_1st[AUDIO_OUTPUT_I2S_MAX_CHAN];
		static audio_block_f32_t *block_2nd[AUDIO_OUTPUT_I2S_MAX_CHAN];
		static uint16_t block_offset[AUDIO_OUTPUT_I2S_MAX_CHAN];
		//
		static void isr(void);
		static void isr_shuffleDataBlocks(audio_block_f32_t *&, audio_block_f32_t *&, uint16_t &);
		void update_1chan(const int, audio_block_f32_t *&, audio_block_f32_t *&, uint16_t &);

		//Compute how much of the i2s buffer we're actually using, which depends upon whether we're doing 32-bit or 16-bit transfers
		static uint32_t get_i2sBufferToUse_bytes(void) { return (audio_block_samples * n_chan * get_i2sBytesPerSample()); }
		static uint32_t get_i2sBufferMidPoint_index(void) { return (get_i2sBufferToUse_bytes() / 2 / BYTES_PER_I2S_BUFFER_ELEMENT); }
	
};



#endif
