
#ifndef _AudioEffectAFC_BTNRH_F32_h
#define _AudioEffectAFC_BTNRH_F32_h

#include "AudioEffectFeedbackCancel_Local_F32.h"

//Purpose: Make an AFC class that is closer to what BTNRH originally wrote
//Author: Chip Audette  2018-06-08

class AudioEffectAFC_BTNRH_F32 : public AudioEffectFeedbackCancel_Local_F32 {
  public:
    //constructor
    AudioEffectAFC_BTNRH_F32(void) : AudioEffectFeedbackCancel_Local_F32() { }
    AudioEffectAFC_BTNRH_F32(const AudioSettings_F32 &settings) : AudioEffectFeedbackCancel_Local_F32(settings) { };

    virtual void cha_afc(float32_t *x, //input audio array
                         float32_t *y, //output audio array
                         int cs) //"chunk size"...the length of the audio array
    {
      float32_t fbe, mum, s0, s1, ipwr;
      int i,ii,j,ij;
      //float32_t *offset_ringbuff;
      float32_t foo;

      //float32_t *ring, *efbp, *sfbp, *merr;
      //float32_t fbe, fbs, mum, dif, fbm, dm, s0, s1, mu, rho, pwr, ipwr, eps = 1e-30f;
      //int i, ii, ij, j, afl, fbl, nqm, rsz, rhd, mask;

//      mu  = (float) CHA_DVAR[_mu];
//      rho = (float) CHA_DVAR[_rho];
//      pwr = (float) CHA_DVAR[_pwr];
//      fbm = (float) CHA_DVAR[_fbm];
//      rsz = CHA_IVAR[_rsz];
//      rhd = CHA_IVAR[_rhd];
//      afl = CHA_IVAR[_afl];
//      fbl = CHA_IVAR[_fbl];
//      nqm = CHA_IVAR[_nqm];
//      ring = (float *) cp[_ring];
//      efbp = (float *) cp[_efbp];
//      sfbp = (float *) cp[_sfbp];
//      merr = (float *) cp[_merr];


      int rsz = max_afc_ringbuff_len;  //length of ring buffer
      int mask = rsz - 1;

      // subtract estimated feedback signal
      for (i = 0; i < cs; i++) {  //step through WAV sample-by-sample
        s0 = x[i];  //current waveform sample
        ii = rhd + i;

        // // simulate feedback [don't need this in the real-time system?]
        //fbs = 0;
        //for (j = 0; j < fbl; j++) {
        //    ij = (ii - j + rsz) & mask;
        //    fbs += ring[ij] * sfbp[j];  //feedback signal
        //}

        // using our estimated feedback model (efbp), let's estimate feedback signal that we think that we should be receiving
        fbe = 0; //initialize to zero
        for (j = 0; j < afl; j++) {    //loop over each AFC filter indices
          ij = (ii - j + rsz) & mask;  //compute index into ring buffer of previous audio data
          fbe += ring[ij] * efbp[j];   //apply the estimated feedback model (efbp) to the audio data to estimate the current feedback signal
        }

        // apply the simulated feedback to input signal and remove the estimated feedback signal
        //s1 = s0 + fbs - fbe;

        // remove estimated feedback from input signal
        s1 = s0 - fbe;

        // calculate instantaneous power of the original and the canceled signal
        ipwr = s0 * s0 + s1 * s1;

        // update adaptive feedback coefficients
        pwr = rho * pwr + ipwr;
        mum = mu / (eps + pwr);  // modified mu
        foo = mum * s1;
        int ii_rsz = ii + rsz;
        for (j = 0; j < afl; j++) {
          //ij = (ii - j + rsz) & mask;
          ij = (ii_rsz - j) & mask;     //calc index into ringbuffer
          //efbp[j] += mum * ring[ij] * s1;  //update the estimated feedback coefficients
          efbp[j] += foo * ring[ij];  //update the estimated feedback coefficients
        }

        // // save quality metrics
        //if (nqm > 0) {
        //    dm = 0;
        //    for (j = 0; j < nqm; j++) {
        //        dif = sfbp[j] - efbp[j];
        //        dm += dif * dif;
        //    }
        //    merr[i] = dm / fbm;
        //}

        if (n_coeff_to_zero > 0) {
          //zero out the first feedback coefficients
          for (int j=0; j < n_coeff_to_zero; j++) {
            efbp[j] = 0.0;
          }
        }

        // copy AFC signal to output
        y[i] = s1;
      }

      // // save power estimate
      //CHA_DVAR[_pwr] = pwr;
    }


    virtual void addNewAudio(audio_block_f32_t *in_block) {
      newest_ring_audio_block_id = in_block->id; 
      addNewAudio(in_block->data, in_block->length);
    }
    virtual void addNewAudio(float *x, //input audio block
                       int cs)   //number of samples in this audio block
    {
      //float *ring;
      int i, j;

      int rsz = max_afc_ringbuff_len;
  
      //rsz = CHA_IVAR[_rsz];    //size of ring buffer
      //rhd = CHA_IVAR[_rhd];    //where to start in the target ring buffer.  Overwritten!
      //rtl = CHA_IVAR[_rtl];    //where it last ended in the target ring buffer
      //ring = (float *) cp[_ring];
      int mask = rsz - 1;          //what sample to jump back to zero?
      
      rhd = rtl;   //where to start in the target ring buffer
      // copy chunk to ring buffer
      for (i = 0; i < cs; i++) {  //loop over each sample in the audio chunk
          j = (rhd + i) & mask;  //get index into target array.  This a fast way of doing modulo?
          ring[j] = x[i];   //save sample into ring buffer
      }
      rtl = (rhd + cs) % rsz;  //save the last write point in the ring buffer
      //CHA_IVAR[_rhd] = rhd;   //save the start point 
      //CHA_IVAR[_rtl] = rtl;   //save the end point
    }
  protected:

    
};

#endif
