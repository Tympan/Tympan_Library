
#include "State.h"
extern State myState;

AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //move this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
#if USE_FIR_FILTERBANK
  #warning "AudioConnections: Using AudioEffectMultiBandWDRC_F32_UI"
  AudioEffectMultiBandWDRC_F32_UI     multiBandWDRC[2];           //how do we set the block size and sample rate? Is it done during filter design?
  #define FILTER_ORDER 128
#else
  #warning "AudioConnections: Using AudioEffectMultiBandWDRC_IIR_F32_UI"
  AudioEffectMultiBandWDRC_IIR_F32_UI multiBandWDRC[2];           //how do we set the block size and sample rate? Is it done during filter design?
  #define FILTER_ORDER 6
#endif
StereoContainerWDRC_UI              stereoContainerWDRC;          //helps with managing the phone App's GUI for left+right.  In AudioEffectMultiBandWDRC_F32.h
AudioOutputI2S_F32                  i2s_out(audio_settings);      //Digital audio output to the DAC.  Should be last.
AudioSDWriter_F32_UI                audioSDWriter(audio_settings);//this is stereo by default

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32      audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32 audioTestMeasurement_IIR(audio_settings);
AudioControlTestAmpSweep_F32        ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32       freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
//AudioControlTestFreqSweep_F32     freqSweepTester_filterbank(audio_settings,audioTestGenerator,audioTestMeasurement_IIR);

//make the audio connections
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll ever make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  
  const int LEFT = StereoContainer_UI::LEFT, RIGHT = StereoContainer_UI::RIGHT; 
  int count=0;

  // ///////////// Do some setup of the algorithm modules.  Maybe would be less confusing if I put them in ConfigureAlgorithms.h??
  //put items inot each side of the stereo container (for better handling the UI elements)
  stereoContainerWDRC.addPairMultiBandWDRC(&(multiBandWDRC[LEFT]),&(multiBandWDRC[RIGHT])); 

  //connect input...normally you'd connect to the algorithms, but we're going to enable some audio self-testing
  //so we're going to first connect to the audioTestGenerator.  To say it again, this routing through the 
  //audioTestGenerator is only being done to allow self testing.  If you don't want to do this, you would route
  //the i2s_in directly to the filterbank.
  patchCord[count++] = new AudioConnection_F32(i2s_in, LEFT, audioTestGenerator, 0); 

  //make more connection for the audio test measurements (not needed for audio, only needed for self-testing)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement,     0);  
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_IIR, 0); //this is the baseline connection for comparison
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement,     0); //this is the baseline connection for comparison

  //connect the inputs to the compressors from slightly different places
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0,     multiBandWDRC[LEFT],  0); //connect to input of multiBandWDRC
  patchCord[count++] = new AudioConnection_F32(i2s_in,             RIGHT, multiBandWDRC[RIGHT], 0);

  for (int Ichan = LEFT; Ichan <= RIGHT; Ichan++) {

    //Set the algorithms sample rate and block size to align with the global values
    multiBandWDRC[Ichan].setSampleRate_Hz(audio_settings.sample_rate_Hz);
    multiBandWDRC[Ichan].setAudioBlockSize(audio_settings.audio_block_samples);
    
    //make filterbank and compressorbank big enough...I'm not sure if this is actually needed
    multiBandWDRC[Ichan].filterbank.set_max_n_filters(N_CHAN);  //is this needed?
    multiBandWDRC[Ichan].compbank.set_max_n_chan(N_CHAN);       //is this needed?

    //send the audio out
    patchCord[count++] = new AudioConnection_F32(multiBandWDRC[Ichan], 0, i2s_out, Ichan);  //output
        
    //Connect the raw inputs and processed outputs to the SD card
    patchCord[count++] = new AudioConnection_F32(i2s_in, Ichan, audioSDWriter, Ichan);            //connect raw input audio to SD writer (chan 1-2)
    patchCord[count++] = new AudioConnection_F32(multiBandWDRC[Ichan],0, audioSDWriter, 2+Ichan); //connect processed audio to SD writer (chan 3-4)
  }

  //make the last connections for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[LEFT], 0, audioTestMeasurement, 1);
    
  return count;
}

void setupAudioProcessing(void) {
  
  //make all of the audio connections
  makeAudioConnections();  //see AudioConnections.h
  
  //setup the preset manager
  presetManager.attachAlgorithms(&multiBandWDRC[0],&multiBandWDRC[1]);
  
  //try to load the prescription from the SD card
  for (int i=0; i<presetManager.n_presets; i++) {
    int ret_val = -1; //assume error
    
    ret_val = presetManager.setFilterOrder(i,FILTER_ORDER);  //do this before loading any presets

    //try to read the preset from the SD card
    ret_val = presetManager.readPresetFromSD(i);  //ignore any built-in preset and read the settings from the SD card instead
    if (ret_val != 0) {
      //it didn't read the preset correctly from the SD card.  So, overwrite what's on the SD card with the factory preset
      Serial.println("AudioConnections: setupAudioProcessing: failed to read preset " + String(i) + " from SD.  Using factory preset and saving to SD...");
      presetManager.resetPresetToFactory(i); //restore to default from Preset_00.h or Preset_01.h
      presetManager.presets[i].wdrc_perBand[0].printAllValues();
      presetManager.savePresetToSD(i,false);      //save to SD card.  false says to NOT rebuild from the source algorithms...just save the factory reset preset
    }
  }

  //configure the algorithms for the starting preset
  int starting_preset = 0;  //zero is the first
  Serial.println("AudioConnections: setupAudioProcessing: setting to preset " + String(starting_preset)); 
  presetManager.setToPreset(starting_preset);
}


float incrementChannelGain(int chan, float increment_dB) {
  if (chan < N_CHAN) {
    for (int Ichan = StereoContainer_UI::LEFT; Ichan <= StereoContainer_UI::RIGHT; Ichan++) {
      (multiBandWDRC[Ichan].compbank.compressors[chan]).incrementGain_dB(increment_dB);
    }
  }
  printGainSettings();  //in main sketch file
  return multiBandWDRC[StereoContainer_UI::LEFT].compbank.getLinearGain_dB(chan);
}
