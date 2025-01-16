
#include "AudioEffectPitchShift_FD_F32.h"


int AudioEffectPitchShift_FD_F32::setup(const AudioSettings_F32 &settings, const int _N_FFT) {
  sample_rate_Hz = settings.sample_rate_Hz;
  audio_block_samples = settings.audio_block_samples;

  //setup the FFT and IFFT.  If they return a negative FFT, it wasn't an allowed FFT size.
	int prev_N_FFT = N_FFT;
	if (prev_N_FFT != _N_FFT) {
		N_FFT = myFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
		if (N_FFT < 1) return N_FFT;
		N_FFT = myIFFT.setup(settings, _N_FFT); //hopefully, we got the same N_FFT that we asked for
		if (N_FFT < 1) return N_FFT;
	}
	
  //decide windowing
  Serial.println("AudioEffectPitchShift_FD_F32: setting myFFT to use hanning...");
  (myFFT.getFFTObject())->useHanningWindow(); //applied prior to FFT
  if (myIFFT.getNBuffBlocks() > 3) {
    Serial.println("AudioEffectFormantShiftFD_F32: setting myIFFT to use hanning...");
    (myIFFT.getIFFTObject())->useHanningWindow(); //window again after IFFT
  }

  //decide how much overlap is happening
  switch (myIFFT.getNBuffBlocks()) {
    case 0:
      //should never happen
      break;
    case 1:
      overlap_amount = NONE;
      break;
    case 2:
      overlap_amount = HALF;
      break;
    case 3:
      //to do...need to add phase shifting logic to the update() function to support this case
      break;
    case 4:
      overlap_amount = THREE_QUARTERS;
      //to do...need to add phase shifting logic to the update() function to support this case
      break;
  }
    

  #if 0
    //print info about setup
    Serial.println("AudioEffectPitchShift_FD_F32: FFT parameters...");
    Serial.print("    : N_FFT = "); Serial.println(N_FFT);
    Serial.print("    : audio_block_samples = "); Serial.println(settings.audio_block_samples);
    Serial.print("    : FFT N_BUFF_BLOCKS = "); Serial.println(myFFT.getNBuffBlocks());
    Serial.print("    : IFFT N_BUFF_BLOCKS = "); Serial.println(myIFFT.getNBuffBlocks());
    Serial.print("    : FFT use window = "); Serial.println(myFFT.getFFTObject()->get_flagUseWindow());
    Serial.print("    : IFFT use window = "); Serial.println((myIFFT.getIFFTObject())->get_flagUseWindow());
  #endif

  //allocate memory to hold frequency domain data
	if (prev_N_FFT != _N_FFT) {
		if (complex_2N_buffer) {delete[] complex_2N_buffer;} complex_2N_buffer = new float32_t[2 * N_FFT];
		if (cur_mag) { delete[] cur_mag;}                    cur_mag = new float32_t[N_FFT/2+1];
		if (prev_mag) { delete[] prev_mag;}              		 prev_mag = new float32_t[N_FFT/2+1];
		if (cur_phase) { delete[] cur_phase;}           		 cur_phase = new float32_t[N_FFT/2+1];
		if (prev_phase) { delete[] prev_phase;}              prev_phase = new float32_t[N_FFT/2+1];
		if (cur_dPhase) { delete[] cur_dPhase;}              cur_dPhase = new float32_t[N_FFT/2+1];
		if (prev_dPhase) { delete[] prev_dPhase;}            prev_dPhase = new float32_t[N_FFT/2+1];
		if (new_mag) { delete[] new_mag;}              		   new_mag = new float32_t[N_FFT/2+1];
		if (prev_new_mag) { delete[] prev_new_mag;}          prev_new_mag = new float32_t[N_FFT/2+1];
		if (shifted_phases) { delete[] shifted_phases;}      shifted_phases = new float32_t[N_FFT/2+1];
	}

  //pass parameters to the resampling filter
  resample_filter.setSampleRate_Hz(sample_rate_Hz);

  //reset all the state-holding variables
  resetState();

  //we're done.  return!
  enabled = 1;
  return N_FFT;
}

