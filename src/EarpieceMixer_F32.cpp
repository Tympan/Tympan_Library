
#include "EarpieceMixer_F32.h"

//update is called by the Audio processing ISR.
void EarpieceMixerBase_F32::update(void) {

	//allocate the input and output audio memory...if successfull, we must remember to release all of these audio blocks
	audio_block_f32_t *audio_in[4], *tmp[6], *audio_out[2];	
	if (allocateAndGetAudioBlocks(audio_in, tmp, audio_out) == false) return;  //If it fails, we stop and return.  If success, we continue.
	
	//do the work
	processData(audio_in, tmp, audio_out);

	//transmit the output
	AudioStream_F32::transmit(audio_out[0], 0); AudioStream_F32::transmit(audio_out[1], 1);

	//release all the memory
	for (int i = 0; i < 6; i++) AudioStream_F32::release(tmp[i]);
	for (int i = 0; i < 2; i++) AudioStream_F32::release(audio_out[i]);
	for (int i = 0; i < 4; i++) AudioStream_F32::release(audio_in[i]);

	//we're done!
}

//the audio_in must be writable and the audio-out must also be writable
void EarpieceMixerBase_F32::processData(audio_block_f32_t *audio_in[4], audio_block_f32_t *tmp[6], audio_block_f32_t *audio_out[2]) {
	audio_block_f32_t *tmp_out_left = tmp[4];
	audio_block_f32_t *tmp_out_right = tmp[5];
	audio_block_f32_t *tmp_input[4]; //each mixer assumes that it's getting a 4-element array of audio pointers
	
	//apply delay to each mic channel
	//AudioConnection_F32           patchcord1(i2s_in, PDM_LEFT_FRONT, frontMicDelay, 0);
	//AudioConnection_F32           patchcord2(i2s_in, PDM_LEFT_REAR,  rearMicDelay, 0);
	//AudioConnection_F32           patchcord6(i2s_in, PDM_RIGHT_FRONT, frontMicDelayR, 0);
	//AudioConnection_F32           patchcord7(i2s_in, PDM_RIGHT_REAR,  rearMicDelayR, 0);
	frontMicDelay[LEFT ].processData(audio_in[PDM_LEFT_FRONT] , tmp[PDM_LEFT_FRONT] );
	frontMicDelay[RIGHT].processData(audio_in[PDM_RIGHT_FRONT], tmp[PDM_RIGHT_FRONT]);
	rearMicDelay[ LEFT ].processData(audio_in[PDM_LEFT_REAR]  , tmp[PDM_LEFT_REAR]  );
	rearMicDelay[ RIGHT].processData(audio_in[PDM_RIGHT_REAR] , tmp[PDM_RIGHT_REAR] );

	//do the front-rear mixing
	//AudioConnection_F32           patchcord3(frontMicDelay, 0, frontRearMixer[LEFT], FRONT);
	//AudioConnection_F32           patchcord4(rearMicDelay,  0, frontRearMixer[LEFT], REAR);
	//AudioConnection_F32           patchcord8(frontMicDelayR, 0, frontRearMixer[RIGHT], FRONT);
	//AudioConnection_F32           patchcord9(rearMicDelayR, 0, frontRearMixer[RIGHT], REAR);
	for (int i=0; i<4; i++) tmp_input[i]=NULL; //clear the temporary inputs
	tmp_input[FRONT] = tmp[PDM_LEFT_FRONT]; tmp_input[REAR] = tmp[PDM_LEFT_REAR]; //put the left-earpiece audio into the temp array
	frontRearMixer[LEFT].processData(tmp_input, tmp_out_left); //process to make a mix from the left earpiece
	tmp_input[FRONT] = tmp[PDM_RIGHT_FRONT]; tmp_input[REAR] = tmp[PDM_RIGHT_REAR]; //put the right-earpiece audio into the temp array 
	frontRearMixer[RIGHT].processData(tmp_input, tmp_out_right); //process to make a mix from the right earpiece
	 
	//switch between digital inputs or analog inputs
	//AudioConnection_F32           patchcord11(i2s_in, LEFT, analogVsDigitalSwitch[LEFT], ANALOG_IN);
	//AudioConnection_F32           patchcord12(frontRearMixer[LEFT], 0, analogVsDigitalSwitch[LEFT], PDM_IN);
	//AudioConnection_F32           patchcord13(i2s_in, RIGHT, analogVsDigitalSwitch[RIGHT], ANALOG_IN);
	//AudioConnection_F32           patchcord14(frontRearMixer[RIGHT], 0, analogVsDigitalSwitch[RIGHT], PDM_IN);
	for (int i=0; i<4; i++) tmp_input[i]=NULL; //clear the temporary inputs
	tmp_input[INPUT_ANALOG] = audio_in[LEFT]; tmp_input[INPUT_PDM] = tmp_out_left;
	analogVsDigitalSwitch[LEFT].processData(tmp_input, tmp_out_left); //will overrite the tmp_out_left (which is also part of the tmp_input array)
	tmp_input[INPUT_ANALOG] = audio_in[RIGHT]; tmp_input[INPUT_PDM] = tmp_out_right;
	analogVsDigitalSwitch[RIGHT].processData(tmp_input, tmp_out_right); //will overrite the tmp_out_right (which is also part of the tmp_input array)

	//do the left-right mixing
	//AudioConnection_F32           patchcord15(analogVsDigitalSwitch[LEFT],  0, leftRightMixer[LEFT], LEFT);  //this mixer will go out left ear
	//AudioConnection_F32           patchcord16(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[LEFT], RIGHT); //this mixer will go out left ear
	//AudioConnection_F32           patchcord17(analogVsDigitalSwitch[LEFT],  0, leftRightMixer[RIGHT], LEFT); //this mixer will go out right ear
	//AudioConnection_F32           patchcord18(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[RIGHT], RIGHT); //this mixer will go out right ear
	for (int i=0; i<4; i++) tmp_input[i]=NULL; //clear the temporary inputs
	tmp_input[LEFT] = tmp_out_left; tmp_input[RIGHT] = tmp_out_right; //put the left and right audio into the temp array
	leftRightMixer[LEFT].processData(tmp_input, audio_out[LEFT]); //process to make a mix for the left output
	leftRightMixer[RIGHT].processData(tmp_input, audio_out[RIGHT]); //process to make a mix for the left output
};

