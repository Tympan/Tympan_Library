/*
 * AudioEffectCompWDRC_F32: Wide Dynamic Rnage Compressor
 * 
 * Created: Chip Audette (OpenAudio) Feb 2017
 * Derived From: WDRC_circuit from CHAPRO from BTNRC: https://github.com/BTNRH/chapro
 *     As of Feb 2017, CHAPRO license is listed as "Creative Commons?"
 * 
 * MIT License.  Use at your own risk.
 * 
 */

#ifndef _AudioEffectCompWDRC_F32
#define _AudioEffectCompWDRC_F32

class AudioCalcGainWDRC_F32;  //forward declared.  Actually defined in later header file, but I need this here to avoid circularity

#include <Arduino.h>
#include "AudioStream_F32.h"
#include <arm_math.h>
#include "AudioCalcEnvelope_F32.h"
#include "AudioCalcGainWDRC_F32.h"  //has definition of CHA_WDRC
#include "BTNRH_WDRC_Types.h"


//This class helps manage some of the configuration and state information of the AudioEffectCompWDRC_F32 classes.
//By having this class, it tries to put everything in one place 
//It is also helpful for managing the GUI on the TympanRemote mobile App.
class AudioCompWDRCState {
	public:
		AudioCompWDRCState(void) { };
		~AudioCompWDRCState(void) { };
		
		//The compressor is tricky because it has many configuration parameters that I'd like to track here in
		//this State-tracking class because I want to access them via the App's GUI.  But, if I keep a local copy
		//of the parameter values, I need to do a lot of management to ensure that they are in-sync with the underlying
		//audio processing classes.  
		//
		//So, instead, I'm going to just get the values from the classes themsevles.
		//
		//This means that, instead of having a bunch of state variables (int, float, whatever) here in this
		//state-tracking class, I'll have a bunch of get() methods.  I could have just asked for these values
		//directly from the AudioEffectCompWDRC_F32 class without have introducting this state-tracking class
		//as an intermediary.  I chose to introduce this class because *all* of the UI-enabled classes use some
		//sort of state-tracking class.  Also, by putting all these critical get() methods in one place here 
		//in the State, it is very clear which are the most important parameters, without getting distracted by
		//all of the other methods in the main class.
	
		//get parameter values from the compressors
		float getSampleRate_Hz(void) { return compressor->getSampleRate_Hz(); }
		float getAttack_msec(int i=0) { if (i < get_n_chan()) { return compressor->getAttack_msec(); } else { return 0.0f; }};
		float getRelease_msec(int i=0) { if (i < get_n_chan()) { return compressor->getRelease_msec(); } else { return 0.0f; }};
		float getScaleFactor_dBSPL_at_dBFS(int i=0) { if (i < get_n_chan()) { return compressor->getMaxdB(); } else { return 0.0f; }};
		float getExpansionCompRatio(int i=0) { if (i < get_n_chan()) { return compressor->getExpansionCompRatio(); } else { return 0.0f; }};
		float getKneeExpansion_dBSPL(int i=0) { if (i < get_n_chan()) { return compressor->getKneeExpansion_dBSPL(); } else { return 0.0f; }};
		float getLinearGain_dB(int i=0) { if (i < get_n_chan()) { return compressor->getGain_dB(); } else { return 0.0f; }};
		float getCompRatio(int i=0) { if (i < get_n_chan()) { return compressor->getCompRatio(); } else { return 0.0f; }};
		float getKneeCompressor_dBSPL(int i=0) { if (i < get_n_chan()) { return compressor->getKneeCompressor_dBSPL(); } else { return 0.0f; }};
		float getKneeLimiter_dBSPL(int i=0) { if (i < get_n_chan()) { return compressor->getKneeLimiter_dBSPL(); } else { return 0.0f; }};

		//These methods are not used to directly maintain the state of the AudioEffectCompWDRC.
		//They are supporting methods
		void setCompressor(AudioEffectCompWDRC_F32 &c); //also defines max_n_chan


	protected:
		AudioEffectCompWDRC_F32 *compressor;  //will be an array of pointers to our compressors
		

	protected:
//int max_n_filters = AudioFilterbank_MAX_NUM_FILTERS;  //should correspond to the length of crossover_freq_Hz
//		int n_filters = AudioFilterbank_MAX_NUM_FILTERS;      //should correspond to however many of the filters are actually being employed by the AuioFilterbank
//		float *crossover_freq_Hz;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
	
};



