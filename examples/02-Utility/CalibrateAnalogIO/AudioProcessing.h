#ifndef _AudioProcessing_h
#define _AudioProcessing_h

AudioInputI2S_F32          i2s_in(audio_settings);             //Digital audio input from the ADC
AudioCalcLeq_F32           calcInputLevel_L(audio_settings);   //use this to measure the input signal level
AudioCalcLeq_F32           calcInputLevel_R(audio_settings);   //use this to measure the input signal level
AudioSynthWaveform_F32     sineWave(audio_settings);           //generate a synthetic sine wave
AudioSwitchMatrix4_F32     outputSwitchMatrix(audio_settings); //use this to route the sine wave to L, R, or Both
AudioCalcLeq_F32           calcOutputLevel(audio_settings);    //use this to measure the input signal level
AudioSDWriter_F32          audioSDWriter(audio_settings);      //this is stereo by default
AudioOutputI2S_F32         i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

/* ///////////////////////////////////////////////////////////
*
*    Here are the audio connections:
*
*      AudioInputI2S (Chan 0, which is Left) 
*          | -----> calcInputLevel_L      [end]
*          | -----> audioSDWriter (Left)  [end]
*
*      AudioInputI2S (Chan 1, which is Right)
*          | ------> calcInputLevel_R      [end]
*          | ----==> audioSDWriter (Right) [end]
*
*      sineWave (Mono source)
*          | ------> calcOutputLevel       [end]
*          | ------> AudioSwitchMatrix
*                           | (Chan 0) ------> AudioOutputI2S(Left)  [end]
*                           | (Chan 1) ------> AudioOutputI2S(Right)  [end]
*
////////////////////////////////////////////////////////// */

//Connect the left input to its destinations
AudioConnection_F32        patchcord11(i2s_in, 0, calcInputLevel_L, 0);    //Left input to the level monitor
AudioConnection_F32        patchcord12(i2s_in, 0, audioSDWriter,    0);    //Left input to the SD writer

//Connect the right input to its destinations
AudioConnection_F32        patchcord21(i2s_in, 1, calcInputLevel_R, 0);    //Right input to the level monitor
AudioConnection_F32        patchcord22(i2s_in, 1, audioSDWriter,    1);    //Right input to the SD writer

//Connect the sineWave to its destinations
AudioConnection_F32        patchcord30(sineWave, 0, calcOutputLevel,    0);   //Sine wave to level monitor
AudioConnection_F32        patchcord31(sineWave, 0, outputSwitchMatrix, 0);   //Sine wave to level monitor
AudioConnection_F32        patchcord32(outputSwitchMatrix, State::OUT_LEFT,  i2s_out, 0);   //Sine wave to left output
AudioConnection_F32        patchcord33(outputSwitchMatrix, State::OUT_RIGHT, i2s_out, 1);   //Sine wave to righ toutput


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

void printInputConfiguration(void) {
  switch (myState.input_source) {
    case State::INPUT_PCBMICS:
      Serial.print("PCB Mics"); break;
    case State::INPUT_JACK_MIC:
      Serial.print("Input Jack as Mic"); break;
    case State::INPUT_JACK_LINE:
      Serial.print("Input Jack as Line-In"); break;
  }
}


int setOutputChan(int chan) {
  const int inputChanForSineWave = 0;
  const int inputChanForMutedOutput = 3;
  const int outputChanLeft = State::OUT_LEFT;
  const int outputChanRight = State::OUT_RIGHT;

  switch (chan) {
    case State::OUT_LEFT:
      Serial.println("setOutputChan: chan = " +  String(chan) + ".  Setting to LEFT...");
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanLeft);     //sine to left output
      outputSwitchMatrix.setInputToOutput(inputChanForMutedOutput,outputChanRight); //mute the right output
      myState.output_chan = chan;
      break;
    case State::OUT_RIGHT:
      Serial.println("setOutputChan: chan = " +  String(chan) + ".  Setting to RIGHT...");
      outputSwitchMatrix.setInputToOutput(inputChanForMutedOutput,outputChanLeft);  //mute the left output
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanRight);    //sine to right output
      myState.output_chan = chan;
      break;
    case State::OUT_BOTH:
      Serial.println("setOutputChan: chan = " +  String(chan) + ".  Setting to BOTH...");
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanLeft);     //sine to left output
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanRight);    //sine to right output
      myState.output_chan = chan;
      break;
    default:
      Serial.println("setOutputChan: *** WARNING ***: chan = " +  String(chan) + " not recognized.  Setting to BOTH...");
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanLeft);     //sine to left output
      outputSwitchMatrix.setInputToOutput(inputChanForSineWave,outputChanRight);    //sine to right output  
      myState.output_chan = chan;
      break;
  }
  return myState.output_chan;
}

void printOutputChannel(void) {
  switch (myState.output_chan) {
    case State::OUT_LEFT:
      Serial.print("LEFT"); break;
    case State::OUT_RIGHT:
      Serial.print("RIGHT"); break;
    case State::OUT_BOTH:
      Serial.print("BOTH"); break;
  }
}

// /////////// Functions for configuring the audio processing

float setCalcLevelTimeWindow(float time_window_sec) {
  time_window_sec = max(time_window_sec,0.0001);       //don't windows that are too short
  calcInputLevel_L.setTimeWindow_sec(time_window_sec); //try to set the window time
  myState.calcLevel_timeWindow_sec = calcInputLevel_L.getTimeWindow_sec(); //ask what time was actually set

  //set the rest of the time windows
  calcInputLevel_R.setTimeWindow_sec(myState.calcLevel_timeWindow_sec);
  calcOutputLevel.setTimeWindow_sec(myState.calcLevel_timeWindow_sec);
  return myState.calcLevel_timeWindow_sec; 
}

float setFrequency_Hz(float freq_Hz) {
  const float min_freq_Hz = 125.0/8; 
  const float max_freq_Hz = 20000.0;
  myState.sine_freq_Hz = constrain(freq_Hz, min_freq_Hz, max_freq_Hz); //constrain between 32 Hz and 16 kHz
  sineWave.frequency(myState.sine_freq_Hz);
  return myState.sine_freq_Hz;
}

float setAmplitude(float amplitude) {
  myState.sine_amplitude = constrain(amplitude, 0.0, 1.0); //constrain between 0.0 and 1.0
  sineWave.amplitude(myState.sine_amplitude);
  return myState.sine_amplitude;
}


#endif