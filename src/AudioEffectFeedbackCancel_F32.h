
/*
   AudioEffectFeedbackCancel_F32

   Created: Chip Audette, OpenAudio May 2018
   Purpose: Adaptive feedback cancelation.  Algorithm from Boys Town National Research Hospital
       BTNRH at: https://github.com/BoysTownorg/chapro

   This processes a single stream fo audio data (ie, it is mono)

   MIT License.  use at your own risk.
*/

#ifndef _AudioEffectFeedbackCancel_F32
#define _AudioEffectFeedbackCancel_F32

#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html
#include <AudioStream_F32.h>
#include <BTNRH_WDRC_Types.h> //from Tympan_Library
#include <Arduino.h>  //for Serial.println()

#ifndef MAX_AFC_FILT_LEN
#define MAX_AFC_FILT_LEN  256  //must be longer than afl
#endif

class AudioEffectFeedbackCancel_F32 : public AudioStream_F32
{
    //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
    //GUI: shortName: FB_Cancel
  public:
    //constructor
    AudioEffectFeedbackCancel_F32(void) : AudioStream_F32(1, inputQueueArray_f32){
      setDefaultValues();
      initializeStates();
      initializeRingBuffer();
    }
    AudioEffectFeedbackCancel_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) {
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
        Serial.println(F("AudioEffectFeedbackCancel_F32: *** ERROR ***"));
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
      afl = min(max(_afl,1),MAX_AFC_FILT_LEN);

      //clear out the upper coefficients
      if (afl < MAX_AFC_FILT_LEN) {
          for (int i=afl; i<MAX_AFC_FILT_LEN; i++) { 
            efbp[i] = 0.0;
          };
      }

      return  afl;
    };
    virtual int getAfl(void) { return afl;};

    //int setNCoeffToZero(int _n_coeff_to_zero) { return n_coeff_to_zero = min(max(_n_coeff_to_zero,0),MAX_AFC_FILT_LEN); }
    //int getNCoeffToZero(void) { return n_coeff_to_zero; };
    
    virtual void setEnable(bool _enable) { enable = _enable; };
    virtual bool getEnable(void) { return enable;};

    //ring buffer
    //static const int max_afc_ringbuff_len = MAX_AFC_FILT_LEN;
    static const int max_afc_ringbuff_len = 2*MAX_AFC_FILT_LEN;
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
      for (int i = 0; i < MAX_AFC_FILT_LEN; i++) efbp[i] = 0.0;
    }

    //here's the method that is called automatically by the Teensy Audio Library
    virtual void update(void) {
     
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
      if ((in_block->id > 100) && (newest_ring_audio_block_id > 0)) { //ignore startup period
        if (abs(in_block->id - newest_ring_audio_block_id) > 1) {  //is the difference more than one block counter?
          //the data in the ring buffer is older than expected!
          Serial.print("AudioEffectFeedbackCancel_F32: falling behind?  in_block = ");
          Serial.print(in_block->id); Serial.print(", ring block = "); Serial.println(newest_ring_audio_block_id);
        }
      }

      //do the work
      if (enable) {
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

    virtual void cha_afc(float32_t *x, //input audio array
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
  
    virtual void addNewAudio(audio_block_f32_t *in_block) {
      newest_ring_audio_block_id = in_block->id; 
      addNewAudio(in_block->data, in_block->length);
    }
    virtual void addNewAudio(float *x, //input audio block
                       int cs)   //number of samples in this audio block
    {
      int Isrc, Idst;
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


  protected:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enable = true;

    //AFC parameters
    float32_t mu;    // AFC scale factor for how fast the filter adapts (bigger is faster)
    float32_t rho;   // AFC averaging factor for estimating audio envelope (bigger is longer averaging)
    float32_t eps;   // AFC when estimating audio level, this is the min value allowed (avoid divide-by-near-zero)
    int afl;         // AFC adaptive filter length
    //int n_coeff_to_zero;  //number of the first AFC filter coefficients to artificially zero out (debugging)

    //AFC states
    float32_t pwr;   // AFC estimate of error power...a state variable
    float32_t efbp[MAX_AFC_FILT_LEN];  //vector holding the estimated feedback impulse response
    float32_t foo_float_array[MAX_AFC_FILT_LEN];

};  //end class definition


class AudioEffectFeedbackCancel_LoopBack_F32 : public AudioStream_F32
{
    //GUI: inputs:1, outputs:0  //this line used for automatic generation of GUI node
    //GUI: shortName: FB_Cancel_LoopBack
  public:
    //constructor
    AudioEffectFeedbackCancel_LoopBack_F32(void) : AudioStream_F32(1, inputQueueArray_f32) { }
    AudioEffectFeedbackCancel_LoopBack_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32) { };

    void setTargetAFC(AudioEffectFeedbackCancel_F32 *_afc) {
      AFC_obj = _afc;
    }

    //here's the method that is called automatically by the Teensy Audio Library
    void update(void) {
      if (AFC_obj == NULL) return;

      //receive the input audio data
      audio_block_f32_t *in_block = AudioStream_F32::receiveReadOnly_f32();
      if (!in_block) return;

      //do the work
      AFC_obj->addNewAudio(in_block);

      //release memory
      AudioStream_F32::release(in_block);
    }

  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    AudioEffectFeedbackCancel_F32 *AFC_obj;
};

#endif