class AudioEffectCompWDRC_F32 : public AudioStream_F32
{
	//GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
	//GUI: shortName: CompressWDRC
  public:
    AudioEffectCompWDRC_F32(void): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
      setDefaultValues();
    }

    AudioEffectCompWDRC_F32(AudioSettings_F32 settings): AudioStream_F32(1,inputQueueArray) { //need to modify this for user to set sample rate
      setSampleRate_Hz(settings.sample_rate_Hz);
      setDefaultValues();
    }

    //here is the method called automatically by the audio library
    void update(void) {
		//receive the input audio data
		audio_block_f32_t *block = AudioStream_F32::receiveReadOnly_f32();
		if (!block) return;

		//allocate memory for the output of our algorithm
		audio_block_f32_t *out_block = AudioStream_F32::allocate_f32();
		if (!out_block) { AudioStream_F32::release(block); return; }

		//do the algorithm
		int is_error = processAudioBlock(block,out_block); //anything other than a zero is an error
		
		// transmit the block and release memory
		if (!is_error) AudioStream_F32::transmit(out_block); // send the output
		AudioStream_F32::release(out_block);
		AudioStream_F32::release(block);
    }

	//here is a standard method for executing the guts of the algorithm without having to call update()
	//This is the access point used by the compressor bank class, for example, since the compressor bank
	//handles the audio_block manipulation normally done by update()
	//
	//This method uses audio_block_f32_t as its inputs and outputs, to be consistent with all the other
	//"processAudioBlock()" methods that are used in many other of my audio-processing classes
	int processAudioBlock(audio_block_f32_t *block, audio_block_f32_t *out_block) {
		if ((!block) || (!out_block)) return -1;  //-1 is error
		
		compress(block->data, out_block->data, block->length);
		
		//copy the audio_block info
		out_block->id = block->id;
		out_block->length = block->length;
		
		return 0;  //0 is OK
	}

    //Here is the function that actually does all the work
	//This method uses simply float arrays as the inptus and outputs, so that this is maximally compatible
	//with other ways of using this class.
     void compress(float *x, float *y, int n)    
     //x, input, audio waveform data
     //y, output, audio waveform data after compression
     //n, input, number of samples in this audio block
    {        
        // find smoothed envelope
        audio_block_f32_t *envelope_block = AudioStream_F32::allocate_f32();
        if (!envelope_block) return;
        calcEnvelope.smooth_env(x, envelope_block->data, n);
        //float *xpk = envelope_block->data; //get pointer to the array of (empty) data values

        //calculate gain
        audio_block_f32_t *gain_block = AudioStream_F32::allocate_f32();
        if (!gain_block) return;
        calcGain.calcGainFromEnvelope(envelope_block->data, gain_block->data, n);
        
        //apply gain
        arm_mult_f32(x, gain_block->data, y, n);

        // release memory
        AudioStream_F32::release(envelope_block);
        AudioStream_F32::release(gain_block);
    }


    void setDefaultValues(void) {
      //set default values...taken from CHAPRO, GHA_Demo.c  from "amplify()"...ignores given sample rate
      //assumes that the sample rate has already been set!!!!
      BTNRH_WDRC::CHA_WDRC gha = {1.0f, // attack time (ms)
        50.0f,     // release time (ms)
        24000.0f,  // fs, sampling rate (Hz), THIS IS IGNORED!
        119.0f,    // maxdB, maximum signal (dB SPL)
        1.0f,      // compression ratio for lowest-SPL region (ie, the expansion region)
        0.0f,      // expansion ending kneepoint (see small to defeat the expansion)
        0.0f,      // tkgain, compression-start gain
        105.0f,    // tk, compression-start kneepoint
        10.0f,     // cr, compression ratio
        105.0f     // bolt, broadband output limiting threshold
      };
      setParams_from_CHA_WDRC(&gha);
    }

    //set all of the parameters for the compressor using the CHA_WDRC "GHA" structure
	void configureFromGHA(float fs_Hz, const BTNRH_WDRC::CHA_WDRC &gha) { setSampleRate_Hz(fs_Hz);  setParams_from_CHA_WDRC(&gha); }
	void configureFromGHA(const BTNRH_WDRC::CHA_WDRC &gha) { setParams_from_CHA_WDRC(&gha); }  //assumes that the sample rate has already been set!!!
    void setParams_from_CHA_WDRC(const BTNRH_WDRC::CHA_WDRC *gha) {  //assumes that the sample rate has already been set!!!
      //configure the envelope calculator...assumes that the sample rate has already been set!
      calcEnvelope.setAttackRelease_msec(gha->attack,gha->release); //these are in milliseconds

      //configure the compressor
      calcGain.setParams_from_CHA_WDRC(gha);
    }
	

    //set all of the user parameters for the compressor...assuming no expansion regime
    //assumes that the sample rate has already been set!!!
	void setParams(float attack_ms, float release_ms, float maxdB, float tkgain, float comp_ratio, float tk, float bolt) {
		float exp_cr = 1.0, exp_end_knee = -200.0;  //this should disable the exansion regime
		setParams(attack_ms, release_ms, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
	}

    //set all of the user parameters for the compressor...assumes that there is an expansion regime
    //assumes that the sample rate has already been set!!!
    void setParams(float attack_ms, float release_ms, float maxdB, float exp_cr, float exp_end_knee, float tkgain, float comp_ratio, float tk, float bolt) {
      
      //configure the envelope calculator...assumes that the sample rate has already been set!
      calcEnvelope.setAttackRelease_msec(attack_ms,release_ms);

      //configure the WDRC gains
      calcGain.setParams(maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
    }

    float setSampleRate_Hz(const float _fs_Hz) {
      //pass this data on to its components that care
      //given_sample_rate_Hz = _fs_Hz;
      return calcEnvelope.setSampleRate_Hz(_fs_Hz);
    }
	float getSampleRate_Hz(void) { return calcEnvelope.getSampleRate_Hz(); }

    float getCurrentLevel_dB(void) { return AudioCalcGainWDRC_F32::db2(calcEnvelope.getCurrentLevel()); }  //this is 20*log10(abs(signal)) after the envelope smoothing

    //set the linear gain of the system
    float setGain_dB(float linear_gain_dB) {
      return calcGain.setGain_dB(linear_gain_dB);
    }
    //increment the linear gain
    float incrementGain_dB(float increment_dB) {
      return calcGain.incrementGain_dB(increment_dB);
    }    
    //returns the linear gain of the system
    float getGain_dB(void) {
      return calcGain.getGain_dB();
    }
	float getCurrentGain_dB(void) { return calcGain.getCurrentGain_dB(); }
	
	void setAttackRelease_msec(float32_t attack_ms, float32_t release_ms) {
		calcEnvelope.setAttackRelease_msec(attack_ms, release_ms);
	}
	float setMaxdB(float32_t foo) { return calcGain.setMaxdB(foo); }
	float getMaxdB(void) { return calcGain.getMaxdB(); }
	float setKneeExpansion_dBSPL(float32_t _knee) { return calcGain.setKneeExpansion_dBSPL(_knee); }
	float getKneeExpansion_dBSPL(void) { return calcGain.getKneeExpansion_dBSPL(); }
	float setExpansionCompRatio(float32_t _cr) { return calcGain.setExpansionCompRatio(_cr); }
	float getExpansionCompRatio(void) { return calcGain.getExpansionCompRatio(); }
	float setKneeCompressor_dBSPL(float32_t foo) { return calcGain.setKneeCompressor_dBSPL(foo); }
	float getKneeCompressor_dBSPL(void) { return calcGain.getKneeCompressor_dBSPL(); }
	float setCompRatio(float32_t foo) { return calcGain.setCompRatio(foo); }
	float getCompRatio(void) { return calcGain.getCompRatio(); }
	float setKneeLimiter_dBSPL(float32_t foo) { return calcGain.setKneeLimiter_dBSPL(foo); }
	float getKneeLimiter_dBSPL(void) { return calcGain.getKneeLimiter_dBSPL(); }
	float getAttack_msec(void) { return calcEnvelope.getAttack_msec(); }
	float getRelease_msec(void) { return calcEnvelope.getRelease_msec(); }
	
    AudioCalcEnvelope_F32 calcEnvelope;
    AudioCalcGainWDRC_F32 calcGain;
    
	AudioCompWDRCState state;
	
  private:
    audio_block_f32_t *inputQueueArray[1];
    //float given_sample_rate_Hz;
};


#endif
    
