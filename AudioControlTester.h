
#ifndef _AudioControlTester_h
#define _AudioControlTester_h

#include <Tympan_Library.h>

//prototypes
class AudioTestSignalGenerator_F32;
class AudioTestSignalMeasurement_F32;
class AudioControlSignalTester_F32;
class AudioControlTestAmpSweep_F32;
class AudioControlTestFreqSweep_F32;

// class definitions
class AudioTestSignalGenerator_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:1  //this line used for automatic generation of GUI node
  //GUI: shortName: testSignGen
  public:
    AudioTestSignalGenerator_F32(void): AudioStream_F32(1,inputQueueArray) {
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
      setDefaultValues();
      makeConnections();
    }
    AudioTestSignalGenerator_F32(const AudioSettings_F32 &settings): AudioStream_F32(1,inputQueueArray) {
      setAudioSettings(settings);
      setDefaultValues();
      makeConnections();
    }
    ~AudioTestSignalGenerator_F32(void) {
      if (patchCord1 != NULL) delete patchCord1;
    }
    void setAudioSettings(const AudioSettings_F32 &settings) {
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    void setSampleRate_Hz(const float _fs_Hz) {
      //pass this data on to its components that care
      sine_gen.setSampleRate_Hz(_fs_Hz);
    }
    void makeConnections(void) {
      patchCord1 = new AudioConnection_F32(sine_gen, 0, gain_alg, 0);
      patchCord2 = new AudioConnection_F32(gain_alg, 0, record_queue, 0);
    }
    
    virtual void update(void);
    void begin(void) {
      is_testing = true;
      //if (Serial) Serial.println("AudioTestSignalGenerator_F32: begin(): ...");
    }
    void end(void) { is_testing = false; }
    
    AudioSynthWaveformSine_F32 sine_gen;
    AudioEffectGain_F32 gain_alg;
    AudioRecordQueue_F32 record_queue;
    AudioConnection_F32 *patchCord1;
    AudioConnection_F32 *patchCord2;

    void amplitude(float val) {
      sine_gen.amplitude(1.0);
      gain_alg.setGain(val);
    }
    void frequency(float val) {
      sine_gen.frequency(val);
    }
	
	virtual void setSignalAmplitude_dBFS(float val_dBFS) {
		amplitude(sqrtf(2.0)*sqrtf(powf(10.0f,0.1*val_dBFS)));
	};
	virtual void setSignalFrequency_Hz(float val_Hz) {
		frequency(val_Hz);
	}
	
  private:
    bool is_testing = false;
    audio_block_f32_t *inputQueueArray[1];

    void setDefaultValues(void) {
      sine_gen.end();  //disable it for now
      record_queue.end(); //disable it for now;
      is_testing = false;
      frequency(1000.f);
      amplitude(0.0f);
    }
};


