
/*
   AudioFeedbackCancelNFXLMS_F32

   Created: Chip Audette, OpenAudio May 2018
   Purpose: Adaptive feedback cancelation.  Algorithm from Boys Town National Research Hospital
       BTNRH at: https://github.com/BoysTownorg/chapro

   This processes a single stream fo audio data (ie, it is mono)

   MIT License.  use at your own risk.
*/

#ifndef _AudioFeedbackCancelNFXLMS_F32
#define _AudioFeedbackCancelNFXLMS_F32

#include <Arduino.h>  //for Serial.println()
#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include "AudioStream_F32.h"  //for MAX_AUDIO_BLOCK_SAMPLES_F32
//include "BTNRH_WDRC_Types.h" //from Tympan_Library
#include "AudioLoopback_F32.h"


#ifndef MAX_AFC_NXFXLMS_FILT_LEN
#define MAX_AFC_NXFXLMS_FILT_LEN  (256)  //must be longer than afl
#endif


class settings_AFC_NFXLMS {
  public:
    float rho;   //forgetting factor...averaging factor for estimating audio envelope (bigger is longer averaging)
    float eps;   //when estimating audio level, this is the min value allowed (avoid divide-by-near-zero)
    float mu;    //AFC scale factor for how fast the filter adapts (bigger is faster)
    float alf;   //band-limit update
    int afl;    //adaptive filter length (42 for 24kHz sample rate)
    int wfl;     //whiten filter length (9 at 24 kHz)
    int pfl;     //band-limit filter length (20 at 24 kHz)
    //int fbl;     //simulated feedback length (this should always be zero?)
    int hdel;    //output/input hardware delay...this is specific to Tympan AIC + Tympan Audio Library
    int pup;     //band-limit update period (what units?)
    //int sqm
};



class AudioFeedbackCancelNFXLMS_F32 : public AudioStream_F32, public AudioLoopBackInterface_F32 {
//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
//GUI: shortName: FB_Cancel_NFXLMS
  public:
    //constructor
    AudioFeedbackCancelNFXLMS_F32(void) : AudioStream_F32(1, inputQueueArray_f32) {
      audio_block_samples = MAX_AUDIO_BLOCK_SAMPLES_F32;
      sample_rate_Hz = AUDIO_SAMPLE_RATE;
      useDefaultSettings(sample_rate_Hz, audio_block_samples);
      cha_afc_prepare();
      initializeStates();
      //initializeRingBuffer();
    }
    AudioFeedbackCancelNFXLMS_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {
      sample_rate_Hz = settings.sample_rate_Hz; //used to set default values
      audio_block_samples = settings.audio_block_samples; //used to set default values
      cs = audio_block_samples;
      useDefaultSettings(sample_rate_Hz, audio_block_samples);
      cha_afc_prepare();
      initializeStates();
      //initializeRingBuffer();

      //if you need the sample rate, it is: fs_Hz = settings.sample_rate_Hz;
      //if you need the block size, it is: n = settings.audio_block_samples;
    };

    //simply get a copy of the default settings, don't actually use them
    static settings_AFC_NFXLMS getDefaultSettings(float sample_rate_Hz, int audio_block_samples) {
      settings_AFC_NFXLMS settings;
      settings.rho = 0.0072119585;  //forgetting factor
      settings.eps = 0.000919300;   //minimum power threshold (for avoiding divide-by-zero during quiet passages)
      settings.mu = 0.004607254;    //step size
      settings.alf = 0.000010658;   //band-limit update
      settings.afl = (int)42.0f * (sample_rate_Hz / 24000.f);  //adaptive filter length (42 for 24kHz sample rate)
      settings.wfl = (int)9.0f * (sample_rate_Hz / 24000.f);   //whiten filter length (9 at 24 kHz)
      settings.pfl = (int)20.0f * (sample_rate_Hz / 24000.f);  //band-limit filter length (20 at 24 kHz)
      //settings.fbl = 0;               //simulated feedback length (this should always be zero?)
      settings.hdel = 38 + 2 * audio_block_samples; //output/input hardware delay...this is specific to Tympan AIC (TI 3206) + Tympan Audio Library
      settings.pup = 8;               //band-limit update period (what units?)
      return settings;
    }