bool EarpieceMixerBase_F32::allocateAndGetAudioBlocks(audio_block_f32_t *audio_in[4], audio_block_f32_t *tmp[6], audio_block_f32_t *audio_out[2]) {

	bool any_data_present = false;
	if ((audio_out[0] = allocate_f32()) == NULL) return false;
	if ((audio_out[1] = allocate_f32()) == NULL) {
		AudioStream_F32::release(audio_out[0]);
		return false;
	}

	//get the audio
	for (int Ichan = 0; Ichan < 4; Ichan++) {
		audio_in[Ichan] = receiveReadOnly_f32(Ichan); //get the data block...must be sure to release this later
		//audio_in[Ichan] = receiveWritable_f32(Ichan);   //get the data block...must be sure to release this later

		//did we get any audio for this channel?
		if (audio_in[Ichan]) any_data_present = true;
	}

	//did we get any input data? if not, return
	if (any_data_present == false) {
		//no input data.  release the blocks and return
		for (int Ichan = 0; Ichan < 4; Ichan++) {if (audio_in[Ichan]) AudioStream_F32::release(audio_in[Ichan]);}
		for (int Ichan = 0; Ichan < 2; Ichan++) {if (audio_out[Ichan]) AudioStream_F32::release(audio_out[Ichan]);}
		return false;
	}

	//there is data, but print an error message if any data is missing
	//if (any_data_present) {
	//	for (int Ichan = 0; Ichan < 4; Ichan++) {
	//		if (audio_in[Ichan] == NULL) {
	//			Serial.print("EarpieceMixer_F32: update: allcoateAndGetAudioBlocks: chan: "); Serial.print(Ichan); Serial.println(" missing...");
	//		}
	//	}
	//}

	//allocate a bunch of working memory
	bool any_allocate_fail = false;
	for (int i = 0; i < 6; i++) {
		tmp[i] = allocate_f32(); 
		if (tmp[i] == NULL) any_allocate_fail = true;
	}
	if (any_allocate_fail) {
		//we must be out of audio memory...so release any buffers that were allocated and then return
		for (int i = 0; i < 6; i++) { if (tmp[i]) AudioStream_F32::release(tmp[i]); }
		for (int Ichan = 0; Ichan < 4; Ichan++) {if (audio_in[Ichan]) AudioStream_F32::release(audio_in[Ichan]);}
		for (int Ichan = 0; Ichan < 2; Ichan++) {if (audio_out[Ichan]) AudioStream_F32::release(audio_out[Ichan]);}
		return false;
	}

	//if we've gotten this far, we're successful!
	return true;
};


// ///////////////////////////////////////////////////////////////////////////////////////////////

