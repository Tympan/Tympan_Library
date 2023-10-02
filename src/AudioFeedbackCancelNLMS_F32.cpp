
#include "AudioFeedbackCancelNLMS_F32.h"
#include <cfloat> //for "isfinite()"


//here's the method that is called automatically by the Teensy Audio Library
void AudioFeedbackCancelNLMS_F32::update(void) {
 
  //receive the input audio data
  audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
  if (!in_block) return;

  //allocate memory for the output of our algorithm
  audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
  if (!out_block) {
	AudioStream_F32::release(in_block);
	return;
  }

	//check to see if we're outpacing our feedback data
	if (newest_ring_audio_block_id != 999999) { //999999 is the default startup number, so ignore it
		if ((in_block->id > 100) && (newest_ring_audio_block_id > 0)) { //ignore startup period
			if (abs(in_block->id - newest_ring_audio_block_id) > 1) {  //is the difference more than one block counter?
				//the data in the ring buffer is older than expected!
				Serial.print("AudioFeedbackCancelNLMS_F32: falling behind?  in_block = ");
				Serial.print(in_block->id); Serial.print(", ring block = "); Serial.println(newest_ring_audio_block_id);
			}
		}
	}

  //do the work
  if (enabled) {
	cha_afc(in_block->data, out_block->data, in_block->length);
  } else {
	//simply copy input to output
	for (int i=0; i < in_block->length; i++) out_block->data[i] = in_block->data[i];
  }

  // transmit the block and release memory
  AudioStream_F32::transmit(out_block); // send the FIR output
  AudioStream_F32::release(out_block);
  AudioStream_F32::release(in_block);
}

void AudioFeedbackCancelNLMS_F32::cha_afc(float32_t *x, //input audio array
		float32_t *y, //output audio array
		int cs) //"chunk size"...the length of the audio array
{
  float32_t fbe, mum, s0, s1, ipwr;
  int i;
  float32_t *offset_ringbuff;
  //float32_t foo;

  // subtract estimated feedback signal
  for (i = 0; i < cs; i++) {  //step through WAV sample-by-sample
	s0 = x[i];  //current waveform sample
	//ii = rhd + i;
	offset_ringbuff = ring + (cs-1) - i;

	// estimate feedback
	#if 1
	  //is this faster?  Tested on Teensy 3.6.  Yes, this is faster.
	  arm_dot_prod_f32(offset_ringbuff, efbp, afl, &fbe); //from CMSIS-DSP library for ARM chips
	#else
	  fbe = 0;
	  for (int j = 0; j < afl; j++) {
		//ij = (ii - j + rsz) & mask;
		//fbe += ring[ij] * efbp[j];
		fbe += offset_ringbuff[j] * efbp[j];
	  }
	#endif

	// remove estimated feedback from the signal
	s1 = s0 - fbe;

	// calculate instantaneous power
	ipwr = s0 * s0 + s1 * s1;

	// estimate magnitude of the signal (low-pass filter the instantaneous signal power)
	pwr = rho * pwr + ipwr; //original

	// update adaptive feedback coefficients
	mum = mu / (eps + pwr);  // modified mu
	#if 1
	  //is this faster?  Tested on Teensy 3.6.  It is not faster
	  arm_scale_f32(offset_ringbuff,mum*s1,foo_float_array,afl);
	  arm_add_f32(efbp,foo_float_array,efbp,afl);
	#else
	  foo = mum*s1;
	  for (int j = 0; j < afl; j++) {
		//ij = (ii - j + rsz) & mask;
		//efbp[j] += mum * ring[ij] * s1;  //update the estimated feedback coefficients
		efbp[j] += foo * offset_ringbuff[j];  //update the estimated feedback coefficients
	  }
	#endif

//        if (n_coeff_to_zero > 0) {
//          //zero out the first feedback coefficients
//          for (int j=0; j < n_coeff_to_zero; j++) {
//            efbp[j] = 0.0;
//          }
//        }

	// copy AFC signal to output
	y[i] = s1;
  }
}

void AudioFeedbackCancelNLMS_F32::receiveLoopBackAudio(float *x, //input audio block
			int cs)   //number of samples in this audio block
{
  int Isrc, Idst;

  //Check to see if the in-coming values are valid floats (ie, not NaN or Inf).
  //If the system is overloading, this could happen, which would lock-up this
  //feedback cancelation algorithm.
  for (int i=0; i<cs; i++) { 
	if (!std::isfinite(x[i])) {
		//bad data found!  reset the states and return early
		initializeStates();
		return;
	}
  }
  
  //we're going to store the audio data in reverse order so that
  //the newest is at index 0 and the oldest is at the end
  //the only part of the buffer that we're using is [0 afl+cs-1]
  
  //slide the data to the older end of the buffer (overwriting the very oldest data)
  Idst = afl+cs-1;  //start with the destination at the end of the buffer
  for (int Isrc = (afl-1); Isrc > -1; Isrc--) {
	ring[Idst]=ring[Isrc]; 
	Idst--;

  }

  //add new data to front (we're also reversing it)
  Idst = cs-1;  //the given data is only "cs" long
  for (Isrc=0; Isrc < cs; Isrc++) { //the given data is in normal order (oldest first, newest last) so start at index 0
	ring[Idst]=x[Isrc]; 
	Idst--;
  }
}