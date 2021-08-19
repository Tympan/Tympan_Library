
// Instantiate the audio classess
AudioInputI2S_F32           i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
EarpieceMixer_F32_UI        earpieceMixer(audio_settings);  //mixes earpiece mics, allows switching to analog inputs, mixes left+right, etc
AudioFilterBiquad_F32       hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32       hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectCompWDRC_F32_UI  comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.  UI enabled!!!
AudioEffectCompWDRC_F32_UI  comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right. UI enabled!!!
AudioOutputI2S_F32          i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

// Make all of the audio connections
AudioConnection_F32         patchcord1(i2s_in,0,earpieceMixer,0);
AudioConnection_F32         patchcord2(i2s_in,1,earpieceMixer,1);
AudioConnection_F32         patchcord3(i2s_in,2,earpieceMixer,2);
AudioConnection_F32         patchcord4(i2s_in,3,earpieceMixer,3);

//connect the left and right outputs of the earpiece mixer to the two filter modules (one for left, one for right)
AudioConnection_F32         patchCord11(earpieceMixer,  earpieceMixer.LEFT,  hp_filt1, 0);   //connect the Left input to the left highpass filter
AudioConnection_F32         patchCord12(earpieceMixer,  earpieceMixer.RIGHT, hp_filt2, 0);   //connect the Right input to the right highpass filter

// connect to the compressors
AudioConnection_F32        patchCord21(hp_filt1, 0, comp1, 0);    //connect to the left gain/compressor/limiter
AudioConnection_F32        patchCord22(hp_filt2, 0, comp2, 0);    //connect to the right gain/compressor/limiter

//Connect the gain modules to the outputs so that you hear the audio
AudioConnection_F32        patchcord31(comp1, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_TYMPAN);    //First AIC, Main tympan board headphone jack, left channel
AudioConnection_F32        patchcord32(comp2, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_TYMPAN);   //First AIC, Main tympan board headphone jack, right channel
AudioConnection_F32        patchcord33(comp1, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_EARPIECE);  //Second AIC (Earpiece!), left output
AudioConnection_F32        patchcord34(comp2, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Secibd AIC (Earpiece!), right output

// Note About the Outputs Being Defined Above: 
//   The four lines above assign audio to the four outputs available on the Tympan+EarpieceShield.  Two
//   of these outputs are part of the base Tympan board (and come out the Tympan's standard headphone jack).
//   The other two inputs are part of the Earpiece shield.  The earpiece shield's audio comes out the two
//   earpieces *and* it comes out of the additional headphone jack that is built into the Earpiece shield.
//
//   This is a lot of potential outputs.  Let's talk about flexibility...
//
//   The two outputs on the Tympan board and two of the outputs on the EarpieceShield can all be sent
//   indpendent signals.  Or they can be sent the same signals.  Whatever you want!  In the example above,
//   there is a left and right audio stream.  The left is sent to both the Tympan board and to the EarpieceShield.
//   The right is also sent to both the Tympan baord and to the Earpiece shield.  This is simple but it doesn't
//   have to be that way.  You could send different audio (such as the raw input audio?) to the Tympan's headphone
//   jack while still listening to the processed audio through the earpieces.  Fun!
//
//   The only rigid, non-flexible routing is that the headphone jack on the EarpieceShield is always outputing
//   the same audio that is going to the earpieces.  That is simply how they are wired.  There is not flexibility
//   here (unless you get out your soldering iron!)  :)