int EarpieceMixer_F32::configureFrontRearMixer(int val) {
  float rearMicScaleFac = sqrtf(powf(10.0, 0.1 * state.rearMicGain_dB));
  switch (val) {
    case EarpieceMixerState::MIC_FRONT:
      frontRearMixer[LEFT].gain(FRONT, 1.0); frontRearMixer[LEFT].gain(REAR, 0.0);
      frontRearMixer[RIGHT].gain(FRONT, 1.0); frontRearMixer[RIGHT].gain(REAR, 0.0);
      state.input_frontrear_config = val;
      setMicDelay_samps(FRONT, 0); setMicDelay_samps(REAR, 0);
      break;
    case EarpieceMixerState::MIC_REAR:
      frontRearMixer[LEFT].gain(FRONT, 0.0); frontRearMixer[LEFT].gain(REAR, rearMicScaleFac);
      frontRearMixer[RIGHT].gain(FRONT, 0.0); frontRearMixer[RIGHT].gain(REAR, rearMicScaleFac);
      state.input_frontrear_config = val;
      setMicDelay_samps(FRONT, 0); setMicDelay_samps(REAR, 0);
      break;
    case EarpieceMixerState::MIC_BOTH_INPHASE:
      frontRearMixer[LEFT].gain(FRONT, 0.5); frontRearMixer[LEFT].gain(REAR, 0.5 * rearMicScaleFac);
      frontRearMixer[RIGHT].gain(FRONT, 0.5); frontRearMixer[RIGHT].gain(REAR, 0.5 * rearMicScaleFac);
      state.input_frontrear_config = val;
      setMicDelay_samps(FRONT, 0); setMicDelay_samps(REAR, 0);
      break;
    case EarpieceMixerState::MIC_BOTH_INVERTED:
      frontRearMixer[LEFT].gain(FRONT, 0.75); frontRearMixer[LEFT].gain(REAR, -0.75 * rearMicScaleFac);
      frontRearMixer[RIGHT].gain(FRONT, 0.75); frontRearMixer[RIGHT].gain(REAR, -0.75 * rearMicScaleFac);
      state.input_frontrear_config = val;
      setMicDelay_samps(FRONT, 0); setMicDelay_samps(REAR, 0);
      break;
    case EarpieceMixerState::MIC_BOTH_INVERTED_DELAYED:
      frontRearMixer[LEFT].gain(FRONT, 0.75); frontRearMixer[LEFT].gain(REAR, -0.75 * rearMicScaleFac);
      frontRearMixer[RIGHT].gain(FRONT, 0.75); frontRearMixer[RIGHT].gain(REAR, -0.75 * rearMicScaleFac);
      state.input_frontrear_config = val;
      setMicDelay_samps(FRONT, state.targetFrontDelay_samps); setMicDelay_samps(REAR, state.targetRearDelay_samps);
      break;
  }
  return state.input_frontrear_config;
}


int EarpieceMixer_F32::setInputAnalogVsPDM(int input) {
  int prev_val = state.input_mixer_config; //get the left-right mix/mute state
  configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MUTE); //temporarily mute the system

  switch (input) {
    case EarpieceMixerState::INPUT_PDM:
      myTympan->enableDigitalMicInputs(true);  //two of the earpiece digital mics are routed here
      earpieceShield->enableDigitalMicInputs(true);  //the other two of the earpiece digital mics are routed here
      analogVsDigitalSwitch[LEFT].switchChannel(input);
      analogVsDigitalSwitch[RIGHT].switchChannel(input);
      state.input_analogVsPDM = input;
      break;
    case EarpieceMixerState::INPUT_ANALOG:
      myTympan->enableDigitalMicInputs(false);  //two of the earpiece digital mics are routed here
      earpieceShield->enableDigitalMicInputs(false);  //the other two of the earpiece digital mics are routed here
      analogVsDigitalSwitch[LEFT].switchChannel(input);
      analogVsDigitalSwitch[RIGHT].switchChannel(input);
      state.input_analogVsPDM = input;
      break;
  }

  configureLeftRightMixer(prev_val); //unmute the system and return to the initial left-right mix state
  return state.input_analogVsPDM;
}


//Choose the analog input to use
int EarpieceMixer_F32::setAnalogInputSource(int input_config) {
  int prev_val = state.input_mixer_config;  //get the left-right mix/mute state
  configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MUTE); //temporarily mute the system

  switch (input_config) {
    case EarpieceMixerState::INPUT_PCBMICS:
      //Select Input
      myTympan->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      //earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);  //this doesn't really exist

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      state.input_analog_config = input_config;
      break;

    case EarpieceMixerState::INPUT_MICJACK_MIC:
      //Select Input
      myTympan->inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      earpieceShield->inputSelect(TYMPAN_INPUT_JACK_AS_MIC);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      state.input_analog_config = input_config;
      break;

    case EarpieceMixerState::INPUT_MICJACK_LINEIN:
      //Select Input
      myTympan->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the mic jack
      earpieceShield->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      state.input_analog_config = input_config;
      break;
    case EarpieceMixerState::INPUT_LINEIN_SE:
      //Select Input
      myTympan->inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      earpieceShield->inputSelect(TYMPAN_INPUT_LINE_IN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      state.input_analog_config = input_config;
      break;
  }

  configureLeftRightMixer(prev_val); //unmute the system and return to previous left-right mix state
  return state.input_analog_config;
}


