/*
*   BasicGain_wApp
*
*   Created: Chip Audette, June 2020
*   Purpose: Process audio using Tympan by applying gain.
*      Also, illustrate how to change Tympan setting via the TympanRemote App.
*
*   Blue potentiometer adjusts the digital gain applied to the audio signal.
*
*   Uses default sample rate of 44100 Hz with Block Size of 128
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote&hl=en_US
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::D);  //do TympanRev::D or TympanRev::C
AudioInputI2S_F32         i2s_in;        //Digital audio *from* the Tympan AIC.
AudioEffectGain_F32       gain1, gain2;  //Applies digital gain to audio data.
AudioOutputI2S_F32        i2s_out;       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gain1, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gain2, 0);    //connect the Right input
AudioConnection_F32       patchCord11(gain1, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(gain2, 0, i2s_out, 1);  //connect the Right gain to the Right output


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 10.0f; //gain on the microphone
float digital_gain_dB = 0.0;      //this will be set by the app
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();  
  delay(3000);
  myTympan.println("BasicGain_wApp: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) respondToByte((char)Serial.read());   //USB Serial
  if (Serial1.available()) respondToByte((char)Serial1.read()); //BT Serial

} //end loop();


// ///////////////// Servicing routines

//respond to serial commands
void respondToByte(char c) {
  myTympan.print("Received character "); myTympan.println(c);
  
  switch (c) {
    case 'J': case 'j':
      printTympanRemoteLayout();
      break;
    case 'k':
      changeGain(3.0);
      printGainLevels();
      break;
    case 'K':
      changeGain(-3.0);
      printGainLevels();
      break;
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// Please don't put commas or colons in your ID strings!
void printTympanRemoteLayout(void) {
  char jsonConfig[] = "JSON={"
    "'pages':["
      "{'title':'MyFirstPage','cards':["
            "{'name':'Change Loudness','buttons':[{'label': 'Less', 'cmd': 'K'},{'label': 'More', 'cmd': 'k'}]}"
      "]}" //no comma on the last one
    "]" 
  "}";
  myTympan.println(jsonConfig);  
  delay(50);
  myTympan.println();
}


//change the gain from the App
void changeGain(float change_in_gain_dB) {
  digital_gain_dB = digital_gain_dB + change_in_gain_dB;
  gain1.setGain_dB(digital_gain_dB);  //set the gain of the Left-channel gain processor
  gain2.setGain_dB(digital_gain_dB);  //set the gain of the Right-channel gain processor
}


//Print gain levels 
void printGainLevels(void) {
  myTympan.print("Analog Input Gain (dB) = "); 
  myTympan.println(input_gain_dB); //print text to Serial port for debugging
  myTympan.print("Digital Gain (dB) = "); 
  myTympan.println(digital_gain_dB); //print text to Serial port for debugging
}
