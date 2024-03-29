/*
 * AudioCalcGainWDRC_F32
 * 
 * Created: Chip Audette, Feb 2017
 * Purpose: This module calculates the gain needed for wide dynamic range compression.
 * Derived From: Core algorithm is from "WDRC_circuit"
 *     WDRC_circuit from CHAPRO from BTNRC: https://github.com/BTNRH/chapro
 *     As of Feb 2017, CHAPRO license is listed as "Creative Commons?"
 *          
 * This processes a single stream fo audio data (ie, it is mono)       
 *          
 * MIT License.  use at your own risk.
*/

#ifndef _AudioCalcGainWDRC_F32_h
#define _AudioCalcGainWDRC_F32_h

#include <arm_math.h> //ARM DSP extensions.  for speed!
#include "AudioStream_F32.h"
#include "BTNRH_WDRC_Types.h"

class AudioCalcGainWDRC_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName:calc_WDRCGain2
  public:
    //default constructor
    AudioCalcGainWDRC_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { setDefaultValues(); };
	AudioCalcGainWDRC_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { setDefaultValues(); };

    //here's the method that does all the work
    void update(void) {
      
      //get the input audio data block
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32(); // must be the envelope!
      if (!in_block) return;
  
      //prepare an output data block
      audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
      if (!out_block) return;
      
      // ////////////////////// do the processing here!
      calcGainFromEnvelope(in_block->data, out_block->data, in_block->length);
      out_block->length = in_block->length; 
	  out_block->fs_Hz = in_block->fs_Hz;
	  out_block->id = in_block->id;
      
      //transmit the block and be done
      AudioStream_F32::transmit(out_block);
      AudioStream_F32::release(out_block);
      AudioStream_F32::release(in_block);
      
    }
  
    void calcGainFromEnvelope(float *env, float *gain_out, const int n)  {
      //env = input, signal envelope (not the envelope of the power, but the envelope of the signal itslef)
      //gain = output, the gain in natural units (not power, not dB)
      //n = input, number of samples to process in each vector
  
      //prepare intermediate data block
      audio_block_f32_t *env_dB_block = AudioStream_F32::allocate_f32();
      if (!env_dB_block) return;
  
      //convert to dB and calibrate (via maxdB)
      for (int k=0; k < n; k++) env_dB_block->data[k] = maxdB + db2(env[k]); //maxdb in the private section 

	  
      // apply wide-dynamic range compression
      //WDRC_circuit_gain(env_dB_block->data, gain_out, n, exp_cr, exp_end_knee, tkgn, tk, cr, bolt);
	  WDRC_circuit_gain_preComputedParams(env_dB_block->data, gain_out, n);
      AudioStream_F32::release(env_dB_block);
    }

	// Here is a faster method, but it gives the wrong answers.  The transition points (the knee points) are correct
	// but the computed gains are wrong
