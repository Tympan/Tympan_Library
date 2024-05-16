/*
 * LongAudioBlocks
 *
 * CREATED: Chip Audette, OpenAudio, Oct 2023
 * PURPOSE: Demonstrate using long audio blocks.
 * 
 * BACKGROUND: The Tympan processes audio in blocks.  By default, the Tympan block 
 * size is 128 samples per channel. Most users want to use short audio blocks as this
 * minimizes the latency of the audio passing through the Tympan.  But, some users are
 * interested in using audio blocks that are longer than 128 points per channel.  
 * Historically, this was not possible with the Tympan Library.
 * 
 * The historical limitation was inhereted from the Teensy Audio library upon which the
 * Tympan Library is based.  In the Teensy library, a number of underlying data buffers are 
 * are pre-allocated to 128 points.  In the Tympan_Library, we allow you to use audio 
 * blocks of less than 128 points because we can simply use less of the pre-allocated
 * buffers. Each buffer was still 128 points long; we simply used only a portion of it.
 * 
 * For users wanting blocks longer than 128 points, we were limited by the fixed length of
 * those buffers.  They were simply not long enough to permit longer blocks.  In Oct of 2023,
 * however, I generalized the underlying system to step away from this limitation.   It works!
 *
 * Unfortunatley, it's not as pretty as it should be.  The system ought to be able to
 * dynamically allocate data arrays for those underlying buffers.  But, there's something
 * weird about were the memory ends up (when using a "new" command) that the underlying
 * system doesn't like.  So, instead, we have the user (you!), allocate your own buffers
 * in your global scope and then hand your buffers to the Tympan Library.  You can allocate
 * your buffers to be however long you'd like (within reason).  Now you can use longer 
 * audio blocks!  Use this example to see how!
 * 
 * KEY FEATURES: This example shows the two key features needed to enable large audio blocks:
 *     #1) This code allocates its own large buffers for the I2S input and I2S output
 *     #2) This code hands those large buffers to the I2S input and I2S output classes
 *         when they are instantiated.
 *
 * License: MIT License, Use At Your Own Risk
 *
 */

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //choose whatever sample rate you'd like (up to 96000)
const int audio_block_samples = 1024;    //choose any factor of 2.  Greater than 128 is "big".  Tested up to 1024.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


//Here is critical feature #1 for using long audio blocks: for blocks > 128 poits per channel, you
//need to allocate your own I2S buffers
const int n_chan = 2;  // 2 = stereo, 4 = quad.  Don't choose anything else
uint32_t i2s_rx_buffer[audio_block_samples/2*n_chan]; //allocate the buffer for the InputI2S class
uint32_t i2s_tx_buffer[audio_block_samples/2*n_chan]; //allocate the buffer for the OutputI2S class


// define classes to control the Tympan and the AIC_Shield
Tympan                    myTympan(TympanRev::F, audio_settings);   //do TympanRev::D or E or F

// define audio classes
AudioInputI2S_F32         i2s_in(audio_settings, i2s_rx_buffer);   //Critical Feature #2: Pass the big buffer to the Tympan Library
AudioOutputI2S_F32        i2s_out(audio_settings, i2s_tx_buffer);  //Critical Feature #2: Pass the big buffer to the Tympan Library

// define audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0); //connect first left input to first left output
AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1); //connect first right input to first right output


// define the setup() function, the function that is called once when the device is booting
void setup(void)
{
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();   delay(2500);
  Serial.println("LongAudioBlocks: Starting setup()...");
  Serial.println("    : Sample rate = " + String(sample_rate_Hz) + " Hz");
  Serial.println("    : block size = " + String(audio_block_samples) + " samples per channel");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); 

  //Choose the desired input for Tympan main board
   myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones (only on main board, not on AIC shield)
   //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);     // use the on board microphones (only on main board, not on AIC shield)
   //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
   myTympan.setInputGain_dB(15.0); // set input volume, 0-47.5dB in 0.5dB setps
  
  //Set the desired output volume levels
  myTympan.volume_dB(0);      // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
 
  Serial.println("Setup complete.");
}

void loop(void)
{
  // Nothing to do - just looping input to output
  delay(2000);
  printCPUandMemoryMessage();
}

void printCPUandMemoryMessage(void) {
    Serial.print("CPU Cur/Pk: ");
    Serial.print(audio_settings.processorUsage(),1);
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax(),1);
    Serial.print("%, ");
    Serial.print("MEM Cur/Pk: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();
}
