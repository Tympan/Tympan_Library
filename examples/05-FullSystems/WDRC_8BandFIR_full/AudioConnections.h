
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //move this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
AudioFilterFIR_F32        firFilt[N_CHAN];               //here are the filters to break up the audio into multiple bands
AudioEffectCompWDRC_F32   expCompLim[N_CHAN];            //here are the per-band compressors
AudioMixer8_F32           mixer1(audio_settings);        //mixer to reconstruct the broadband audio
AudioEffectGain_F32       broadbandGain(audio_settings); //broad band gain (could be part of compressor below)
AudioEffectCompWDRC_F32   compBroadband(audio_settings); //broadband compressor
AudioOutputI2S_F32        i2s_out(audio_settings);       //Digital audio output to the DAC.  Should be last.
AudioSDWriter_F32_UI      audioSDWriter(audio_settings); //this is stereo by default


//complete the creation of the tester objects
AudioTestSignalMeasurement_F32   audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_FIR(audio_settings);
AudioControlTestAmpSweep_F32     ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_filterbank(audio_settings,audioTestGenerator,audioTestMeasurement_FIR);

//make the audio connections
#define N_MAX_CONNECTIONS 100  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count=0;

  //connect input
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioTestGenerator, 0); 

  //make the connection for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_FIR, 0);

  //make per-channel connections
  for (int i = 0; i < N_CHAN; i++) {
    //audio connections
    patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, firFilt[i], 0); //connect to FIR filter
    patchCord[count++] = new AudioConnection_F32(firFilt[i], 0, expCompLim[i], 0); //connect filter to compressor
    patchCord[count++] = new AudioConnection_F32(expCompLim[i], 0, mixer1, i); //connect to mixer

    //make the connection for the audio test measurements
    patchCord[count++] = new AudioConnection_F32(firFilt[i], 0, audioTestMeasurement_FIR, 1+i);
  }

  //connect the output of the mixers to the final broadband compressor
  patchCord[count++] = new AudioConnection_F32(mixer1, 0, broadbandGain, 0);
  patchCord[count++] = new AudioConnection_F32(broadbandGain, 0, compBroadband, 0);  //connect to final limiter

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 0);  //left output
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, i2s_out, 1);  //right output
  
  //make the connections for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband, 0, audioTestMeasurement, 1);

  //Connect the raw inputs to the SD card for recording the raw microphone audio
  patchCord[count++] = new AudioConnection_F32(i2s_in, 0, audioSDWriter, 0);       //connect raw input audio to SD writer (left channel)
  patchCord[count++] = new AudioConnection_F32(compBroadband,0, audioSDWriter, 1); //connect processed audio to SD writer (right channel)

  return count;
}