void AudioEffectPitchShift_FD_F32::resetState(void) {
  auto len = getNFFT()/2 + 1;
  for (auto i = 0; i < len; i++) {
    cur_mag[i] = 0.0f;
    prev_mag[i] = 0.0f;
    cur_phase[i] = 0.0f;
    prev_phase[i] = 0.0f;
    cur_dPhase[i] = 0.0f;
    prev_dPhase[i] = 0.0f;
    new_mag[i] = 0.0f;
    prev_new_mag[i] = 0.0f;
    shifted_phases[i] = 0.0f;
  }
  targ_idx = 1.0f;   //initialize to 1.0 to cause it to not compute output the first time through
  ind_float = 0.0f;  //used in resampling
  resampled_audio_blocks.release_all_and_clear();   //this releases all the pointers and empties the deque
  resample_filter.resetState();
}
void AudioEffectPitchShift_FD_F32::update(void)
{
  //get a pointer to the latest data
  audio_block_f32_t *in_audio_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_audio_block) return;

  //simply return the audio if this class hasn't been enabled
  if (!enabled) {
    AudioStream_F32::transmit(in_audio_block);
    AudioStream_F32::release(in_audio_block);
    return;
  }

  //extract meta-info from the input audio blocks
  audio_block_samples = in_audio_block->length; //update the length of the blocks based on the input
  auto incoming_id = in_audio_block->id;

  // /////////////////////// Beginning of the actual audio procssing

  //create time-stretched (not pitch shifted) version of the audo
  MultiAudioBlocks_F32 stretched_audio_blocks;
  timeStretchAudio(in_audio_block,stretched_audio_blocks);
  AudioStream_F32::release(in_audio_block); //we're finished with the input audio, so release it

  //resample the audio to "play" the audio back at a different speed, thereby
  //raising or lowering the pitch
  resampleAudio(stretched_audio_blocks,resampled_audio_blocks);
  stretched_audio_blocks.release_all_and_clear();  //we don't need the stretched audio blocks any more so, clear

  // /////////////////////// End of the actual audio procssing

  //is there enough audio to send to the output?
  if (resampled_audio_blocks.size() == 0) return;  //no audio is ready
  if ((resampled_audio_blocks.size() == 1) && (resampled_audio_blocks.len_written < audio_block_samples)) return;  //not enough audio is ready

  //transmit output...get the oldest audio block and send to the output
  audio_block_f32_t *out_audio_block = resampled_audio_blocks.front(); //get the oldest from the queue
  resampled_audio_blocks.pop_front();  //remove it from the deque
  if (out_audio_block == NULL) return; //check to see if it's valid
  out_audio_block->id = incoming_id;   //sets its ID number (hopefully, this is sequential)
  AudioStream_F32::transmit(out_audio_block);  //transmit
  AudioStream_F32::release(out_audio_block);   //release
  
  return;
}

void AudioEffectPitchShift_FD_F32::timeStretchAudio(audio_block_f32_t *in_audio_block, MultiAudioBlocks_F32 &out_audio_blocks)
{

  // ////////////// Convert input data into frequency domain, including related metrics (ie, Analysis)

  //move the previously-computed data over to the "prev" variables...though, never actually move the data, just repoint our references
  auto foo_ptr = prev_mag; prev_mag = cur_mag;       cur_mag = foo_ptr;    //reference previous data with prev pointer
  foo_ptr = prev_phase;    prev_phase = cur_phase;   cur_phase = foo_ptr;  //reference previous data with prev pointer
  foo_ptr = prev_dPhase;   prev_dPhase = cur_dPhase; cur_dPhase = foo_ptr; //reference previous data with prev pointer

  //convert to frequency domain
  myFFT.execute(in_audio_block, complex_2N_buffer); //FFT is in complex_2N_buffer, interleaved real, imaginary, real, imaginary, etc
  auto N_2 = myFFT.getNFFT() / 2 + 1;

  // extract magnitude and info about the FFT
  arm_cmplx_mag_f32(complex_2N_buffer, cur_mag, N_2);  //get the magnitude for each FFT bin and store somewhere safes
  for (auto Ifreq=0; Ifreq < N_2; Ifreq++ ) {
    cur_phase[Ifreq] = atan2f(complex_2N_buffer[2*Ifreq+1],complex_2N_buffer[2*Ifreq]); //get the phase
    cur_dPhase[Ifreq] = cur_phase[Ifreq] - prev_phase[Ifreq];  //get the dPhase
    cur_dPhase[Ifreq] -= (int)(cur_dPhase[Ifreq]/(2.0*PI))*(2.0*PI);  //wrap to [0 -> 2*pi]
  }

  // ///////////// Create new FFT blocks (ie, Synthesis) to time-stretch the audio without changing pitch
 
  //begin loop to create new FFT audio blocks ("synthesis" blocks) based on interpolating between the previous
  //given FFT block (previous "analysis" block) and the current given FFT block (current "analysis" block)

  audio_block_f32_t *synth_audio_block;
  while (targ_idx < 1.0f) {
    // yes, it's time to generate a synthesis block!

    //grab some memory to hold the results
    synth_audio_block = AudioStream_F32::allocate_f32();
    if (synth_audio_block == NULL) return; //out of memory!
 
    //create a new audio block ("synthesis") with the time-lengthened (or time-shortened) audio...but pitch is unchanged
    auto foo_ptr = prev_new_mag; prev_new_mag = new_mag;  new_mag = foo_ptr; //copy previous data
    synthesizeNewAudioBlock(targ_idx, N_2,synth_audio_block); //all other input data is passed via the class's data members.  tmp_audio_block is the output

    //add the full block to the output queue
    out_audio_blocks.push_back(synth_audio_block); //these are time-stretched.  Pitch shifting will occur when they are resapmled
    out_audio_blocks.len_written = synth_audio_block->length; //last sampele written

    //prepare for the next loop to create the next block of synthesized audio
    targ_idx = targ_idx + 1.0/scale_fac;  //increment our index for interpolating into the "analysis" data
    
  } //end of looping through the current input data

  // prepare for next batch of input data (ie, the next call to update()). For targ_idx, a value of 0.0
  // represents the previous audio block and 1.0 represents the most recent.  Therefore, we subtract 1.0
  // from targ_idx at this point in the code in anticipation of receiving the next audio block.
  targ_idx -= 1.0f;  // do not use targ_idx again until the next call to update().

  return;
}

