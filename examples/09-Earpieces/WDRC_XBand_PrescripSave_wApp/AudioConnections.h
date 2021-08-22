//create audio objects for the algorithm
AudioInputI2SQuad_F32         i2s_in(audio_settings);   //Digital audio input from the ADC
AudioEffectDelay_F32          rearMicDelay(audio_settings), rearMicDelayR(audio_settings);
AudioMixer4_F32               frontRearMixer[2];         //mixes front-back earpiece mics
AudioSummer4_F32              analogVsDigitalSwitch[2];  //switches between analog and PDM (a summer is cpu cheaper than a mixer, and we don't need to mix here)
AudioMixer4_F32               leftRightMixer[2];        //mixers to control mono versus stereo
AudioFilterBiquad_F32         preFilter(audio_settings), preFilterR(audio_settings);  //remove low frequencies near DC
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //keep this to be *after* the creation of the i2s_in object

AudioEffectAFC_BTNRH_F32   feedbackCancel(audio_settings), feedbackCancelR(audio_settings);  //original adaptive feedback cancelation from BTNRH
AudioFilterBiquad_F32      bpFilt[2][N_CHAN_MAX];         //here are the filters to break up the audio into multiple bands
AudioConfigIIRFilterBank_F32 filterBankCalculator(audio_settings);  //this computes the filter coefficients
//AudioFilterIIR_F32         bpFilt[2][N_CHAN_MAX];           //here are the filters to break up the audio into multiple bands
AudioEffectDelay_F32       postFiltDelay[2][N_CHAN_MAX];  //Here are the delay modules that we'll use to time-align the output of the filters
AudioEffectCompWDRC_F32    expCompLim[2][N_CHAN_MAX];     //here are the per-band compressors
AudioMixer8_F32            mixerFilterBank[2];                     //mixer to reconstruct the broadband audio
AudioEffectCompWDRC_F32    compBroadband[2];              //broad band compressor
AudioEffectFeedbackCancel_LoopBack_Local_F32 feedbackLoopBack(audio_settings), feedbackLoopBackR(audio_settings);
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2SQuad_F32      i2s_out(audio_settings);    //Digital audio output to the DAC.  Should be last.

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32  audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_filterbank(audio_settings);
AudioControlTestAmpSweep_F32    ampSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_perBand(audio_settings, audioTestGenerator, audioTestMeasurement_filterbank);