// //////////////////////////////////////////////////////////////////////////
class AudioTestSignalMeasurement_F32 : public AudioStream_F32
{
  //GUI: inputs:1, outputs:0  //this line used for automatic generation of GUI node
  //GUI: shortName: testSigMeas
  public:
    AudioTestSignalMeasurement_F32(void): AudioStream_F32(2,inputQueueArray) {
      setSampleRate_Hz(AUDIO_SAMPLE_RATE);
      setDefaultValues();
    }
    AudioTestSignalMeasurement_F32(const AudioSettings_F32 &settings): AudioStream_F32(2,inputQueueArray) {
      setAudioSettings(settings);
      setDefaultValues();
    }
    void setAudioSettings(const AudioSettings_F32 &settings) {
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    void setSampleRate_Hz(const float _fs_Hz) {
      //pass this data on to its components that care
      //sine_gen.setSampleRate_Hz(_fs_Hz);
    }
    virtual void update(void);

    float computeRMS(float data[], int n) {
      float rms_value;
      arm_rms_f32 (data, n, &rms_value);
      return rms_value;
    }
    void begin(AudioControlSignalTester_F32 *p_controller) {
      //if (Serial) Serial.println("AudioTestSignalMeasurement_F32: begin(): ...");
      testController = p_controller;
      is_testing = true;
    }
    void end(void) {
      //if (Serial) Serial.println("AudioTestSignalMeasurement_F32: end(): ...");
      testController = NULL;
      is_testing = false;
    }

  private:
    bool is_testing = false;
    audio_block_f32_t *inputQueueArray[2];
    AudioControlSignalTester_F32 *testController = NULL;

    void setDefaultValues(void) {
      is_testing = false;
    }
};

// ///////////////////////////////////////////////////////////////////////////////////
#define max_steps 128
class AudioControlSignalTesterInterface_F32 {
  public:
    AudioControlSignalTesterInterface_F32(void) {};
    //virtual void setAudioBlockSamples(void) = 0;
    //virtual void setSampleRate_hz(void) = 0;
    virtual void begin(void) = 0;
    virtual void end(void) = 0;
    virtual void setStepPattern(float, float, float) = 0;
    virtual void transferRMSValues(float, float) = 0;
    virtual bool available(void) = 0;
};

// ////////////////////////////////////////////////
class AudioControlSignalTester_F32 : public AudioControlSignalTesterInterface_F32
{
  //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
  //GUI: shortName: sigTest(Abstract)
  public: 
    AudioControlSignalTester_F32(AudioSettings_F32 &settings, AudioTestSignalGenerator_F32 &_sig_gen, AudioTestSignalMeasurement_F32 &_sig_meas) 
        : AudioControlSignalTesterInterface_F32(), sig_gen(_sig_gen), sig_meas(_sig_meas) {

      setAudioBlockSamples(settings.audio_block_samples);
      setSampleRate_Hz(settings.sample_rate_Hz);
      resetState();
    }
    virtual void begin(void) {
      Serial.println("AudioControlSignalTester_F32: begin(): ...");
      recomputeTargetCountsPerStep(); //not needed, just to print some debugging messages
      
      //activate the instrumentation
      sig_gen.begin();
      sig_meas.begin(this);

      //start the test
      resetState();
      gotoNextStep();
    }

    //use this to cancel the test
    virtual void end(void) {
      finishTest();
    }
    
    void setAudioSettings(AudioSettings_F32 audio_settings) {
      setAudioBlockSamples(audio_settings.audio_block_samples);
      setSampleRate_Hz(audio_settings.sample_rate_Hz);
    }
    void setTargetDurPerStep_sec(float sec) {
      if (sec > 0.001) {
        target_dur_per_step_sec = sec;
        recomputeTargetCountsPerStep();
      } else {
        Serial.print("AudioControlSignalTester_F32: setTargetDurPerStep_sec: given duration too short: ");
        Serial.print(target_dur_per_step_sec);
        Serial.print(". Ignoring...");
        return;
      }
    }
    void setAudioBlockSamples(int block_samples) {
      audio_block_samples = block_samples;
      recomputeTargetCountsPerStep();
    }
    void setSampleRate_Hz(float fs_Hz) {
      sample_rate_Hz = fs_Hz;
      recomputeTargetCountsPerStep();
    }
    virtual void setStepPattern(float _start_val, float _end_val, float _step_val) {
      start_val = _start_val; end_val = _end_val; step_val = _step_val;
      recomputeTargetNumberOfSteps();
    }
    
    virtual void transferRMSValues(float baseline_rms, float test_rms) {
      if (counter_ignore > 0) {
        //ignore this reading
        counter_ignore--;
        return;
      }

      //add this number
      sum_sig_pow_baseline[counter_step] += (baseline_rms*baseline_rms);
      sum_sig_pow_test[counter_step] += (test_rms*test_rms);
	  freq_at_each_step_Hz[counter_step] = signal_frequency_Hz;
      counter_sum[counter_step]++;
      if (counter_sum[counter_step] >= target_counts_per_step) {
        gotoNextStep();
      }
    }
	
	virtual void setSignalFrequency_Hz(float freq_Hz) {
      signal_frequency_Hz = freq_Hz;
      sig_gen.setSignalFrequency_Hz(signal_frequency_Hz);
    }
    virtual void setSignalAmplitude_dBFS(float amp_dBFS) {
      signal_amplitude_dBFS = amp_dBFS;
      sig_gen.setSignalAmplitude_dBFS(amp_dBFS);
    }
	virtual void printTableOfResults(Stream *s) {
      float ave1_dBFS, ave2_dBFS, gain_dB;
      s->println("  : Freq (Hz), Input (dBFS), Output (dBFS), Gain (dB)");
      for (int i=0; i < target_n_steps; i++) {
        ave1_dBFS = 10.f*log10f(sum_sig_pow_baseline[i]/counter_sum[i]);
        ave2_dBFS = 10.f*log10f(sum_sig_pow_test[i]/counter_sum[i]);
        gain_dB = ave2_dBFS - ave1_dBFS;
		s->print("     ");  s->print(freq_at_each_step_Hz[i],0);
        s->print(",       ");  s->print(ave1_dBFS,1);
        s->print(",       ");  s->print(ave2_dBFS,1);
        s->print(",       ");  s->println(gain_dB,2);
      }
	}
	