    //configure the algorithm to use the default settings
    virtual settings_AFC_NFXLMS useDefaultSettings(float fs_Hz, int block_size_samps) {  //sample rate and block size
      settings_AFC_NFXLMS settings = getDefaultSettings(fs_Hz, block_size_samps);
      return setParams(settings);
      //setParams(_rho, _eps, _mu, _alf, _afl, _wfl, _pfl, _fbl, _hdel, _pup);
    }

    virtual settings_AFC_NFXLMS setParams(settings_AFC_NFXLMS &settings) {
      setRho(settings.rho);      setEps(settings.eps);      setMu(settings.mu);
      setALF(settings.alf);      setAFL(settings.afl);      setWFL(settings.wfl);
      setPFL(settings.pfl);  
      //setFBL(settings.fbl);      
      setHDel(settings.hdel);    setPUP(settings.pup);
      
      return getSettings();
    }

    virtual settings_AFC_NFXLMS getSettings(void) {
      settings_AFC_NFXLMS settings;
      settings.rho = rho;      settings.eps = eps;      settings.mu = mu;
      settings.alf = alf;      settings.afl = afl;      settings.wfl = wfl;
      settings.pfl = pfl;      
      //settings.fbl = fbl;      
      settings.hdel = hdel;    settings.pup = pup;
      
      return settings;
    }

    virtual float setRho(float _rho) {  return rho = min(max(_rho, 0.0), 1.0); }
    virtual float getRho(void) { return rho;};
    virtual float setEps(float _eps) {  return eps = min(max(_eps, 1e-30), 1.0); };
    virtual float getEps(void) { return eps;};
    virtual float setMu(float _mu) { return mu = max(0.0,_mu); }
    virtual float getMu(void) {  return mu; };
    virtual int setALF(int _alf) { return alf = max(0,_alf); }
    virtual int getALF(void) { return alf; }
    virtual int setAFL(int _afl) {
      //apply limits on the input value
      afl = min(max(_afl, 1), min(MAX_AFC_NXFXLMS_FILT_LEN, MAX_RSZ)); //sets afl for this instance of the class

      //change values that depend upon afl..."maxl length" and "ring buffer size"
      int fbl = 0;  //assume simulated feedback is always zero (this was used by BTNRH for development only, I think)
      mxl = computeNewMaxLength(fbl,afl,wfl,pfl);  //sets mxl for this instance of the class
      rsz = computeNewRingbufferSize(mxl,hdel,cs); //sets rsx for this instance of the class

      //should we clear out the ring buffers if the afl has changed???

      //clear out the upper coefficients that are no longer used
      if (afl < MAX_AFC_NXFXLMS_FILT_LEN) { for (int i = afl; i < MAX_AFC_NXFXLMS_FILT_LEN; i++) efbp[i] = 0.0; }
      return  afl;
    };
    virtual int getAFL(void) {  return afl; }
    virtual int setWFL(int _wfl) { 
      wfl = max(0,_wfl);

      //change values that depend upon wfl..."maxl length" and "ring buffer size"
      int fbl = 0;  //assume simulated feedback is always zero (this was used by BTNRH for development only, I think)
      mxl = computeNewMaxLength(fbl,afl,wfl,pfl);  //sets mxl for this instance of the class
      rsz = computeNewRingbufferSize(mxl,hdel,cs); //sets rsx for this instance of the class 
      return wfl;
    };
    virtual int getWFL(void) { return wfl; }
    virtual int setPFL(int _pfl) { 
      pfl = max(0,_pfl); 

      //change values that depend upon pfl..."maxl length" and "ring buffer size"
      int fbl = 0;  //assume simulated feedback is always zero (this was used by BTNRH for development only, I think)
      mxl = computeNewMaxLength(fbl,afl,wfl,pfl);  //sets mxl for this instance of the class
      rsz = computeNewRingbufferSize(mxl,hdel,cs); //sets rsx for this instance of the class 

      return pfl;
    }  
    virtual int getPFL(void) { return pfl; }
    //virtual int setFBL(int _fbl) { return fbl = max(0, _fbl); }
    //virtual int getFBL(void) { return fbl; }
    virtual int setHDel(int _hdel) { 
      hdel = max(0, _hdel); 

      //change values that depend upon hdel..."maxl length" and "ring buffer size"
      int fbl = 0;  //assume simulated feedback is always zero (this was used by BTNRH for development only, I think)
      mxl = computeNewMaxLength(fbl,afl,wfl,pfl);  //sets mxl for this instance of the class
      rsz = computeNewRingbufferSize(mxl,hdel,cs); //sets rsx for this instance of the class 

      return hdel;
    }
    virtual int getHDel(void) { return hdel; }
    virtual int setPUP(int _pup) { return pup = max(0, _pup); }
    virtual int getPUP(void) { return pup; }

