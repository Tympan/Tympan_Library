
#include "FFT_Overlapped_F32.h"

//output is via complex_2N_buffer
void FFT_Overlapped_F32::execute(audio_block_f32_t *block, float *complex_2N_buffer) //results returned inc omplex_2N_buffer
{
  int targ_ind;
  
  //get a pointer to the latest data
  //audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
  if (block == NULL) return;

  //Serial.println("FFT_Overlapped_F32: execute: N_BUFF_BLOCKS = " + String(N_BUFF_BLOCKS) + ", audio_block_samples = " + String(audio_block_samples));

  //shuffle all of input data blocks in preperation for this latest processing
  float32_t *tmp_block = buff_blocks[0]; //hold onto this for re-use in a moment
  for (int i = 0; i < (N_BUFF_BLOCKS-1); i++) buff_blocks[i] = buff_blocks[i+1];  //shuffle new blocks to be older blocks
  buff_blocks[N_BUFF_BLOCKS-1] = tmp_block; //put the pointer to the first block back at the end for re-use

  //copy the latest data into the last block in the buffer
  //for (int j = 0; j < audio_block_samples; j++) buff_blocks[N_BUFF_BLOCKS-1]->data[j] = block->data[j];
  for (int j = 0; j < audio_block_samples; j++) buff_blocks[N_BUFF_BLOCKS-1][j] = block->data[j];

	
  //aggregate all input data blocks into one long (complex valued) data vector...the long vector is interleaved [real,imaginary]
  targ_ind = 0;
  for (int i = 0; i < N_BUFF_BLOCKS; i++) {
    for (int j = 0; j < audio_block_samples; j++) {
      //complex_2N_buffer[2*targ_ind] = buff_blocks[i]->data[j];  //real
      complex_2N_buffer[2*targ_ind] = buff_blocks[i][j];  //real
	  complex_2N_buffer[2*targ_ind+1] = 0.0f;  //imaginary
      targ_ind++;
    }
  }
  //call the FFT...windowing of the data happens in the FFT routine, if configured
  myFFT.execute(complex_2N_buffer);
}

//output is via out_block
void IFFT_Overlapped_F32::execute(float *complex_2N_buffer, audio_block_f32_t *out_block) { //real results returned through audio_block_f32_t
 
  if (out_block == NULL) return;
  
 
  //Serial.println("IFFT_Overlapped_F32: execute: N_BUFF_BLOCKS = " + String(N_BUFF_BLOCKS) + ", audio_block_samples = " + String(audio_block_samples));
 
  //call the IFFT...any follow-up windowing is handdled in the IFFT routine, if configured
  myIFFT.execute(complex_2N_buffer);
  
  //prepare for the overlap-and-add for the output..shuffle the memory
  //audio_block_f32_t *temp_buff = buff_blocks[0]; //hold onto this one for a moment...it'll get overwritten later
  float32_t *temp_buff = buff_blocks[0]; //hold onto this one for a moment...it'll get overwritten later
  for (int i = 0; i < (N_BUFF_BLOCKS-1); i++) buff_blocks[i] = buff_blocks[i+1]; //shuffle the output data blocks
  buff_blocks[N_BUFF_BLOCKS-1] = temp_buff; //put the oldest output buffer back in the list

  //do overlap and add with previously computed data
  int output_count = 0;
  for (int i = 0; i < (N_BUFF_BLOCKS-1); i++) { //Notice that this loop does NOT do the last block.  That's a special case after.
	for (int j = 0; j < audio_block_samples; j++) {
		//(buff_blocks[i]->data[j]) +=  complex_2N_buffer[2*output_count]; //add only the real part into the previous results
		buff_blocks[i][j] +=  complex_2N_buffer[2*output_count]; //add only the real part into the previous results
		output_count++;
	}
  }

  //now write in the newest data into the last block of the buffer, overwriting any garbage that might have existed there
  for (int j = 0; j < audio_block_samples; j++) {
	//buff_blocks[(N_BUFF_BLOCKS - 1)]->data[j] = complex_2N_buffer[2*output_count]; //overwrite with the newest data
	buff_blocks[N_BUFF_BLOCKS-1][j] = complex_2N_buffer[2*output_count]; //overwrite with the newest data
	output_count++;
  }
 
  //finally, copy out the oldest (now complete) buffered data as our output
  //for (int j = 0; j < audio_block_samples; j++) out_block->data[j] = buff_blocks[0]->data[j]; //original
  for (int j = 0; j < audio_block_samples; j++) out_block->data[j] = buff_blocks[0][j];         //original for float32_t blocks
  
};