    bool isDataAvailable = false;
    bool available(void) { return isDataAvailable; }
    
  protected:
    AudioTestSignalGenerator_F32 &sig_gen;
    AudioTestSignalMeasurement_F32 &sig_meas;
	float signal_frequency_Hz = 1000.f;
    float signal_amplitude_dBFS = -50.0f;
    int counter_ignore = 0;
    //bool is_testing = 0;
    
    int audio_block_samples = AUDIO_BLOCK_SAMPLES;
    float sample_rate_Hz = AUDIO_SAMPLE_RATE_EXACT;
    float target_dur_per_step_sec = 0.2;
    int target_counts_per_step = 1;
    
    //const int max_steps = 64;
    float start_val = 0, end_val = 1.f, step_val = 1.f;
    int target_n_steps = 1;

    float sum_sig_pow_baseline[max_steps];
    float sum_sig_pow_test[max_steps];
	float freq_at_each_step_Hz[max_steps];
    int counter_sum[max_steps], counter_step=-1;

    int recomputeTargetCountsPerStep(void) {
      target_counts_per_step = max(1,(int)((target_dur_per_step_sec * sample_rate_Hz / ((float)audio_block_samples))+0.5)); //round
//      if (Serial) {
//        Serial.println("AudioControlSignalTester_F32: recomputeTargetCountsPerStep: ");
//        Serial.print("   : target_dur_per_step_sec = "); Serial.println(target_dur_per_step_sec);
//        Serial.print("   : sample_rate_Hz = "); Serial.println(sample_rate_Hz);
//        Serial.print("   : audio_block_samples = "); Serial.println(audio_block_samples);
//        Serial.print("   : target_counts_per_step = "); Serial.println(target_counts_per_step);
//      }
      return target_counts_per_step; 
    }
    virtual int recomputeTargetNumberOfSteps(void) {
      return target_n_steps = (int)((end_val - start_val)/step_val + 0.5)+1;  //round
    }

    virtual void resetState(void) {
      isDataAvailable = false;
      for (int i=0; i<max_steps; i++) {
        sum_sig_pow_baseline[i]=0.0f;
        sum_sig_pow_test[i]=0.0f;
        counter_sum[i] = 0;
      }
      counter_step = -1;
    }
    virtual void gotoNextStep(void) {
      counter_step++;
      //Serial.print("AudioControlSignalTester_F32: gotoNextStep: starting step ");
      //Serial.println(counter_step);
      if (counter_step >= target_n_steps) {
        finishTest();
        return;
      } else {
        counter_ignore = 10; //ignore first 10 packets
        counter_sum[counter_step]=0;
        sum_sig_pow_baseline[counter_step]=0.0f;
        sum_sig_pow_test[counter_step]=0.0f;
        updateSignalGenerator();
		freq_at_each_step_Hz[counter_step]=0.0f;
		
      }
    }
    virtual void updateSignalGenerator(void) 
    {
      //if (Serial) Serial.println("AudioControlSignalTester_F32: updateSignalGenerator(): did the child version get called?");
    }  //override this is a child class!
    
    virtual void finishTest(void) {
      //Serial.println("AudioControlSignalTester_F32: finishTest()...");
      //disable the test instrumentation
      sig_gen.end();
      sig_meas.end();

      //let listeners know that data is available
      isDataAvailable = true;
    }
};

// //////////////////////////////////////////////////////////////////////////
class AudioControlTestAmpSweep_F32 : public AudioControlSignalTester_F32
{
  //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
  //GUI: shortName: ampSweepTester
  public:
    AudioControlTestAmpSweep_F32(AudioSettings_F32 &settings, AudioTestSignalGenerator_F32 &_sig_gen, AudioTestSignalMeasurement_F32 &_sig_meas) 
      : AudioControlSignalTester_F32(settings, _sig_gen,_sig_meas)
    {
      float start_amp_dB = -100.0f, end_amp_dB = 0.0f, step_amp_dB = 2.5f;
      setStepPattern(start_amp_dB, end_amp_dB, step_amp_dB);
      setTargetDurPerStep_sec(0.25);
      resetState();
    }
    void begin(void) {
      //activate the instrumentation
      sig_gen.begin();
      sig_meas.begin(this);

      //start the test
      resetState();
      gotoNextStep();
    }