void AudioEffectPitchShift_FD_F32::synthesizeNewAudioBlock(float32_t frac, const int N_2, audio_block_f32_t *synth_audio_block) {

  //loop over each frequency
  for (auto Ifreq=0; Ifreq < N_2; Ifreq++) {
    //interpolate new magnitude and new phase differences
    new_mag[Ifreq] = prev_mag[Ifreq]*(1.0f-frac) + cur_mag[Ifreq]*frac;  //compute the new magnitude values
    float32_t new_dPhase = prev_dPhase[Ifreq]*(1.0f-frac) + cur_dPhase[Ifreq]*frac;
    
    //calculate new phases, ignoring transients
    shifted_phases[Ifreq] += new_dPhase;
    
    //look for transient     
    auto transient = (new_mag[Ifreq] - prev_new_mag[Ifreq]) / (new_mag[Ifreq] + prev_new_mag[Ifreq]);
    if (transient >= transient_thresh) {
      //this is a transient!  
      shifted_phases[Ifreq] = cur_phase[Ifreq]; //reset phase to be the measured current phase
      transient_count++; //counter used for debugging
    }
    
    //wrap the new phase
    shifted_phases[Ifreq] -= (int)(shifted_phases[Ifreq]/(2.0*PI))*(2.0*PI);  //wrap to [0 -> 2*pi]
    
    //convert back to complex values
    complex_2N_buffer[2*Ifreq]   = new_mag[Ifreq] * cosf(shifted_phases[Ifreq]); //real
    complex_2N_buffer[2*Ifreq+1] = new_mag[Ifreq] * sinf(shifted_phases[Ifreq]); //imaginary
  } //end of loop over frequencies
  
  //convert the newly-created FFT block into the time domain (this includes the windowing and the overlap-and-add
  myFFT.rebuildNegativeFrequencySpace(complex_2N_buffer); //set the negative frequency space based on the positive
  myIFFT.execute(complex_2N_buffer, synth_audio_block); //output is via synth_audio_block 
}