/* 	void WDRC_circuit_gain_preComputedParams(float *env_dB, float *gain_out, const int n) {
		float *pdb = env_dB; //just rename it to keep the code below unchanged (input SPL dB)
		float gdb;
		for (int k = 0; k < n; k++) {  //loop over each sample
			if (pdb[k] < exp_end_knee) {  //if below the expansion threshold, do expansion
			  //expansion region.
			  gdb = gain_at_exp_end_knee - ((exp_end_knee-pdb[k])*exp_cr_const); //reduce gain the farther down you are from the end of the expansion region
			} else if ((pdb[k] < tk_tmp) && (cr >= 1.0f)) {  //if below the compression threshold, go linear
				gdb = tkgn;  //we're in the linear region.  Apply linear gain.
			} else if (pdb[k] > pblt) { //we're beyond the compression region into the limitting region
				gdb = bolt + ((pdb[k] - pblt) / 10.0f) - pdb[k]; //10:1 limiting!
			} else {
				gdb = cr_const * pdb[k] + tkgo; 
			}
			gain_out[k] = undb2(gdb);
			//y[k] = x[k] * undb2(gdb); //apply the gain
		}
		last_gain = gain_out[n-1];  //hold this value, in case the user asks for it later (not needed for the algorithm)
	} */
	
	//instead, for now, let's just point to the full code and accept the computational penalty
	void WDRC_circuit_gain_preComputedParams(float *env_dB, float *gain_out, const int n) {
		WDRC_circuit_gain(env_dB, gain_out, n, 
			exp_cr, exp_end_knee, 
			tkgn, tk, cr, bolt);
		
	}


    //original call to WDRC_circuit
    //void WDRC_circuit(float *x, float *y, float *pdb, int n, float tkgn, float tk, float cr, float bolt)
    //void WDRC_circuit(float *orig_signal, float *signal_out, float *env_dB, int n, float tkgn, float tk, float cr, float bolt)
    //modified to output just the gain instead of the fully processed signal
    void WDRC_circuit_gain(float *env_dB, float *gain_out, const int n,
        const float exp_cr, const float exp_end_knee,
        const float tkgn, const float tk, const float cr, const float bolt) 
		//exp_cr = compression ratio for expansion
		//exp_end_knee = kneepoint for end of the expansion region
    	//tkgn = gain (dB?) at start of compression (ie, gain for linear behavior?)
    	//tk = compression start kneepoint (pre-compression, dB SPL?)
    	//cr = compression ratio
    	//bolt = broadband output limiting threshold (post-compression, dB SPL?)
    {
      float gdb, tkgo, pblt;
      int k;
      float *pdb = env_dB; //just rename it to keep the code below unchanged (input SPL dB)
      float tk_tmp = tk;   //temporary, threshold for start of compression (input SPL dB)
      
      if ((tk_tmp + tkgn) > bolt) { //after gain, would the compression threshold be above the output-limitting threshold ("bolt")
          tk_tmp = bolt - tkgn;  //if so, lower the compression threshold to be the pre-gain value resulting in "bolt"
      }

      const float cr_const = ((1.0f / cr) - 1.0f); //pre-calc a constant that we'll need later
      //tkgo = tkgn + tk_tmp * (1.0f - 1.0f / cr);  //intermediate calc
	  tkgo = tkgn + tk_tmp * (-cr_const);  //intermediate calc
      pblt = cr * (bolt - tkgo); //calc input level (dB) where we need to start limiting, not just compression


      //compute gain at transition between expansion and linear/compression regions
      float gain_at_exp_end_knee = tkgn;
      if (tk_tmp < exp_end_knee) {
        gain_at_exp_end_knee  = cr_const * exp_end_knee + tkgo;
      }

      float exp_cr_const = 1.0f/max(0.01f,exp_cr) - 1.0f;
      for (k = 0; k < n; k++) {  //loop over each sample
        if (pdb[k] < exp_end_knee) {  //if below the expansion threshold, do expansion
          //expansion region.
          gdb = gain_at_exp_end_knee - ((exp_end_knee-pdb[k])*exp_cr_const); //reduce gain the farther down you are from the end of the expansion region
        } else if ((pdb[k] < tk_tmp) && (cr >= 1.0f)) {  //if below the compression threshold, go linear
            gdb = tkgn;  //we're in the linear region.  Apply linear gain.
        } else if (pdb[k] > pblt) { //we're beyond the compression region into the limitting region
            gdb = bolt + ((pdb[k] - pblt) / 10.0f) - pdb[k]; //10:1 limiting!
        } else {
            gdb = cr_const * pdb[k] + tkgo; 
        }
        gain_out[k] = undb2(gdb);
        //y[k] = x[k] * undb2(gdb); //apply the gain
      }
      last_gain = gain_out[n-1];  //hold this value, in case the user asks for it later (not needed for the algorithm)
    }
    
    void setDefaultValues(void) {
      BTNRH_WDRC::CHA_WDRC gha = {1.0f, // attack time (ms), IGNORED HERE
        50.0f,     // release time (ms), IGNORED HERE
        24000.0f,  // fs, sampling rate (Hz), IGNORED HERE
        119.0f,    // maxdB, maximum signal (dB SPL)
        1.0,       // compression ratio for lowest-SPL region (ie, expansion region)
        0.0f,      // kneepoint for end of the expansion region
        0.0f,      // tkgain, compression-start gain
        105.0f,    // tk, compression-start kneepoint
        10.0f,     // cr, compression ratio
        105.0f     // bolt, broadband output limiting threshold
      };
      //setParams(gha.maxdB, gha.tkgain, gha.cr, gha.tk, gha.bolt); //also sets calcEnvelope
      setParams_from_CHA_WDRC(&gha);
    }
    void setParams_from_CHA_WDRC(const BTNRH_WDRC::CHA_WDRC *gha) { //ignores any sample rate that is in GHA
      setParams(gha->maxdB, gha->exp_cr, gha->exp_end_knee, gha->tkgain, gha->cr, gha->tk, gha->bolt); //also sets calcEnvelope
    }
	void getParams_from_CHA_WDRC(BTNRH_WDRC::CHA_WDRC *gha) {
		gha->maxdB = maxdB; gha->exp_cr = exp_cr; gha->exp_end_knee = exp_end_knee; gha->tkgain = tkgn; gha->cr = cr; gha->tk = tk; gha->bolt = bolt;
	}
    void setParams(float _maxdB, float _exp_cr, float _exp_end_knee, float _tkgain, float _cr, float _tk, float _bolt) {
      maxdB = _maxdB;
      exp_cr = _exp_cr;
      exp_end_knee = _exp_end_knee;
      tkgn = _tkgain;
      tk = _tk;
      cr = _cr;
      bolt = _bolt;
	  
	  recomputeDerivedQuantities();
    }

	void recomputeDerivedQuantities(void) { 
		tk_tmp = tk;   //temporary, threshold for start of compression (input SPL dB)
      
		if ((tk_tmp + tkgn) > bolt) { //after gain, would the compression threshold be above the output-limitting threshold ("bolt")
          tk_tmp = bolt - tkgn;  //if so, lower the compression threshold to be the pre-gain value resulting in "bolt"
		}

		cr_const = ((1.0f / cr) - 1.0f); //pre-calc a constant that we'll need later
		//tkgo = tkgn + tk_tmp * (1.0f - 1.0f / cr);  //intermediate calc
		tkgo = tkgn + tk_tmp * (-cr_const);  //intermediate calc
		pblt = cr * (bolt - tkgo); //calc input level (dB) where we need to start limiting, not just compression

		//compute gain at transition between expansion and linear/compression regions
		gain_at_exp_end_knee = tkgn;
		if (tk_tmp < exp_end_knee) {
			gain_at_exp_end_knee  = cr_const * exp_end_knee + tkgo;
		}

		exp_cr_const = 1.0f/max(0.01f,exp_cr) - 1.0f;		
	}

    //set the linear gain of the system
    float setGain_dB(float linear_gain_dB) {
      tkgn  = linear_gain_dB;
      return getGain_dB();
    }
    //increment the linear gain
    float incrementGain_dB(float increment_dB) {
      return setGain_dB(getGain_dB() + increment_dB);
    }    


	
    float getGain_dB(void) { return tkgn;  }	//returns the linear gain of the system
	float getCurrentGain(void) { return last_gain; }
	float getCurrentGain_dB(void) { return db2(getCurrentGain()); }
    
	float setMaxdB(float32_t _maxdB) { maxdB = _maxdB; recomputeDerivedQuantities(); return maxdB;}
	float getMaxdB(void) { return maxdB; }
	float setKneeExpansion_dBSPL(float32_t _knee) { exp_end_knee = _knee; recomputeDerivedQuantities(); return exp_end_knee; }
	float getKneeExpansion_dBSPL(void) { return exp_end_knee; }
	float setExpansionCompRatio(float32_t _cr) { exp_cr = _cr; recomputeDerivedQuantities(); return exp_cr; }
	float getExpansionCompRatio(void) { return exp_cr; }
	float setKneeCompressor_dBSPL(float32_t _knee) { tk = _knee; recomputeDerivedQuantities(); return tk; }
	float getKneeCompressor_dBSPL(void) { return tk; }
	float setCompRatio(float32_t _cr) { cr = _cr; recomputeDerivedQuantities(); return cr;}
	float getCompRatio(void) { return cr; }
	float setKneeLimiter_dBSPL(float32_t _bolt) { bolt = _bolt; recomputeDerivedQuantities(); return bolt; }
	float getKneeLimiter_dBSPL(void) { return bolt; }

    //dB functions.  Feed it the envelope amplitude (not squared) and it computes 20*log10(x) or it does 10.^(x/20)
    static float undb2(const float &x)  { return expf(0.11512925464970228420089957273422f*x); } //faster:  exp(log(10.0f)*x/20);  this is exact
    static float db2(const float &x)  { return 6.020599913279623f*log2f_approx(x); } //faster: 20*log2_approx(x)/log2(10);  this is approximate

    /* ----------------------------------------------------------------------
    ** Fast approximation to the log2() function.  It uses a two step
    ** process.  First, it decomposes the floating-point number into
    ** a fractional component F and an exponent E.  The fraction component
    ** is used in a polynomial approximation and then the exponent added
    ** to the result.  A 3rd order polynomial is used and the result
    ** when computing db20() is accurate to 7.984884e-003 dB.
    ** ------------------------------------------------------------------- */
    //https://community.arm.com/tools/f/discussions/4292/cmsis-dsp-new-functionality-proposal/22621#22621
    static float log2f_approx(float X) {
      //float *C = &log2f_approx_coeff[0];
      float Y;
      float F;
      int E;
    
      // This is the approximation to log2()
      F = frexpf(fabsf(X), &E);
      //  Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
      Y = 1.23149591368684f; //C[0]
      Y *= F;
      Y += -4.11852516267426f;  //C[1]
      Y *= F;
      Y += 6.02197014179219f;  //C[2]
      Y *= F;
      Y += -3.13396450166353f; //C[3]
      Y += E;
    
      return(Y);
    }

  private:
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    float maxdB, exp_cr, exp_end_knee, tkgn, tk, cr, bolt;
	float tk_tmp, cr_const, tkgo, pblt, gain_at_exp_end_knee, exp_cr_const;
	float last_gain = 1.0;  //what was the last gain value computed for the signal
};

#endif

