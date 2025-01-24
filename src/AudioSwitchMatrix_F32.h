/*
 * AudioSwitchMatrix4_F32 (or AudioSwitchmatrix8_F32)
 *
 * Created: Chip Audette, OpenAudio, Sept 2024
 * Purpose: Switch 4 inputs among 4 outputs (or 8 and 8)    
 *          
 * MIT License.  use at your own risk.
*/

#ifndef AUDIOSWITCHMATRIX_F32_H
#define AUDIOSWITCHMATRIX_F32_H

#include "AudioStream_F32.h"

class AudioSwitchMatrix4_F32 : public AudioStream_F32 {
	//GUI: inputs:4, outputs:4  //this line used for automatic generation of GUI node
	//GUI: shortName:SwitchMtrx4
	public:
		AudioSwitchMatrix4_F32() : AudioStream_F32(4, inputQueueArray) { setDefaultValues(); }
		AudioSwitchMatrix4_F32(const AudioSettings_F32 &settings) : AudioStream_F32(4, inputQueueArray) { setDefaultValues(); }
		void setDefaultValues(void) {
			for (int i=0; i < max_n_chan; i++) setInputToOutput(i,i); //default to straight through...output 0 uses input 0, output 1 uses input 1, etc
		}
		virtual void update(void);

		// For a given output channel, set the input chan to negative to mute the output.
		// To mute an input, go through each output and ensure that the input isn't connected.
		// You can route an input to multiple outputs.
		// Each output can only have 1 input (it will use the most recently specified input)
		int setInputToOutput(int input_chan, int output_chan) {
			if ((output_chan >= 0) && (output_chan < max_n_chan)) {
				return inputForEachOutput[output_chan] = input_chan;
			}
			return -1;
		}

	private:
		const int max_n_chan = 4;
		audio_block_f32_t *inputQueueArray[4] = {};
		int inputForEachOutput[4];  //inputForEachOutput[0] is the input for output[0], inputForEachOutput[1] is the input for output[1], etc
};

class AudioSwitchMatrix8_F32 : public AudioStream_F32 {
	//GUI: inputs:8, outputs:8  //this line used for automatic generation of GUI node
	//GUI: shortName:SwitchMtrx8
	public:
		AudioSwitchMatrix8_F32() : AudioStream_F32(8, inputQueueArray) { setDefaultValues(); }
		AudioSwitchMatrix8_F32(const AudioSettings_F32 &settings) : AudioStream_F32(8, inputQueueArray) { setDefaultValues(); }
		
		void setDefaultValues(void) {
			for (int i=0; i < max_n_chan; i++) setInputToOutput(i,i); //default to straight through...output 0 uses input 0, output 1 uses input 1, etc
		}

		virtual void update(void);
	
		// For a given output channel, set the input chan to negative to mute the output.
		// To mute an input, go through each output and ensure that the input isn't connected.
		// You can route an input to multiple outputs.
		// Each output can only have 1 input (it will use the most recently specified input)
		int setInputToOutput(int input_chan, int output_chan) {
			if ((output_chan >= 0) && (output_chan < max_n_chan)) {
				return inputForEachOutput[output_chan] = input_chan;
			}
			return -1;
		}

	private:
		const int max_n_chan = 8;
		audio_block_f32_t *inputQueueArray[8] = {};
		int inputForEachOutput[8];  //inputForEachOutput[0] is the input for output[0], inputForEachOutput[1] is the input for output[1], etc
};

#endif