//make the audio connections
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count = 0;

  //connect input to earpiece mixer
  patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_LEFT_FRONT, frontRearMixer[LEFT], FRONT); 
  patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_LEFT_REAR, rearMicDelay, 0); 
  patchCord[count++] = new AudioConnection_F32(rearMicDelay, 0, frontRearMixer[LEFT], REAR); 
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_RIGHT_FRONT, frontRearMixer[RIGHT], FRONT); 
    patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_RIGHT_REAR, rearMicDelayR, 0); 
    patchCord[count++] = new AudioConnection_F32(rearMicDelayR, 0, frontRearMixer[RIGHT], REAR); 
  //}

  //analog versus earpiece switching
  patchCord[count++] = new AudioConnection_F32(i2s_in, LEFT, analogVsDigitalSwitch[LEFT], ANALOG_IN); 
  patchCord[count++] = new AudioConnection_F32(frontRearMixer[LEFT], 0, analogVsDigitalSwitch[LEFT], PDM_IN); 
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(i2s_in, RIGHT, analogVsDigitalSwitch[RIGHT], ANALOG_IN); 
    patchCord[count++] = new AudioConnection_F32(frontRearMixer[RIGHT], 0, analogVsDigitalSwitch[RIGHT], PDM_IN); 
  //}
 
  //connect analog and digital inputs for left side and then for the right side
  patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[LEFT], 0, leftRightMixer[LEFT], LEFT);      //all possible connections intended for LEFT output
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[LEFT], RIGHT);    //all possible connections intended for LEFT output
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[LEFT], 0, leftRightMixer[RIGHT], LEFT);     //all possible connections intended for RIGHT output
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[RIGHT], RIGHT);   //all possible connections intended for RIGHT output
  }

  //add prefilters
  patchCord[count++] = new AudioConnection_F32(leftRightMixer[LEFT],0,preFilter,0);
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(leftRightMixer[RIGHT],0,preFilterR,0);
  }
  
  
  //configure the mixer's default state until set later
  //configureFrontRearMixer(State::MIC_FRONT);
  //configureLeftRightMixer(State::INPUTMIX_MUTE); //set left mixer to only listen to left, set right mixer to only listen to right
  frontRearMixer[LEFT].gain(FRONT,1.0);frontRearMixer[LEFT].gain(REAR,0.0);
  frontRearMixer[RIGHT].gain(FRONT,1.0);frontRearMixer[RIGHT].gain(REAR,0.0);
  leftRightMixer[LEFT].gain(LEFT,0.0);leftRightMixer[LEFT].gain(RIGHT,0.0);     //mute the left side
  leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,0.0);   //mute the right side
  


  //make the connection for the audio test measurements...only relevant when the audio test functions are being invoked by the engineer/user
  patchCord[count++] = new AudioConnection_F32(preFilter, 0, audioTestGenerator, 0); // set output of left mixer to the audioTestGenerator (which is will then pass through)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0); // connect test generator to test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_filterbank, 0); //connect test generator to the test measurement filterbank

  //start the algorithms with the feedback cancallation block
  #if 1  //set to zero to discable the adaptive feedback cancelation
    patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, feedbackCancel, 0); //remember, even the normal audio is coming through the audioTestGenerator
  #endif

  //make per-channel connections: filterbank -> delay -> WDRC Compressor -> mixer (synthesis)
  for (int Iear = 0; Iear < N_EARPIECES; Iear++) { //loop over channels
    for (int Iband = 0; Iband < N_CHAN_MAX; Iband++) {
      if (Iear == LEFT) {
        #if 1  //set to zero to discable the adaptive feedback cancelation
          patchCord[count++] = new AudioConnection_F32(feedbackCancel, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler //perhaps 
        #else
          patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, bpFilt[Iear][Iband], 0); //input is coming from the audio test generator
        #endif
      } else {
        #if 1  //set to zero to discable the adaptive feedback cancelation
          patchCord[count++] = new AudioConnection_F32(feedbackCancelR, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler
        #else
          patchCord[count++] = new AudioConnection_F32(preFilterR, 0, bpFilt[Iear][Iband], 0); //input is coming directly from i2s_in
        #endif
      }
      patchCord[count++] = new AudioConnection_F32(bpFilt[Iear][Iband], 0, postFiltDelay[Iear][Iband], 0);  //connect to delay
      patchCord[count++] = new AudioConnection_F32(postFiltDelay[Iear][Iband], 0, expCompLim[Iear][Iband], 0); //connect to compressor
      patchCord[count++] = new AudioConnection_F32(expCompLim[Iear][Iband], 0, mixerFilterBank[Iear], Iband); //connect to mixer

      //make the connection for the audio test measurements
      if (Iear == LEFT) {
        patchCord[count++] = new AudioConnection_F32(bpFilt[Iear][Iband], 0, audioTestMeasurement_filterbank, 1 + Iband);
      }
    }

    //connect the output of the mixers to the final broadband compressor
    patchCord[count++] = new AudioConnection_F32(mixerFilterBank[Iear], 0, compBroadband[Iear], 0);  //connect to final limiter
  }

  //connect the loop back to the adaptive feedback canceller
  feedbackLoopBack.setTargetAFC(&feedbackCancel);   //left ear
  if (RUN_STEREO) feedbackLoopBackR.setTargetAFC(&feedbackCancelR); //right ear
  
  #if 1  // set to zero to disable the adaptive feedback canceler
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, feedbackLoopBack, 0); //loopback to the adaptive feedback canceler
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, feedbackLoopBackR, 0); //loopback to the adaptive feedback canceler
  #endif

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_LEFT_EARPIECE); 
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_LEFT_TYMPAN); 
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, i2s_out, OUTPUT_RIGHT_EARPIECE); 
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, i2s_out, OUTPUT_RIGHT_TYMPAN); 
  } else {
    //copy mono audio to other channel to present in both ears
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_RIGHT_EARPIECE); 
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_RIGHT_TYMPAN); 
  }
  
  //make the last connections for the audio test measurements and SD writer
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, audioTestMeasurement, 1);
  patchCord[count++] = new AudioConnection_F32(leftRightMixer[LEFT], 0, audioSDWriter, 0);
  patchCord[count++] = new AudioConnection_F32(i2s_out, OUTPUT_LEFT_EARPIECE, audioSDWriter, 1);
  

  return count;
}
