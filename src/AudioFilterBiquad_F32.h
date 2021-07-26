/*
 * AudioFilterBiquad_F32
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 *
 * License: MIT License.  Use at your own risk.
 * 
 */

#ifndef _filter_iir_f32
#define _filter_iir_f32

#include <Arduino.h>
#include <arm_math.h>
#include "AudioStream_F32.h"


// Indicates that the code should just pass through the audio
// without any filtering (as opposed to doing nothing at all)
#define IIR_F32_PASSTHRU ((const float32_t *) 1)

#define IIR_MAX_STAGES 4  //meaningless right now

class AudioFilterBiquad_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName:IIR
  public:
    AudioFilterBiquad_F32(void): AudioStream_F32(1,inputQueueArray), coeff_p(IIR_F32_PASSTHRU) { 
		setSampleRate_Hz(AUDIO_SAMPLE_RATE_EXACT);
		clearCoeffArray();		
	}
	AudioFilterBiquad_F32(const AudioSettings_F32 &settings): 
		AudioStream_F32(1,inputQueueArray), coeff_p(IIR_F32_PASSTHRU) {
			setSampleRate_Hz(settings.sample_rate_Hz); 
	}

    virtual void begin(const float32_t *cp, int n_stages = 1) {
      coeff_p = cp;
      // Initialize Biquad instance (ARM DSP Math Library)
      if (coeff_p && (coeff_p != IIR_F32_PASSTHRU) && n_stages <= IIR_MAX_STAGES) {
        //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html
        arm_biquad_cascade_df1_init_f32(&iir_inst, n_stages, (float32_t *)coeff_p,  &StateF32[0]);
      }
    }
    virtual void end(void) {
      coeff_p = NULL;
    }
	
	virtual void clearCoeffArray(void) {
		for (int i=0; i<IIR_MAX_STAGES*5;i++) coeff[i]=0.0;
		coeff[0]=1.0f;  //makes this be a simple pass-thru
	}
	
	virtual float32_t getSampleRate_Hz(void) { return sampleRate_Hz; }
	virtual void setSampleRate_Hz(float32_t _fs_Hz) { sampleRate_Hz = _fs_Hz; }
    
    virtual void setBlockDC(void) {
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff for HP at 40Hz: [b,a]=butter(2,40/(44100/2),'high'); %assumes fs_Hz = 44100
      float32_t b[] = {8.173653471988667e-01,  -1.634730694397733e+00,   8.173653471988667e-01};  //from Matlab
      float32_t a[] = { 1.000000000000000e+00,   -1.601092394183619e+00,  6.683689946118476e-01};  //from Matlab
      setFilterCoeff_Matlab(b, a);
    }
    
    virtual void setFilterCoeff_Matlab(float32_t b[], float32_t a[]) { //one stage of N=2 IIR
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff, such as: [b,a]=butter(2,20/(44100/2),'high'); %assumes fs_Hz = 44100
      coeff[0] = b[0];   coeff[1] = b[1];  coeff[2] = b[2]; //here are the matlab "b" coefficients
      coeff[3] = -a[1];  coeff[4] = -a[2];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
      begin(coeff,1);
    }

    virtual void setFilterCoeff_Matlab_sos(float32_t sos[], int n_sos) { //n_sos second-order sections of IIR
      //https://www.keil.com/pack/doc/CMSIS/DSP/html/group__BiquadCascadeDF1.html#ga8e73b69a788e681a61bccc8959d823c5
      //Use matlab to compute the coeff, such as: 
	  //   fs_Hz = 44100;  %sample rate of the signal to be processed
	  //   N_IIR = 3;      %order of the IIR filter 
	  //   bp_Hz = [1000 2000];    %define the desired cutoff frequencies [low, high] of the bandpass filter in Hz
	  //   [b,a]=butter(N_IIR,bp_Hz/(fs_Hz/2)));  %creates bandpass filter, but not yet a second-order sections
	  //   [sos]=tf2sos(b,a);  %convert to second order section  (no gain term...try to add that into this code later)
	  int start_ind;
	  for (int i=0; i < min(n_sos,IIR_MAX_STAGES); i++) {
		  start_ind = i*5;
		  coeff[start_ind+0] = sos[i*6+0];   
		  coeff[start_ind+1] = sos[i*6+1];  
		  coeff[start_ind+2] = sos[i*6+2];  
		  //sos[i][3];  //the DSP data structure skips over this because it should because should be 1.0 (ie, it is a[0])
		  coeff[start_ind+3] = -sos[i*6+4];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
		  coeff[start_ind+4] = -sos[i*6+5];  //the DSP needs the "a" terms to have opposite sign vs Matlab ;
	  }
      begin(coeff,n_sos);
    }
	
	
	// //////////////////////// From Audio EQ Cookbook
	
	//This setCoefficients method sets the coefficients given the equations below from the AudioEQ Cookbook
	//note: stage is currently ignored
	virtual void setCoefficients(int stage, float32_t c[]) {
		if (stage > 0) {
			if (Serial) {
				Serial.println(F("AudioFilterBiquad_F32: setCoefficients: *** ERROR ***"));
				Serial.print(F("    : This module only accepts one stage."));
				Serial.print(F("    : You are attempting to set stage "));Serial.print(stage);
				Serial.print(F("    : Ignoring this filter."));
			}
			return;
		}
		coeff[0] = c[0];
		coeff[1] = c[1];
		coeff[2] = c[2];
		coeff[3] = -c[3];  //notice the sign flip!  from Matlab convention to ARM convention
		coeff[4] = -c[4]; //notice the sign flip!  from Matlab convention to ARM convention
		begin(coeff);
	}
	
	// Compute common filter functions...all second order filters...all with Matlab convention on a1 and a2 coefficients
	// http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	void calcLowpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcHighpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcBandpass(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcNotch(float32_t freq_Hz, float32_t q, float32_t *c);
	void calcLowShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *c);
	void calcHighShelf(float32_t freq_Hz, float32_t gain, float32_t slope, float32_t *c);
	
	//set the filter coefficients without the caller having to explicitly handle the coefficients
	void setLowpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcLowpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setHighpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcHighpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setBandpass(uint32_t stage, float32_t freq_Hz, float32_t q = 0.7071) {
		calcBandpass(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setNotch(uint32_t stage, float32_t freq_Hz, float32_t q = 1.0) {
		calcNotch(freq_Hz, q, coeff);
		setCoefficients(stage,coeff);
	}
	void setLowShelf(uint32_t stage, float32_t freq_Hz, float32_t gain, float32_t slope = 1.0f) {
		calcLowShelf(freq_Hz, gain, slope, coeff);
		setCoefficients(stage,coeff);
	}
	void setHighShelf(uint32_t stage, float32_t freq_Hz, float32_t gain, float32_t slope = 1.0f) {
		calcHighShelf(freq_Hz, gain, slope, coeff);
		setCoefficients(stage,coeff);
	}
    
    virtual void update(void);
	virtual float32_t getCutoffFrequency_Hz(void) { return cutoff_Hz; }
   
  protected:
    audio_block_f32_t *inputQueueArray[1];
    float32_t coeff[5 * IIR_MAX_STAGES]; //no filtering. actual filter coeff set later
	float32_t sampleRate_Hz = AUDIO_SAMPLE_RATE_EXACT; //default.  from AudioStream.h??
	float32_t cutoff_Hz = -999;
  
    // pointer to current coefficients or NULL or FIR_PASSTHRU
    const float32_t *coeff_p;
  
    // ARM DSP Math library filter instance
    arm_biquad_casd_df1_inst_f32 iir_inst;
    float32_t StateF32[4*IIR_MAX_STAGES];
};



#endif


