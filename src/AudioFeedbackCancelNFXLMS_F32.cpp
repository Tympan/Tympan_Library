
#include "AudioFeedbackCancelNFXLMS_F32.h"

// iltass.h - inverted long-term average speech spectrum
#include "utility/iltass.h"  //part of Tympan_Library (in the "hutility" directory).  Includes iltass[] and WFSZ


void AudioFeedbackCancelNFXLMS_F32::update(void) {

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
		Serial.print("AudioFeedbackCancelNFXLMS_F32: falling behind?  in_block = ");
		Serial.print(in_block->id); Serial.print(", ring block = "); Serial.println(newest_ring_audio_block_id);
	  }
	}
  }

  //do the work
  if (enabled) {
	cha_afc_input(in_block->data, out_block->data, in_block->length);
  } else {
	//simply copy input to output
	for (int i = 0; i < in_block->length; i++) out_block->data[i] = in_block->data[i];
  }

  // transmit the block and release memory
  AudioStream_F32::transmit(out_block); // send the FIR output
  AudioStream_F32::release(out_block);
  AudioStream_F32::release(in_block);
}

void AudioFeedbackCancelNFXLMS_F32::cha_afc_prepare(void) {
  //float fbm = 0;
 
  cs = audio_block_samples;
  //const int FBSZ = 100; //from ite_fb.h
  //mxl = 0;
  //rsz = 32; //why start with this value?  why not just compute what is needed, as is done below

  //fbl = (fbg > 0) ? FBSZ : 0;
  int fbl = 0; //force simulated feedback legnth to be zero
  //mxl = (fbl > afl) ? fbl : (afl > wfl) ? afl : (wfl > pfl) ? wfl : pfl;
  //while (rsz < (mxl + hdel + cs))  rsz *= 2;
  //rsz = max(rsz, (mxl + hdel + cs)); //chip's replacement
  mxl = computeNewMaxLength(fbl,afl,wfl,pfl);
  rsz = computeNewRingbufferSize(mxl,hdel,cs);

  // initialize whiten filter
  if (wfl > 0) {
	  //cha_allocate(cp, rsz, sizeof(float), _rng3); // ee -> rng3
	  //cha_allocate(cp, rsz, sizeof(float), _rng2); // uf -> rng2
	  //wfrp = cha_allocate(cp, wfl, sizeof(float), _wfrp);
	  white_filt(wfrp, wfl);
  }

  // initialize band-limit filter
  if (pfl > 0) {
	  //cha_allocate(cp, rsz, sizeof(float), _rng1); // uu -> rng1
	  //ffrp = cha_allocate(cp, pfl, sizeof(float), _ffrp);
	  ffrp[0] = 1;
  } else {
	  pup = 0;
  }
  //none of the rest of the code in afc_prepare.c is needed for this implementation here
}

void AudioFeedbackCancelNFXLMS_F32::white_filt(float *h, int n) {  //initialize the whitening filter
	if (n < 3) {
		h[0] = 1;
		} else {
		int m = (n - 1) / 2;
		m = min(m, WFSZ-1);
		h[m] = iltass[0];
		for (int i = 1; i <= m; i++) h[m - i] = h[m + i] = iltass[i];
	}
}