//resample ALL of the input audio blocks and put output into resampled_audio_blocks.
//The resampled_audio_blocks might have pre-existing data.  This method *adds* its output
//to any pre-existing data in resampled_audio_bocks.  This method will release any of the
//input audio blocks that have been used.
void AudioEffectPitchShift_FD_F32::resampleAudio(MultiAudioBlocks_F32 &stretched_audio_blocks, \
                                                 MultiAudioBlocks_F32 &resampled_audio_blocks)
{
  //if we're not given any data, just return
  if (stretched_audio_blocks.empty()) return;
  
  //ensure we have memory allocated for our output
  if (resampled_audio_blocks.empty()) {
    auto is_ok = resampled_audio_blocks.allocateAndAddBlock();
    if (!is_ok) { stretched_audio_blocks.release_all_and_clear(); return; }
  }
  float32_t *out_audio = resampled_audio_blocks.back()->data; //get the newest one

  //loop over all of the input blocks
  while (!stretched_audio_blocks.empty())
  {
    //get the oldest audio block that was given to us as an input
    audio_block_f32_t *in_audio_block = stretched_audio_blocks.front(); //get an audio block
    stretched_audio_blocks.pop_front();  //remove it from the queue
    if (in_audio_block == NULL) continue; //skip this iteration if the input is empty...which should never happen
    
    //if we're down-sampling (ie, going to higher pitch), pre-filter the input to reduce aliasing
    if (scale_fac > 1.0f) filterAudioBlock(in_audio_block);

    //generate new (resampled) audio datapoints from the in_audio_block, assuming each
    //in_audio_block is full of data.
    while (ind_float < (float32_t)(in_audio_block->length))  //loop until we use all the input data
    {
      //is our output array filled?  if so, make another output array
      if (resampled_audio_blocks.len_written >= audio_block_samples) {
        //it is filled! so, close out the filled array
        if (scale_fac < 1.0f) filterAudioBlock(resampled_audio_blocks.back()); //if we're up-sampling (ie, going to lower pitch), post-filter to reduce aliasing

        //next, allocate a new output array
        auto is_ok = resampled_audio_blocks.allocateAndAddBlock(); //allocate and add a new block
        if (!is_ok) { AudioStream_F32::release(in_audio_block); return; }
        out_audio = resampled_audio_blocks.back()->data; //get the newly-added audio block...get a pointer to the float array itself
      }
        
      //linearly interpolate a new waveform point...but are we fully into the new synth_audio_block yet?
      float32_t cur_val;
      if (ind_float < 1.0f) {
        //we need to use the historical data point..the last point in the prev synth_audio_block
        auto frac = ind_float - ((int)ind_float);
        cur_val = prev_last_sample_value*(1.0f-frac) + in_audio_block->data[0]*frac;
      } else {
        //we're just using the new data that's within synth_audio_block
        auto ind_below = (int)(ind_float-1.0f); //ind_float of 1.0 corresponds to the zeroth element of synth_audio_block
        auto frac = ind_float - ((int)ind_float);   //this is the amount it is between samples.  0.001 is close to the sample below and 0.999 is close to the sample above
        cur_val = in_audio_block->data[ind_below]*(1.0f-frac) + in_audio_block->data[ind_below+1]*frac; //do the linear interpolation
      }

      //save value
      out_audio[resampled_audio_blocks.len_written++] = cur_val;
   
      //prepare for next interpolation within the resmpling
      ind_float += scale_fac; //increment the index into the synthesized audio block      
      
    } //end of the point-by-point processing of this input audio block
    
    //prepare for the next input audio block
    ind_float -= in_audio_block->length; //reset ind_float back to the begining (keepig the residual) in anticipation of the next "synthesis" data
    prev_last_sample_value = in_audio_block->data[(in_audio_block->length)-1]; //get the last sample from the most recent "synthesis" data
    AudioStream_F32::release(in_audio_block);
    
  } // end loop over input data blocks

  //check to see if the last sample has filled the array, which means that we could close it out and transmit
  if ((resampled_audio_blocks.empty() == false) && (resampled_audio_blocks.len_written >= audio_block_samples)) {
    //it is filled! so, close out the filled array
    if (scale_fac < 1.0f) filterAudioBlock(resampled_audio_blocks.back()); //if we're up-sampling (ie, going to lower pitch), post-filter to reduce aliasing
  }
  
  return;
};

void AudioEffectPitchShift_FD_F32::configureResampleFilterGivenScaleFac(float scaleFac) {
  float cutoff_Hz;
  if (scaleFac > 1.0f) {
    //raising pitch, so resampling will be a downsample (fewer samples when we're done),
    cutoff_Hz = 0.9f * (sample_rate_Hz*0.5f) / scaleFac;
  } else { //such that scaleFac < 1.0
    //lowering pitch, so resampling will be an upsample (more samples when we're done),
    cutoff_Hz = 0.9f * (sample_rate_Hz*0.5f) * scaleFac;
  }
  resample_filter.setLowpass(0, cutoff_Hz);
  return;
}

void AudioEffectPitchShift_FD_F32::filterAudioBlock(audio_block_f32_t *audio_block) {
  resample_filter.processAudioBlock(audio_block, audio_block); //results are in-place
  return;
}