    //use this to cancel the test
    //void end(void) {
    //  finishTest();
    //}
    
    void printTableOfResults(Stream *s) {
      s->println("AudioControlTestAmpSweep_F32: Start Table of Results...");
	  AudioControlSignalTester_F32::printTableOfResults(s);
      s->println("AudioControlTestAmpSweep_F32: End Table of Results...");
    }

  protected:
  
    virtual void updateSignalGenerator(void) {
      float new_amp_dB = start_val + ((float)counter_step)*step_val; //start_val and step_val are in parent class
      Serial.print("AudioControlTestAmpSweep_F32: updateSignalGenerator(): setting amplitude to (dBFS) ");
      Serial.println(new_amp_dB);
      setSignalAmplitude_dBFS(new_amp_dB);
    }
    void finishTest(void) {     
      //disable the test instrumentation
      setSignalAmplitude_dBFS(-1000.0f); //some very quiet value
 
      //do all of the common actions
      AudioControlSignalTester_F32::finishTest();

      //print results
      printTableOfResults(&Serial);
    }

    //void resetState(void) {
    //  AudioControlSignalTester_F32::resetState();
    //}
};


// //////////////////////////////////////////////////////////////////////////
class AudioControlTestFreqSweep_F32 : public AudioControlSignalTester_F32
{
  //GUI: inputs:0, outputs:0  //this line used for automatic generation of GUI node
  //GUI: shortName: freqSweepTester
  public:
    AudioControlTestFreqSweep_F32(AudioSettings_F32 &settings, AudioTestSignalGenerator_F32 &_sig_gen, AudioTestSignalMeasurement_F32 &_sig_meas) 
      : AudioControlSignalTester_F32(settings, _sig_gen,_sig_meas)
    {
      float start_freq_Hz = 125.f, end_freq_Hz = 16000.f, step_freq_octave = sqrtf(2.0);
      setStepPattern(start_freq_Hz, end_freq_Hz, step_freq_octave);
      setTargetDurPerStep_sec(0.25);
      resetState();
    }
    void begin(void) {
      //activate the instrumentation
      sig_gen.begin();
      sig_meas.begin(this);

      //start the test
      resetState();
	  recomputeTargetNumberOfSteps();
      gotoNextStep();
    }

    //use this to cancel the test
    //void end(void) {
    //  finishTest();
    //}
    
    void printTableOfResults(Stream *s) {
      s->println("AudioControlTestFreqSweep_F32: Start Table of Results...");
	  AudioControlSignalTester_F32::printTableOfResults(s);
      s->println("AudioControlTestFreqSweep_F32: End Table of Results...");
    }

	
  protected:
	float signal_frequency_Hz = 1000.f;
    float signal_amplitude_dBFS = -50.0f;
	
	virtual int recomputeTargetNumberOfSteps(void) {
      return target_n_steps = (int)(log10f(end_val/start_val)/log10f(step_val)+0.5) + 1;  //round
    }  
	
    virtual void updateSignalGenerator(void) {
	  //logarithmically step the frequency
	  float new_freq_Hz = start_val * powf(step_val,counter_step);
	  Serial.print("AudioControlTestFreqSweep_F32: updateSignalGenerator(): setting freq to ");
      Serial.println(new_freq_Hz);
      setSignalFrequency_Hz(new_freq_Hz);
    }
    void finishTest(void) {
      
      //disable the test instrumentation
      setSignalAmplitude_dBFS(-1000.0f);  //some very quiet value
	  setSignalFrequency_Hz(1000.f);

      //do all of the common actions
      AudioControlSignalTester_F32::finishTest();

      //print results
      printTableOfResults(&Serial);
    }

    //void resetState(void) {
    //  AudioControlSignalTester_F32::resetState();
    //}
};



#endif
