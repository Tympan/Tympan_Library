/*
   SDPlay_and_SDWrite
   
   Created: Chip Audette, OpenHearing, Mar 2024
   Purpose: Read audio from SD card and Write audio to SD card at the same time.

   Background: The Teensy default AudioSdPlayer does not work well when one is also
      using the AudioSdWriter.  The issue is that the default AudioSdPlayer reads from
      the SD card in its update() method, which is part of the interrupt-driven part of
      the audio handling.  Being interrupt-driven, it can interrupt the SD writing that
      is being performed by the AudioSdWriter.  This seems to cause problems.

   Solution: This example uses the revised AudioSdPlayer that is in the Tympan_Library.
      The Tympan's AudioSdPlayer moves the SD reading activities out of the update()
      function and into a "serviceSD()" method that you have to call regularly from the
      Arduino loop().  While it is annoying to have to remember to include the serviceSD
      in your main loop (as shown here), it does make it possible to stream audio from
      the SD and to the SD within the same program.  Nice!

   WARNING: Be sure to use a really good SD card.  Like a SanDisk Ultra or SanDisk Extreme.
      Poor quality SD cards might not be able to keep up with the simultaneous writing
      and recording.

   HOW TO USE:  You need to open the Serial Monitor to use this program on the Tympan.
      So, compile and upload the sketch to the Tympan, then have the Arduino IDE open
      the serial monitor (Tools menu -> Serial Monitor).  The Serial Monitor window
      should show the startup text produced by the Tympan.  If it does not, send an 'h'
      (without the quotes) to get the Tympan's help menu.  In the menu, you can see
      the options.  The most relevant commands are:
          * Send a 'p' (no quotes) to play a WAV file from the SD card
          * Send a 'r' (no quotes) to start recording a WAV file on the SD card
          * Send a 's' (no quotes) to stop recording on the SD card

   WAV FILE, PLAY: This sketch assumes that you have already put a WAV file on your SD card.
      There is an example file "PLAY1.WAV" included in the directory along with this
      sketch.  You have to manually copy the file over to the SD card prior to running
      the Tympan.

      Alternatively, you can create your own WAV file.  You can use any program, including
      Audacity. You can use any sample rate (the Tympan will ignore the WAV files sample rate)
      but, for simplicity, use a sample rate of 44100 Hz.  Your WAV can be mono or stereo.
      Your WAV can be any length.  The most crucial detail is that you must save your WAV
      using 16-bit integer samples (not 8-bit, not 24-bit, not floating point).  Then, save
      your WAV file as "PLAY1.WAV" and put it on your SD card.  
      
      If you wish to change the filename assumed by this sketch, see the variable 
      "sdPlay_filename" in the file SerialManager.h.  
  
   WAV FILE, RECORD: If you record to the SD card using this sketch, it will create a
      WAV file with two channels (left and right) using 16-bit integer samples.  The
      sample rate of the WAV file will be the same as the sample rate used by the 
      Tympan in this sketch, which is 44.1 kHz (the same as an audio CD).

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.
   For Tympan Rev F, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //Desired sample rate (we ignore the sample rate of the WAV file)
const int audio_block_samples = 128;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// define classes to control the Tympan
Tympan           myTympan(TympanRev::F, audio_settings);  //do TympanRev::D or E or F
SdFs             sd;                                      //because we're doing both a player and recorder, explicitly create the shared SD resource.  I'm not sure this is really necessary.


// /////////// Create the audio processing objects and connections

//Instantiate the audio objects
AudioInputI2S_F32        i2s_in(audio_settings);              //Bring audio in...not strictly needed for this example
AudioSDPlayer_F32        audioSDPlayer(&sd, audio_settings);  //from the Tympan_Library
AudioEffectGain_F32      gain(audio_settings);                //placeholder for whatever processing you care to do
AudioSDWriter_F32        audioSDWriter(&sd, audio_settings);  //Write audio to the SD card (if activated)
AudioOutputI2S_F32       i2s_out(audio_settings);             //Send audio out

//There are two ways that the audio flow can be configured...
#if 0
  //OPTION 1: Take audio from the SD card, process it, then write it back to the SD card
  String description = String("Read Audio from SD, Process It, and Write Audio Back to SD");

  //Connect the audio objects to each other
  AudioConnection_F32     patchcord1(audioSDPlayer, 0, gain, 0); //connect left channel of the SD WAV file to your processing
  AudioConnection_F32     patchcord2(gain, 0, audioSDWriter, 0); //connect your procssed audio to the left channel of the SD writer
  AudioConnection_F32     patchcord3(gain, 0, audioSDWriter, 1); //connect your procssed audio to the right channel of the SD writer
  AudioConnection_F32     patchcord4(gain, 0, i2s_out, 0);       //connect your procssed audio to the left headphone channel
  AudioConnection_F32     patchcord5(gain, 0, i2s_out, 1);       //connect your procssed audio to the right headphone channel 

#else
  //OPTION 2: Play audio out from the SD card while recording audio to the SD card from the analog audio input
  String description = String("Play Audio Out from SD while Recording Audio from Inputs to SD");

  //Connect the audio objects to each other
  AudioConnection_F32     patchcord1(audioSDPlayer, 0, gain, 0);   //connect left channel of the SD WAV file to your processing
  AudioConnection_F32     patchcord2(gain, 0, i2s_out, 0);         //connect your procssed audio to the left headphone channel
  AudioConnection_F32     patchcord3(gain, 0, i2s_out, 1);         //connect your procssed audio to the right headphone channel
  AudioConnection_F32     patchcord4(i2s_in, 0, audioSDWriter, 0); //connect left input to the left channel of the SD writer
  AudioConnection_F32     patchcord5(i2s_in, 1, audioSDWriter, 1); //connect right input to the right channel of the SD writer 
#endif


// /////////// Create objects for controlling the system via USB Serial 
#include         "SerialManager.h"
SerialManager    serialManager;     //create the serial manager for real-time control (via USB or App)


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //myTympan.beginBothSerial(); //only needed if using bluetooth
  delay(1500);
  Serial.println("SDPlay_and_SDWrite: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));
  Serial.println("Description: " + description);

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //enable the Tympan
  myTympan.enable();

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                      // DAC output volume:  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(0.0);              // set volume of analog inputs (if used): 0-47.5dB in 0.5dB setps

  //configure your audio processing however you'd like (if used)
  gain.setGain_dB(0.0);    //a value of 0.0 dB means that it doesn't do anything to the audio

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);             //four channels for this quad recorder, but you could set it to 2
  Serial.println("SD configured to write " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW);

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the SD reading and writing
  service_SD_read_write();
  
  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //provide status messages to the user over the Serial link
  serviceStatusMessages();

} //end loop()

// //////////////////////////////////// Servicing routines not otherwise embedded in other classes

//For the SD cards, it needs to periodically read bytes from the SD card into the SDplayer's read buffer.
//Similarly, it needs to periodically write bytes from the SDwriter's write buffer to the SD card.
//In this function, we look to see which one is closest to running out of buffer, and we service its
//needs first.
int service_SD_read_write(void) {
  //see how much empty space is left in the write buffer (as measured in milliseconds of in-coming data)
  uint32_t writeBuffer_empty_msec = 9999;
  if (audioSDWriter.isFileOpen()) writeBuffer_empty_msec = audioSDWriter.getNumUnfilledSamplesInBuffer_msec();

  //see how much data is left in the read buffer (as measured in milliseconds of out-going data)
  uint32_t playBuffer_full_msec = 9999;
  if (audioSDPlayer.isFileOpen()) playBuffer_full_msec = audioSDPlayer.getNumBytesInBuffer_msec();

  //choose which one to update first based on who's buffer is closest to running out
  if (playBuffer_full_msec < writeBuffer_empty_msec) {
    //service the player first as its buffer is closest to being out
    audioSDPlayer.serviceSD();   
    audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
  } else { 
    //service the writer first, as its buffer is closest to being out
    audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
    audioSDPlayer.serviceSD();   
  }
  return 0;
}


void serviceStatusMessages(void) {
  static unsigned long nextUpdate_millis = 0UL;
  static unsigned long updatePeriod_millis = 1000UL;  //provide update to user ever 1000 msec (ie, once per second)

  //is it time to print a status message to the serial monitor?
  if (millis() >= nextUpdate_millis) {
    //check to see if playing
    String isPlaying_txt = "False";
    bool isPlaying = false;
    if (audioSDPlayer.isPlaying()) { isPlaying=true; isPlaying_txt = "True"; }

    //check to see if writing
    String isRecording_txt = "False";
    bool isRecording = false;
    if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) { isRecording = true; isRecording_txt = "True"; }

    //print message to the user
    if (isPlaying || isRecording) {
      Serial.println("status: SD is playing = " + isPlaying_txt + ", SD is recording = " + isRecording_txt);
    }

    //set the next time for the update message to appear
    nextUpdate_millis += updatePeriod_millis;
  }
}