int EarpieceMixer_F32::configureLeftRightMixer(int val) {  //call this if you want to change left-right mixing (or if the input source was changed)
  switch (val) {  // now configure that mix for the analog inputs
    case EarpieceMixerState::INPUTMIX_STEREO:
      state.input_mixer_config = val;  state.input_mixer_nonmute_config = state.input_mixer_config;
      leftRightMixer[LEFT].gain(LEFT, 1.0); leftRightMixer[LEFT].gain(RIGHT, 0.0); //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT, 0.0); leftRightMixer[RIGHT].gain(RIGHT, 1.0); //set what is sent right
      break;
    case EarpieceMixerState::INPUTMIX_MONO:
      state.input_mixer_config = val; state.input_mixer_nonmute_config = state.input_mixer_config;
      leftRightMixer[LEFT].gain(LEFT, 0.5); leftRightMixer[LEFT].gain(RIGHT, 0.5); //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT, 0.5); leftRightMixer[RIGHT].gain(RIGHT, 0.5); //set what is sent right
      break;
    case EarpieceMixerState::INPUTMIX_MUTE:
      state.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT, 0.0); leftRightMixer[LEFT].gain(RIGHT, 0.0);  //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT, 0.0); leftRightMixer[RIGHT].gain(RIGHT, 0.0); //set what is sent right
      break;
    case EarpieceMixerState::INPUTMIX_BOTHLEFT:
      state.input_mixer_config = val; state.input_mixer_nonmute_config = state.input_mixer_config;
      leftRightMixer[LEFT].gain(LEFT, 1.0); leftRightMixer[LEFT].gain(RIGHT, 0.0);  //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT, 1.0); leftRightMixer[RIGHT].gain(RIGHT, 0.0); //set what is sent right
      break;
    case EarpieceMixerState::INPUTMIX_BOTHRIGHT:
      state.input_mixer_config = val; state.input_mixer_nonmute_config = state.input_mixer_config;
      leftRightMixer[LEFT].gain(LEFT, 0.0); leftRightMixer[LEFT].gain(RIGHT, 1.0);  //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT, 0.0); leftRightMixer[RIGHT].gain(RIGHT, 1.0); //set what is sent right
      break;
  }
  return state.input_mixer_config;
}

float EarpieceMixer_F32::setInputGain_dB(float gain_dB) { 
  state.inputGain_dB = myTympan->setInputGain_dB(gain_dB);   //set the AIC on the main Tympan board
  earpieceShield->setInputGain_dB(gain_dB);  //set the AIC on the Earpiece Sheild
  return state.inputGain_dB;
}

//This only sets the *target* delay value.  It doesn't change the actual
//delay value if the system is in the mode where the delays are actually being used
int EarpieceMixer_F32::setTargetMicDelay_samps(const int front_rear, int samples) {
  //Serial.print("setTargetMicDelay_samps: setting to "); Serial.print(samples);
  int out_val = -1;
  if (samples >= 0) {
    if (front_rear == FRONT) {
      out_val = state.targetFrontDelay_samps = samples;
    } else {
      out_val = state.targetRearDelay_samps = samples;
    }
  } else {
    if (front_rear == FRONT) {
      out_val = state.targetFrontDelay_samps;
    } else {
      out_val = state.targetRearDelay_samps;
    }
  }

  //activate these new values, if that's what mode we're in
  if (state.input_frontrear_config == EarpieceMixerState::MIC_BOTH_INVERTED_DELAYED) {
    //Serial.print("setTargetMicDelay_samps: and updating current value to  "); Serial.print(myState.targetRearDelay_samps);
    out_val = setMicDelay_samps(front_rear, out_val);
  }
  return out_val;
}

int EarpieceMixer_F32::setMicDelay_samps(const int front_rear,  int samples) {
  //Serial.print("setMicDelay_samps: setting to "); Serial.print(samples);
  int out_val = -1;
  if (samples >= 0) {
    float sampleRate_Hz = frontMicDelay[LEFT].getSampleRate_Hz(); //all the delay modules *should* have the same sample rate
    float delay_msec = ((float)samples) / sampleRate_Hz * 1000.0f;

    if (front_rear == FRONT) {
      frontMicDelay[LEFT].delay(0, delay_msec);
      frontMicDelay[RIGHT].delay(0, delay_msec);
      out_val = state.currentRearDelay_samps = samples;
    } else {
      rearMicDelay[LEFT].delay(0, delay_msec);
      rearMicDelay[RIGHT].delay(0, delay_msec);
      out_val = state.currentFrontDelay_samps = samples;
    }
  } else {
    if (front_rear == FRONT) {
      out_val = state.currentFrontDelay_samps;
    } else {
      out_val = state.currentRearDelay_samps;
    }
  }
  return out_val;
}
