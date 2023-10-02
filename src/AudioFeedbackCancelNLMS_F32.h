
/*
   AudioFeedbackCancelNLMS_F32

   Created: Chip Audette, OpenAudio May 2018
   Purpose: Adaptive feedback cancelation.  Algorithm from Boys Town National Research Hospital
       BTNRH at: https://github.com/BoysTownorg/chapro

   This processes a single stream fo audio data (ie, it is mono)

   MIT License.  use at your own risk.
*/

#ifndef _AudioFeedbackCancelNLMS_F32
#define _AudioFeedbackCancelNLMS_F32

#include <Arduino.h>  //for Serial.println()
#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include "AudioStream_F32.h"
#include "BTNRH_WDRC_Types.h" //from Tympan_Library
#include "AudioLoopBack_F32.h" //form Tympan_Library


#ifndef MAX_AFC_NLMS_FILT_LEN
#define MAX_AFC_NLMS_FILT_LEN  256  //must be longer than afl
#endif

class AudioFeedbackCancelNLMS_F32 : public AudioStream_F32, public AudioLoopBackInterface_F32
{
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName: FB_Cancel_NLMS
  public:
    //constructor
    AudioFeedbackCancelNLMS_F32(void) : AudioStream_F32(1, inputQueueArray_f32){
      setDefaultValues();
      initializeStates();
      initializeRingBuffer();
    }
    AudioFeedbackCancelNLMS_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {
      //do any setup activities here
      setDefaultValues();
      initializeStates();
      initializeRingBuffer();

      //if you need the sample rate, it is: fs_Hz = settings.sample_rate_Hz;
      //if you need the block size, it is: n = settings.audio_block_samples;
    };

  //Daniel, go ahead and change these as you'd like!
    virtual void setDefaultValues(void) {
      //set default values...taken from BTNRH tst_iffb.c
      float _mu = 1.E-3;
      float _rho = 0.9;  //was 0.984, then Neely 2018-05-07 said try 0.9
      float _eps = 0.008;  //was 1e-30, then Neely 2018-05-07 said try 0.008
      int _afl = 100; //was 100.  For 24kHz sample rate
      //n_coeff_to_zero = 0;
      setParams(_mu, _rho, _eps, _afl);
    }
    virtual void setParams(BTNRH_WDRC::CHA_AFC cha) {
      setParams(cha.mu, cha.rho, cha.eps, cha.afl);
      //setNCoeffToZero(0);
      setEnable(cha.default_to_active);
    }
    virtual void setParams(float _mu, float _rho, float _eps, int _afl) {
      //AFC parameters
      setMu(_mu);     // AFC step size
      setRho(_rho);   // AFC forgetting factor
      setEps (_eps);   // AFC tolerance for setting a floor on the smallest signal level (thereby avoiding divide-by-near-zero)
      setAfl(_afl);  // AFC adaptive filter length
      if (_afl > max_afc_ringbuff_len) {
        Serial.println(F("AudioFeedbackCancelNLMS_F32: *** ERROR ***"));
        Serial.print(F("    : Adaptive filter length (")); Serial.print(afl);
        Serial.print(F(") too long for ring buffer (")); Serial.print(max_afc_ringbuff_len); Serial.println(")");
        afl = setAfl(max_afc_ringbuff_len);
        Serial.print(F("    : Limiting filter length to ")); Serial.println(afl);
      }
    }

    virtual float setMu(float _mu) { return mu = _mu; }
    virtual float setRho(float _rho) { return rho = min(max(_rho,0.0),1.0); };
    virtual float setEps(float _eps) { return eps = min(max(_eps,1e-30),1.0); };
    virtual float getMu(void) { return mu; };
    virtual float getRho(void) { return rho; };
    virtual float getEps(void) { return eps; };
    virtual int setAfl(int _afl) { 
      //apply limits on the input value
      afl = min(max(_afl,1),MAX_AFC_NLMS_FILT_LEN);

      //clear out the upper coefficients
      if (afl < MAX_AFC_NLMS_FILT_LEN) {
          for (int i=afl; i<MAX_AFC_NLMS_FILT_LEN; i++) { 
            efbp[i] = 0.0;
          };
      }

      return  afl;
    };
    virtual int getAfl(void) { return afl;};

