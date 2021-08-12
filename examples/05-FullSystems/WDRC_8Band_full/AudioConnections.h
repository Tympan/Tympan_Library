
#include "State.h"
extern State myState;


// Do you want to use IIR (Biquad) filters for the filterbank or FIR filters for the filterbank?
#define USE_IIR_FILTERS true  //if true, uses IIR filters. If false uses FIR filters

#if (USE_IIR_FILTERS==true)
    const int N_FILT_ORDER = 6;
    String FILT_TYPE = String("IIR");  
#else
    const int N_FILT_ORDER = 96;
    String FILT_TYPE = String("FIR");
#endif


// ///////////////////////////////////  Defining the audio classes
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //move this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
#if (USE_IIR_FILTERS)
  AudioFilterbankBiquad_F32_UI   filterbank(audio_settings);   //a filterbank holding the filters to break up the audio into multiple bands  
#else  
  AudioFilterbankFIR_F32_UI      filterbank(audio_settings);   //a filterbank holding the filters to break up the audio into multiple bands
#endif
AudioEffectCompBankWDRC_F32_UI compbank(audio_settings);     //a cmopressor bank holding the WDRC compressors
AudioMixer16_F32               mixer1(audio_settings);       //mixer to reconstruct the broadband audio after the per-band processing
AudioEffectGain_F32            broadbandGain(audio_settings);//broad band gain (could be part of compressor below)
AudioEffectCompWDRC_F32_UI     compBroadband(audio_settings);//broadband compressor
AudioOutputI2S_F32             i2s_out(audio_settings);      //Digital audio output to the DAC.  Should be last.
AudioSDWriter_F32_UI           audioSDWriter(audio_settings);//this is stereo by default

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32   audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_FIR(audio_settings);
AudioControlTestAmpSweep_F32     ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_filterbank(audio_settings,audioTestGenerator,audioTestMeasurement_FIR);

// ///////////////////////////////////  Make the connections between the audio classes
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll ever make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];

int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count=0;

  //make filterbank and compressorbank big enough...I'm not sure if this is actually needed
  filterbank.set_max_n_filters(N_CHAN);  //only needed if we're going to connect directly to the filter bank's underlying filters
  compbank.set_max_n_chan(N_CHAN); //only needed if we're going to connect directly to the compressor bank's underlying compressors

  //connect input...normally you'd connect to the algorithms, but we're going to enable some audio self-testing
  //so we're going to first connect to the audioTestGenerator.  To say it again, this routing through the 
  //audioTestGenerator is only being done to allow self testing.  If you don't want to do this, you would route
  //the i2s_in directly to the filterbank.
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioTestGenerator, 0); 

  //make more connection for the audio test measurements (not needed for audio, only needed for self-testing)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);  
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_FIR, 0);

  //connect the audio to the filterbank (the raw audio from the microphones is coming through the audioTestGenerator)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, filterbank, 0); //connect to input of FIR filterbank

  //make the per-channel connections
  for (int i = 0; i < N_CHAN; i++) {
    
    //connect filterbank to compressor bank
    patchCord[count++] = new AudioConnection_F32(filterbank, i, compbank, i); 
    
    //connect compressor bank to mixer
    patchCord[count++] = new AudioConnection_F32(compbank,   i, mixer1,   i);

    //connect the filterbank back to the audio test measurement stuff (not needed for audio, only needed for self-testing)
    patchCord[count++] = new AudioConnection_F32(filterbank, i, audioTestMeasurement_FIR, 1+i);
  }

  //connect the output of the mixer to an overall gain control (for loudness
  patchCord[count++] = new AudioConnection_F32(mixer1, 0, broadbandGain, 0);

  //connect to the final broadband compressor that is being used as a limitter
  patchCord[count++] = new AudioConnection_F32(broadbandGain, 0, compBroadband, 0);  //connect to final limiter

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 0);  //left output
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 1);  //right output

  //make the last connections for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, audioTestMeasurement, 1);

  //Connect the raw inputs to the SD card for recording the raw microphone audio
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioSDWriter, 0);       //connect raw input audio to SD writer (left channel)
  patchCord[count++] = new AudioConnection_F32(compBroadband,0, audioSDWriter, 1); //connect processed audio to SD writer (right channel)

  return count;
}
