#include <arm_math.h>
#include "synth_waveform_F32.h"

void AudioSynthWaveform_F32::update(void) {
	if (active) {  // active is in AudioStream.h.  It turns out that update() will never get called if active is false.  So, this "if" should be redundent
		//Serial.println("AudioSynthWaveform_F32: update()");
		block_counter++;
		
		//get input block (the modulation of the frequency)
		audio_block_f32_t *lfo_block = receiveReadOnly_f32(0);
		
		//get output block
		audio_block_f32_t *block_new = allocate_f32();
		if (!block_new) { AudioStream_F32::release(lfo_block); return; } //could not allocate block.  So, release memory and return.
		
		//process the audio to fill the output block with data samples
		processAudioBlock(lfo_block, block_new);
		
		//updat ethe counter on the new block
		block_new->id = block_counter;

		AudioStream_F32::transmit(block_new);
		AudioStream_F32::release(block_new);
		AudioStream_F32::release(lfo_block);
	}
}


float AudioSynthWaveform_F32::calc_modulation(audio_block_f32_t *lfo, int length, audio_block_f32_t *dphase_rad_block) {
  const float32_t orig_frequency = _Frequency;  //save the original frequency, in case we need to restore it at the end of this function
  float32_t final_frequency = orig_frequency;

  //first, check to see if we can do the simplest case...no modulation
  if ((lfo == nullptr) && (_PortamentoSamples==0)) {
    //this is the samplest case!  no modulation.  constant phase increment, constant frequency
    for (int i = 0; i < length; i++) {  dphase_rad_block->data[i] = _PhaseIncrement; }; //fill with constant value
    return final_frequency;
  }

  //do the harder case and compute all the modulation
  const float32_t orig_phase_increment = _PhaseIncrement;  //save the original phase increment, in case we need to restore it at the end of this function
  for (int sample=0; sample<length; sample++) {
    if (_PortamentoSamples > 0 && _CurrentPortamentoSample++ < _PortamentoSamples) {  _Frequency +=_PortamentoIncrement; }
    float32_t osc_frequency = _Frequency;  //current frequency after portamento

    if (lfo && _PitchModAmt > 0.0f) {
      if (_ModMode == MOD_MODE_PER_OCT) {
        //input signal is assumed to be a pitch shift where 1.0 is one octave
        osc_frequency = _Frequency * powf(2.0f, 0.0f / 1200.0f + lfo->data[sample] * _PitchModAmt);
      } else {
        //input signal is assumed to be a frequency value in Hz
        osc_frequency = _Frequency + lfo->data[sample] * _PitchModAmt;
      }
    }

    _PhaseIncrement = osc_frequency * twoPI / sample_rate_Hz;
    dphase_rad_block->data[sample] = _PhaseIncrement;
  }
  final_frequency = _Frequency;  //save the final frequency after portamento modulation
  _Frequency = orig_frequency;  //restore the original frequency
  _PhaseIncrement = orig_phase_increment;  //restore the original phase increment...the modulated value is passed back via the array

  return final_frequency;
}