    virtual bool enable(void) { return enable(true); }
    virtual bool enable(bool _enabled) { return enabled = _enabled; }
    virtual bool setEnable(bool _enabled) { return enable(_enabled); }
    virtual bool getEnable(void) {  return enabled; };

    //initializeStates
    virtual void initializeStates(void) {
      pwr = 0.0;
      for (int i = 0; i < MAX_AFC_NXFXLMS_FILT_LEN; i++) efbp[i] = 0.0;
      for (int i = 0; i < MAX_RSZ; i++) rng0[i] = 0.0;
      for (int i = 0; i < MAX_RSZ; i++) rng1[i] = 0.0;
      for (int i = 0; i < MAX_RSZ; i++) rng2[i] = 0.0;
      for (int i = 0; i < MAX_RSZ; i++) rng3[i] = 0.0;

      //should we also clear the arrays for wfrp and ffrp?
    }

    //here's the method that is called automatically by the Teensy Audio Library
    virtual void update(void);
	virtual void cha_afc_prepare(void);
	virtual int cha_afc_input(float32_t *x, float32_t *y, int cs);
	virtual void cha_afc_output(float *x, int cs);

 

    static int computeNewMaxLength(int _fbl, int _afl, int _wfl, int _pfl) {
      int _mxl = (_fbl > _afl) ? _fbl : (_afl > _wfl) ? _afl : (_wfl > _pfl) ? _wfl : _pfl; //why so complicated?
      return _mxl;
    }
    static int computeNewRingbufferSize(int _mxl, int _hdel, int _cs) {
      int _rsz = 32;  //why start with this?  why not just compute it directly?
      while (_rsz < (_mxl + _hdel + _cs))  _rsz *= 2;  //original...needs to be a factor of two for "mask" to work (see later code)
      //_rsz = max(_rsz, (_mxl + _hdel + _cs)); //chip's replacement     
      
      if (_rsz > MAX_RSZ) {
        Serial.println("AudioFeedbackCancelNFXLMS_F32: computeNewRingbufferSize: *** WARNING *** ");
        Serial.println("   : desires a ring buffer size of " + String(_rsz) + " but only returning " + String(MAX_RSZ));
        Serial.println("   : continuing anyway...");
        _rsz = MAX_RSZ;        
      }       
      return _rsz;
    }
    static void white_filt(float *h, int n);  //initialize the whitening filter
    
    virtual void receiveLoopBackAudio(audio_block_f32_t *in_block) {
      newest_ring_audio_block_id = in_block->id;
      receiveLoopBackAudio(in_block->data, in_block->length);
    }
    virtual void receiveLoopBackAudio(float *x, //input audio block
                             int cs)   //number of samples in this audio block
    {
      cha_afc_output(x, cs);
    }


