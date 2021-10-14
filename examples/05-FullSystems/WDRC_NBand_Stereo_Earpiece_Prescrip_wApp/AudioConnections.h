
#include "State.h"
extern State myState;

AudioInputI2S_F32             i2s_in(audio_settings);          //Digital audio input from the ADC over the i2s bus
EarpieceMixer_F32_UI          earpieceMixer(audio_settings);   //mixes earpiece mics, allows switching to analog inputs, mixes left+right, etc
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //move this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
#if USE_FIR_FILTERBANK
  AudioEffectMultiBandWDRC_F32_UI     multiBandWDRC[2];       //use FIR filterbank
  #define FILTER_ORDER 128                                    //usually is 96, but we have spare CPU to allow for 128
#else
  AudioEffectMultiBandWDRC_IIR_F32_UI multiBandWDRC[2];       //use biquad (IIR) filterbank
  #define FILTER_ORDER 6                                      //usually is 6.  never tried anything else (like 8)
#endif
StereoContainerWDRC_UI              stereoContainerWDRC;           //helps with managing the phone App's GUI for left+right.  In AudioEffectMultiBandWDRC_F32.h
AudioOutputI2S_F32                  i2s_out(audio_settings);       //Digital audio output to the DAC via the i2s bus.  Should be last, except for SD writing
AudioSDWriter_F32_UI                audioSDWriter(audio_settings); //this is 2-channels of audio by default, but can be changed to 4 in setup()

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32      audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32 audioTestMeasurement_Filters(audio_settings);
AudioControlTestAmpSweep_F32        ampSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
AudioControlTestFreqSweep_F32       freqSweepTester(audio_settings,audioTestGenerator,audioTestMeasurement);
//AudioControlTestFreqSweep_F32     freqSweepTester_filterbank(audio_settings,audioTestGenerator,audioTestMeasurement_Filters);

//make the audio connections
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll ever make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  
  const int LEFT = StereoContainer_UI::LEFT, RIGHT = StereoContainer_UI::RIGHT; 
  int count=0;

  // ///////////// Do some setup of the algorithm modules.  Maybe would be less confusing if I put them in ConfigureAlgorithms.h??
  //put items inot each side of the stereo container (for better handling the UI elements)
  stereoContainerWDRC.addPairMultiBandWDRC(&(multiBandWDRC[LEFT]),&(multiBandWDRC[RIGHT])); 

  // connect each earpiece mic to the earpiece mixer
  patchCord[count++] = new AudioConnection_F32(i2s_in,0,earpieceMixer,0);
  patchCord[count++] = new AudioConnection_F32(i2s_in,1,earpieceMixer,1);
  patchCord[count++] = new AudioConnection_F32(i2s_in,2,earpieceMixer,2);
  patchCord[count++] = new AudioConnection_F32(i2s_in,3,earpieceMixer,3);
  
  //connect left input to...well, normally you'd start connecting to the algorithms, but we're going to enable
  //some audio self-testing, so we're going to first connect to the audioTestGenerator.  To say it again, this
  //routing through the audioTestGenerator is only being done to allow self testing.  If you don't want to do
  //this, you would route the audio directly to the filterbank.
  patchCord[count++] = new AudioConnection_F32(earpieceMixer, earpieceMixer.LEFT, audioTestGenerator, 0); 

  //make more connection for the audio test measurements (not needed for audio, only needed for self-testing)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement,     0);  
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_Filters, 0); //this is the baseline connection for comparison
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement,     0); //this is the baseline connection for comparison

  //connect the inputs to the compressors from slightly different places
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0,                   multiBandWDRC[LEFT],  0); //connect to input of multiBandWDRC used for the left
  patchCord[count++] = new AudioConnection_F32(earpieceMixer,      earpieceMixer.RIGHT, multiBandWDRC[RIGHT], 0); //connect to the input of multiBandWDRC used for the right

  for (int Ichan = LEFT; Ichan <= RIGHT; Ichan++) {

    //Set the algorithms sample rate and block size to align with the global values
    multiBandWDRC[Ichan].setSampleRate_Hz(audio_settings.sample_rate_Hz);
    multiBandWDRC[Ichan].setAudioBlockSize(audio_settings.audio_block_samples);
    
    //make filterbank and compressorbank big enough...I'm not sure if this is actually needed
    multiBandWDRC[Ichan].filterbank.set_max_n_filters(N_CHAN);  //is this needed?
    multiBandWDRC[Ichan].compbank.set_max_n_chan(N_CHAN);       //is this needed?

  }

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[LEFT],  0, i2s_out, EarpieceShield::OUTPUT_LEFT_TYMPAN);    //First AIC, Main tympan board headphone jack, left channel
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[RIGHT], 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_TYMPAN);   //First AIC, Main tympan board headphone jack, right channel
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[LEFT],  0, i2s_out, EarpieceShield::OUTPUT_LEFT_EARPIECE);  //Second AIC (Earpiece!), left output i2s_out, Ichan);  //output
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[RIGHT], 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Second AIC (Earpiece!), right output
          
  //Connect the inputs to the SD and the processed outputs to the SD card
  patchCord[count++] = new AudioConnection_F32(earpieceMixer, earpieceMixer.LEFT, audioSDWriter, 0);  //connect raw input audio to SD writer (chan 1-2)
  patchCord[count++] = new AudioConnection_F32(earpieceMixer, earpieceMixer.RIGHT, audioSDWriter, 1);  //connect raw input audio to SD writer (chan 1-2)
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[LEFT],  0, audioSDWriter, 2); //connect processed audio to SD writer (chan 3-4)
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[RIGHT], 0, audioSDWriter, 3); //connect processed audio to SD writer (chan 3-4)

  //make the last connections for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(multiBandWDRC[LEFT], 0, audioTestMeasurement, 1);
    
  return count;
}

void setupAudioProcessing(void) {
  
  //make all of the audio connections
  makeAudioConnections();  //see AudioConnections.h

  //make some software connections to allow different parts of the code to talk with each other
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