//Here is the routine for actually doing the feedback cancelation and updating
//the adaptive filter (the name "cha_afc_input" is simply the name used by BTNRH
//in their original code)
int AudioFeedbackCancelNFXLMS_F32::cha_afc_input(float32_t *x, float32_t *y, int cs) {
  //float ye, yy, mmu, dif, dm, xx, ee, uu, ef, uf, cfc, sum, pwr;
  float ye, yy, mmu, xx, ee, uu, ef, uf, cfc, sum; //, pwr;
  //int i, ih, ij, is, id, j, jp1, k, nfc, puc, iqm = 0;
  int i, ih, ij, is, id, j, jp1, k, nfc; //, puc, iqm = 0;
  //static float *rng0, *rng1, *rng2, *rng3;
  //static float *efbp, *sfbp, *wfrp, *ffrp, *qm;
  //static float mu, rho, eps, alf, fbm;
  //static int rhd, rsz, mask, afl, wfl, pfl, fbl, nqm, hdel, pup, *iqmp; 
  
  int mask = rsz - 1;  //added by WEA.  "rsz" must be a factor of two for this to work!

  // loop over chunk
  for (i = 0; i < cs; i++) {
	  //------------------------------------
	  xx = x[i];
	  ih = (rhd + i) & mask;
	  is = ih + rsz;
	  id = is - hdel;
	  // simulate feedback
	  yy = 0;
//        for (j = 0; j < fbl; j++) {
//            ij = (id - j) & mask;
//            yy += sfbp[j] * rng0[ij];
//        }
	  // apply band-limit filter
	  if (pfl > 0) {
		  uu = 0;
		  for (j = 0; j < pfl; j++) {
			  ij = (is - j) & mask;
			  uu += ffrp[j] * rng0[ij];
		  }
		  rng1[ih] = uu;
	  }
	  // estimate feedback
	  ye = 0;
	  if (afl > 0) {
		  for (j = 0; j < afl; j++) {
			  ij = (id - j) & mask;
			  ye += efbp[j] * rng1[ij];
		  }
	  }
	  // apply feedback to input signal
	  ee = xx + yy - ye;
	  //------------------------------------
	  // apply whiten filter
	  if (wfl > 0) {
		  rng3[ih] = ee;
		  ef = uf = 0;
		  for (j = 0; j < wfl; j++) {
			  ij = (is - j) & mask;
			  ef += rng3[ij] * wfrp[j];
			  uf += rng1[ij] * wfrp[j];
		  }
		  rng2[ih] = uf;
	  } else {
		  ef = ee;
	  }
	  // update adaptive feedback coefficients
	  if (afl > 0) {
		  uf = rng2[id & mask];
		  //pwr = rho * sqrtf(ef * ef + uf * uf) + (1.0f - rho) * pwr;  //WEA Nov 2021...per Steve Neely email Nov 8, 2021
		  pwr = rho * (ef * ef + uf * uf) + (1 - rho) * pwr;
		  mmu = mu / (eps + pwr);  // modified mu
		  for (j = 0; j < afl; j++) {
			  ij = (id - j) & mask;
			  uf = rng2[ij];
			  efbp[j] += mmu * ef * uf;
		  }
	  }
	  // update band-limit filter coefficients
	  if (pup) {
		puc = (puc + 1) % pup;
		if (puc == 0) {
		  sum = 0;
		  for (j = 0; j < pfl; j++) {
			jp1 = j + 1;
			nfc = (jp1 < pfl) ? jp1 : pfl;
			cfc = 0;
			for (k = 0; k < nfc; k++) {
			  cfc += efbp[j - k] * ffrp[k];
			}
			ffrp[j] += alf * (cfc - ffrp[j]);
			sum += ffrp[j];
		  }
		  sum /= pfl;
		  for (j = 0; j < pfl; j++) {
			  ffrp[j] -= sum;
		  }
		}
	  }
	  // save quality metrics
//        if (nqm) {
//          float dm = 0;
//          for (j = 0; j < fbl; j++) {
//            if (pfl) {
//              jp1 = j + 1;
//              nfc = (jp1 < pfl) ? jp1 : pfl;
//              cfc = 0;
//              for (k = 0; k < nfc; k++) {
//                  cfc += efbp[j - k] * ffrp[k];
//              }
//            } else {
//              cfc = efbp[j];
//            }
//            float dif = (j < afl) ? sfbp[j] - cfc : sfbp[j];
//            dm += dif * dif;
//          }
//          qm[iqm++] = dm / fbm;
//        }
	  // copy AFC signal to output
	  y[i] = ee;
  }
//    CHA_IVAR[_puc] = puc;
//    CHA_DVAR[_pwr] = pwr;
//    if (nqm) {
//        iqmp[0] = iqm;
//        if ((iqm + cs) > nqm) nqm = 0;
//    }

  return 0;
}

//Here is the routine for handling the looped-back audio (the name "cha_afc_output"
//is simply the name used by BTNRH in their original code)
void AudioFeedbackCancelNFXLMS_F32::cha_afc_output(float *x, int cs) { // audio input array and the block (chunk) size 
  //int i, j, rhd, rtl;
  int i, j;  //, rhd, rtl;
  //static float *rng0;
  //static int rsz, mask;

  int mask = rsz - 1;  //"rsz" must be a factor of 2 for this to work!!

//      if (CHA_IVAR[_mxl] == 0) { // if no AFC, do nothing
//          return;
//      }
//      if (CHA_IVAR[_in1] == 0) {
//          rng0 = (float *) cp[_rng0];
//          rsz = CHA_IVAR[_rsz];
//          mask = rsz - 1;
//          CHA_IVAR[_in1] = CHA_IVAR[_in2];
//      }
//      rsz = CHA_IVAR[_rsz];
//      rtl = CHA_IVAR[_rtl];
  // copy chunk to ring buffer
  rhd = rtl;
  for (i = 0; i < cs; i++) {
	  j = (rhd + i) & mask;
	  rng0[j] = x[i];
  }
  rtl = (rhd + cs) % rsz;
//     CHA_IVAR[_rhd] = rhd;
//      CHA_IVAR[_rtl] = rtl;
}
