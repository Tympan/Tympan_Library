#ifndef _AudioProcessing_h
#define _AudioProcessing_h

/* ///////////////////////////////////////////////////////////
*
*    Here are the audio connections:
*
*      AudioInputI2S (Left) 
*          | ------------> calcInputLevel_L      [end]
*          | ------------> audioSDWriter (Left)  [end]
*
*      AudioInputI2S (Right)
*          | ------------> calcInputLevel_R      [end]
*          | ----==------> audioSDWriter (Right) [end]
*
*      sineWave (Mono source)
*          | ------------> calcOutputLevel       [end]
*          | ------------> AudioOutputI2S(Left)  [end]
*          | ------------> AudioOutputI2S(Right) [end]
*
////////////////////////////////////////////////////////// */

AudioInputI2S_F32          i2s_in(audio_settings);            //Digital audio input from the ADC
AudioSynthWaveform_F32     sineWave(audio_settings);          //generate a synthetic sine wave
AudioCalcLevel_F32         calcInputLevel_L(audio_settings);  //use this to measure the input signal level
AudioCalcLevel_F32         calcInputLevel_R(audio_settings);  //use this to measure the input signal level
AudioSDWriter_F32          audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2S_F32         i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect the left input to its destinations
AudioConnection_F32        patchcord11(i2s_in, 0, calcInputLevel_L, 0);    //Left input to the level monitor
AudioConnection_F32        patchcord12(i2s_in, 0, audioSDWriter,    0);    //Left input to the SD writer

//Connect the right input to its destinations
AudioConnection_F32        patchcord21(i2s_in, 1, calcInputLevel_R, 0);    //Right input to the level monitor
AudioConnection_F32        patchcord22(i2s_in, 1, audioSDWriter,    1);    //Right input to the SD writer

//Connect the sineWave to its destinations
AudioConnection_F32        patchcord31(sineWave, 0, i2s_out,         0);   //Sine wave to left output
AudioConnection_F32        patchcord32(sineWave, 0, i2s_out,         1);   //Sine wave to righ toutput


// /////////// Functions for configuring the system

float setInputGain_dB(float val_dB) {
  return myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
}

void setConfiguration(int config) {
  myState.input_source = config;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      setInputGain_dB(myState.input_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      setInputGain_dB(myState.input_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      setInputGain_dB(myState.input_gain_dB);
      break;
  }
}

float setCalcLevelTimeConstants(float time_const_sec) {
  time_const_sec = max(time_const_sec,0.0001);
  calcInputLevel_L.setTimeConst_sec(time_const_sec);
  myState.calcLevel_timeConst_sec = calcInputLevel_L.getTimeConst_sec();
  calcInputLevel_R.setTimeConst_sec(myState.calcLevel_timeConst_sec);
  return myState.calcLevel_timeConst_sec; 
}

float setFrequency_Hz(float freq_Hz) {
  myState.sine_freq_Hz = constrain(freq_Hz, 125.0f/4.0, 16000.0f); //constrain between 32 Hz and 16 kHz
  sineWave.frequency(myState.sine_freq_Hz);
  return myState.sine_freq_Hz;
}

float setAmplitude(float amplitude) {
  myState.sine_amplitude = constrain(amplitude, 0.0, 1.0); //constrain between 0.0 and 1.0
  sineWave.amplitude(myState.sine_amplitude);
  return myState.sine_amplitude;
}

#endif