    //int setNCoeffToZero(int _n_coeff_to_zero) { return n_coeff_to_zero = min(max(_n_coeff_to_zero,0),MAX_AFC_NLMS_FILT_LEN); }
    //int getNCoeffToZero(void) { return n_coeff_to_zero; };
    
	virtual bool enable(void) { return enable(true); }
	virtual bool enable(bool _enabled) { return enabled = _enabled; }
    virtual void setEnable(bool _enabled) { enable(_enabled); }
    virtual bool getEnable(void) { return enabled;};

    //ring buffer
    //static const int max_afc_ringbuff_len = MAX_AFC_NLMS_FILT_LEN;
    static const int max_afc_ringbuff_len = 2*MAX_AFC_NLMS_FILT_LEN;
    float32_t ring[max_afc_ringbuff_len];
    int rhd, rtl;
    unsigned long newest_ring_audio_block_id = 999999;
    void initializeRingBuffer(void) {
      rhd = 0;  rtl = 0;
      for (int i = 0; i < max_afc_ringbuff_len; i++) ring[i] = 0.0;
    }
    //int rsz = max_afc_ringbuff_len;  //"ring buffer size"...variable name inherited from original BTNRH code
    //int mask = rsz - 1;

    //initializeStates
    virtual void initializeStates(void) {
      pwr = 0.0;
      for (int i = 0; i < MAX_AFC_NLMS_FILT_LEN; i++) efbp[i] = 0.0;
	  initializeRingBuffer();
    }

	virtual void update(void);
	virtual void cha_afc(float32_t *x, float32_t *y, int cs);  //input array, output array, block (chunk) size
  
  
    virtual void receiveLoopBackAudio(audio_block_f32_t *in_block) {
      newest_ring_audio_block_id = in_block->id; 
      receiveLoopBackAudio(in_block->data, in_block->length);
    }
    virtual void receiveLoopBackAudio(float *x, int cs); //input array, block (chunk) size 
	
	
	virtual void printEstimatedFeedbackImpulseResponse(void) {
      printEstimatedFeedbackImpulseResponse(&Serial, false);
    }
	virtual void printEstimatedFeedbackImpulseResponse(bool flag) {
      printEstimatedFeedbackImpulseResponse(&Serial, flag);
    }
	virtual void printEstimatedFeedbackImpulseResponse(Print *p) {
		printEstimatedFeedbackImpulseResponse(p,false);
	}
    virtual void printEstimatedFeedbackImpulseResponse(Print *p, bool flag_eachOnNewLine) {
      p->println("AudioEffectFeedbacCancel_F32: estimated feedback impulse response:");
	  float scale = 1.0;
	  if (flag_eachOnNewLine) scale = 20.0;
      for (int i=0; i<afl; i++) { 
		p->print(efbp[i]*scale,5); 
		if (flag_eachOnNewLine) {
			p->println();
		} else {
			p->print(", ");
		}	
	  }
	  if (!flag_eachOnNewLine) p->println();
    }



    virtual void printAlgorithmInfo(void) {
      Serial.println("AudioFeedbackCancelNLMS_F32: parameter values...");
      Serial.println("    rho = " + String(rho,6));
      Serial.println("    eps = " + String(eps,6));
      Serial.println("    mu = " + String(mu,6));
      Serial.println("    afl = " + String(afl));
      Serial.println("    pwr = " + String(pwr,6));
    }

  protected:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enabled = true;

    //AFC parameters
    float32_t mu;    // AFC scale factor for how fast the filter adapts (bigger is faster)
    float32_t rho;   // AFC averaging factor for estimating audio envelope (bigger is longer averaging)
    float32_t eps;   // AFC when estimating audio level, this is the min value allowed (avoid divide-by-near-zero)
    int afl;         // AFC adaptive filter length
    //int n_coeff_to_zero;  //number of the first AFC filter coefficients to artificially zero out (debugging)

    //AFC states
    float32_t pwr;   // AFC estimate of error power...a state variable
    float32_t efbp[MAX_AFC_NLMS_FILT_LEN];  //vector holding the estimated feedback impulse response
    float32_t foo_float_array[MAX_AFC_NLMS_FILT_LEN];

};  //end class definition



#endif