int AudioSynthWaveform_F32::processAudioBlock(audio_block_f32_t *lfo_block, audio_block_f32_t *block_new)  {
  audio_block_f32_t *all_dphase_rad_per_sample = allocate_f32();
    if (all_dphase_rad_per_sample == nullptr) { return -1; }  //could not allocate block.  So, release memory and return.
  float32_t final_freq_Hz = calc_modulation(lfo_block, audio_block_samples, all_dphase_rad_per_sample);

  switch (_OscillatorMode) {
    case OSCILLATOR_MODE_SINE:
        for (int i = 0; i < audio_block_samples; i++) {
          //applyMod(i, lfo_block);

          block_new->data[i] = arm_sin_f32(_Phase);
          //_Phase += _PhaseIncrement;
          _Phase += all_dphase_rad_per_sample->data[i];
          while (_Phase >= twoPI) { _Phase -= twoPI;  }
        }
        break;
    case OSCILLATOR_MODE_SAW:
        for (int i = 0; i < audio_block_samples; i++) {
          //applyMod(i, lfo_block);

          block_new->data[i] = 1.0f - (2.0f * _Phase / twoPI);
          //_Phase += _PhaseIncrement;
          _Phase += all_dphase_rad_per_sample->data[i];
          while (_Phase >= twoPI) { _Phase -= twoPI; }
        }
        break;
    case OSCILLATOR_MODE_SQUARE:
      for (int i = 0; i < audio_block_samples; i++) {
        //applyMod(i, lfo_block);

        if (_Phase <= _PI) {
          block_new->data[i] = 1.0f;
        } else {
          block_new->data[i] = -1.0f;
        }

        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
    case OSCILLATOR_MODE_TRIANGLE:
      for (int i = 0; i < audio_block_samples; i++) {
        //applyMod(i, lfo_block);

        float32_t value = -1.0f + (2.0f * _Phase / twoPI);
        block_new->data[i] = 2.0f * (fabs(value) - 0.5f);
        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
  }

  //save final state for next time
  _PhaseIncrement = all_dphase_rad_per_sample->data[audio_block_samples-1];
  _Frequency = final_freq_Hz;
  AudioStream_F32::release(all_dphase_rad_per_sample);  //release the temporary block used for phase increment calculations

  //scale the output by the given magnitude
  if (_magnitude != 1.0f) {
    arm_scale_f32(block_new->data, _magnitude, block_new->data, audio_block_samples);
  }
	
	return 0;
}

// /////////////////////////////////////////////////////////////////////////////
// /////  Repeat update and processAudioBlock for the quadrature versions //////
// /////////////////////////////////////////////////////////////////////////////


void AudioSynthWaveformQuadrature_F32::update(void) {
	
  //get input block (the modulation of the frequency)
  audio_block_f32_t *lfo_block = receiveReadOnly_f32(0);
  
  //get output block
  audio_block_f32_t *block_new = allocate_f32();
  audio_block_f32_t *block2_new = allocate_f32();
  if (!block_new || !block2_new) { 
 	  //could not allocate block.  So, release memory and return.
		AudioStream_F32::release(lfo_block); 
		AudioStream_F32::release(block_new);
		AudioStream_F32::release(block2_new);
		return; 
	}
	
  //process the audio to fill the output block with data samples
	processAudioBlock(lfo_block, block_new, block2_new);
  
	//update the counter on the new block
  block_counter++; block_new->id = block_counter;
	block_counter++; block2_new->id = block_counter;

	//transmit and release
  AudioStream_F32::transmit(block_new,0);
  AudioStream_F32::transmit(block2_new,1);
  AudioStream_F32::release(block_new);
  AudioStream_F32::release(block2_new);
  AudioStream_F32::release(lfo_block);
}


int AudioSynthWaveformQuadrature_F32::processAudioBlock(audio_block_f32_t *lfo_block, audio_block_f32_t *block_new, audio_block_f32_t *block2_new)  {
  audio_block_f32_t *all_dphase_rad_per_sample = allocate_f32();
  if (all_dphase_rad_per_sample==nullptr) { return -1; }  //could not allocate block.  So, release memory and return.
  float32_t final_freq_Hz = calc_modulation(lfo_block, audio_block_samples, all_dphase_rad_per_sample);

  switch (_OscillatorMode) {
    case OSCILLATOR_MODE_SINE:
      for (int i = 0; i < audio_block_samples; i++) {
        //applyMod(i, lfo_block);

        block_new->data[i] = arm_sin_f32(_Phase);
        block2_new->data[i] = arm_sin_f32(_Phase + PI_DIV_2);
        
        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
    case OSCILLATOR_MODE_SAW:
      for (int i = 0; i < audio_block_samples; i++) {
        //applyMod(i, lfo_block);

        block_new->data[i] = 1.0f - (2.0f * _Phase / twoPI);
        block2_new->data[i] = 1.0f - (2.0f * (_Phase+PI_DIV_2) / twoPI);
        
        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
    case OSCILLATOR_MODE_SQUARE:
      for (int i = 0; i < audio_block_samples; i++) {
        //applyMod(i, lfo_block);

        if (_Phase <= _PI) {
          block_new->data[i] = 1.0f;
        } else {
          block_new->data[i] = -1.0f;
        }
        
        if ( (_Phase+PI_DIV_2) <= _PI) {
          block2_new->data[i] = 1.0f;
        } else {
          block2_new->data[i] = -1.0f;
        }				

        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
    case OSCILLATOR_MODE_TRIANGLE:
      for (int i = 0; i < audio_block_samples; i++) {
        // applyMod(i, lfo_block);

        float32_t value = -1.0f + (2.0f * _Phase / twoPI);
        block_new->data[i] = 2.0f * (fabs(value) - 0.5f);

        float32_t value2 = -1.0f + (2.0f * (_Phase + PI_DIV_2) / twoPI);
        block2_new->data[i] = 2.0f * (fabs(value2) - 0.5f);

        //_Phase += _PhaseIncrement;
        _Phase += all_dphase_rad_per_sample->data[i];
        while (_Phase >= twoPI) { _Phase -= twoPI; }
      }
      break;
  }

  //save final state for next time
  _PhaseIncrement = all_dphase_rad_per_sample->data[audio_block_samples-1];
  _Frequency = final_freq_Hz;
  AudioStream_F32::release(all_dphase_rad_per_sample);  //release the temporary block used for phase increment calculations

  //scale the output by the given magnitude
  if (_magnitude != 1.0f) {
    arm_scale_f32(block_new->data, _magnitude, block_new->data, audio_block_samples);
		arm_scale_f32(block2_new->data, _magnitude, block2_new->data, audio_block_samples);
  }
	
	return 0;
}


// ////////////////////////////////////////////////////////////////////////////////
// /////// Return to defining methods for the non-quadrature version of the class
// /////////////////////////////////////////////////////////////////////////////////


// inline float32_t AudioSynthWaveform_F32::applyMod(uint32_t sample, audio_block_f32_t *lfo) {
	
// 	if (_PortamentoSamples > 0 && _CurrentPortamentoSample++ < _PortamentoSamples) {
// 		_Frequency+=_PortamentoIncrement;
// 	}

// 	float32_t osc_frequency = _Frequency;

// 	if (lfo && _PitchModAmt > 0.0f) {
// 		if (_ModMode == MOD_MODE_PER_OCT) {
// 			//input signal is assumed to be a pitch shift where 1.0 is one octave
// 			osc_frequency = _Frequency * powf(2.0f, 0.0f / 1200.0f + lfo->data[sample] * _PitchModAmt);
// 		} else {
// 			//input signal is assumed to be a frequency value in Hz
// 			osc_frequency = _Frequency + lfo->data[sample] * _PitchModAmt;
// 		}
// 	}

// 	_PhaseIncrement = osc_frequency * twoPI / sample_rate_Hz;

// 	return osc_frequency;
// }