    virtual void printEstimatedFeedbackImpulseResponse(void) {
      printEstimatedFeedbackImpulseResponse(&Serial, false);
    }
    virtual void printEstimatedFeedbackImpulseResponse(bool flag) {
      printEstimatedFeedbackImpulseResponse(&Serial, flag);
    }
    virtual void printEstimatedFeedbackImpulseResponse(Print *p) {
      printEstimatedFeedbackImpulseResponse(p, false);
    }
    virtual void printEstimatedFeedbackImpulseResponse(Print *p, bool flag_eachOnNewLine) {
      p->println("AudioEffectFeedbacCancel_F32: estimated feedback impulse response:");
      float scale = 1.0;
      if (flag_eachOnNewLine) scale = 20.0;
      for (int i = 0; i < afl; i++) {
        p->print(efbp[i]*scale, 5);
        if (flag_eachOnNewLine) {
          p->println();
        } else {
          p->print(", ");
        }
      }
      if (!flag_eachOnNewLine) p->println();
    }

    virtual void printAlgorithmInfo(void) {
      Serial.println("AudioFeedbackCancelNFXLMS_F32: parameter values...");
      Serial.println("    rsz = " + String(rsz));
      Serial.println("    puc = " + String(puc));
      Serial.println("    rho = " + String(rho,6));
      Serial.println("    eps = " + String(eps,6));
      Serial.println("    mu = " + String(mu,6));
      Serial.println("    alf = " + String(alf,6));
      Serial.println("    afl = " + String(afl));
      Serial.println("    wfl = " + String(wfl));
      Serial.println("    pfl = " + String(pfl));
      Serial.println("    hdel = " + String(hdel));
      Serial.println("    pup = " + String(hdel));
      Serial.println("    pwr = " + String(pwr,6));
    }
  protected:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enabled = true;
    float sample_rate_Hz; //set in constructor
    int audio_block_samples; //set in constructor
    static const int MAX_RSZ = 2 * MAX_AFC_NXFXLMS_FILT_LEN;
    //static const int max_afc_ringbuff_len = MAX_RSZ;

    //AFC parameters
    int cs;      //chunk size (aka, audio block size)
    int mxl;     //max length 
    int rsz;     //ring buffer size?
    int puc = 0; // ???
    float rho;   //forgetting factor...averaging factor for estimating audio envelope (bigger is longer averaging)
    float eps;   //when estimating audio level, this is the min value allowed (avoid divide-by-near-zero)
    float mu;    //AFC scale factor for how fast the filter adapts (bigger is faster)
    float alf;   //band-limit update
    int afl;     //adaptive filter length (42 for 24kHz sample rate)
    int wfl;     //whiten filter length (9 at 24 kHz)
    int pfl;     //band-limit filter length (20 at 24 kHz)
    //int fbl;     //simulated feedback length (this should always be zero?)
    int hdel;    //output/input hardware delay...this is specific to Tympan AIC + Tympan Audio Library
    int pup;     //band-limit update period (what units?)
    
    //AFC states
    float32_t pwr;           // AFC estimate of error power...a state variable
    float32_t efbp[MAX_RSZ];  //vector holding the estimated feedback impulse response
    //float32_t sfbp[MAX_AFC_NXFXLMS_FILT_LEN];   //simulated-feedback buffer (length is fbl)
    float32_t wfrp[MAX_AFC_NXFXLMS_FILT_LEN];   //whitening-feedback buffer (length is wfl)
    float32_t ffrp[MAX_AFC_NXFXLMS_FILT_LEN];   //persistent-feedback buffer (length is pfl)
    //float32_t merr[xxxxx];  //chunk-error buffer (aka. block-error buffer)
    //float32_t qm[xxxxx];    //quality-meric buffer
    //int nqm;                //quality metric buffer size
    //int iqm;                //quality metric index
    //int sqm;                //save quality metric
 
    //ring buffer stuff
    int rhd, rtl;
    unsigned long newest_ring_audio_block_id = 999999;
    float32_t rng0[MAX_RSZ], rng1[MAX_RSZ], rng2[MAX_RSZ], rng3[MAX_RSZ];  //ring buffers
 

};  //end class definition


#endif
