/*
  SDTransferToPC
   
  Created: Chip Audette, OpenHearing, Oct 2024
  Purpose: Transfer files from your PC to the Tympan via the Serial link.  To send and
    receive the files, you'll want some sort of program (Python script included here) 
    to open the file on your PC and to either send the file's bytes to the Tympan or to
    receive the bytes from teh Tympan.  The Arduino Serial Monitor will not help you send
    or receive any files.  Sorry.

  Requirements: Obviously, you need a Tympan and you need to put an SD card in your Tympan.

  How to send a file from the Tympan's SD to your PC:
    1) [Optional] Send 'L' to list the files on the SD card
    2) Send 't' to start the transfer process from the Tympan to the PC
       * When prompted, send the filename of the file on the SD that you want to read (as text, ending with newline character)
       * Tympan will respond with the number of bytes in the file (as text, ending with newline character)
       * Tympan will respond stating that the bytes will begin being transfered
       * Tympan will send all of the file's bytes

  How to receive a file from your PC to the Tympan's SD
    1) Send 'T' to start the transfer process from the PC to the Tympan
       * When prompted, send the filename you want to write (as text, ending with newline character)
       * When prompted, send the length of the file in bytes (as text, ending with newline character)
       * When prompted, send all of the file's bytes
       * Tympan will respond with message about success or failure

    If you cannot connect to the Tympan via the included Python script, be sure that
    you have closed the Arduino Serial Monitor.  Only one program can use your PC's
    serial link to the Tympan.  If the Arduino Serial Monitor is open, it will
    block any other program's attempts to open a connection to the Tympan!

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.
   For Tympan Rev F, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/

// Include all the of the needed libraries
#include        <Tympan_Library.h>
#include        "SdFat.h"  // included as part of the Teensy installation
#include        "SerialManager.h"

// Create the entities that we need
Tympan          myTympan(TympanRev::F);        //use TympanRev::D or E or F
SdFs            sd;                            //This is the SD card.  SdFs is part of the Teensy install
SdFileTransfer  sdFileTransfer(&sd, &Serial);  //Transfers raw bytes of files from Serial to the SD card.  Part of Tympan_Library
SerialManager   serialManager;                 //create the serial manager for real-time control (via USB)


// ///////////////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //myTympan.beginBothSerial(); //only needed if using bluetooth
  delay(500);  while ((millis() < 2000) && (!Serial)) { delay(10); }  //stall a bit to see if Serial is connected
  Serial.println("SDTransferToPC: setup():...");

  //enable the Tympan
  myTympan.enable();

  //Set the state of the LEDs (just to see that the Tympan is alive)
  myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW); 

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),LOW);  //update blinking at LOW speed

} //end loop()


// ///////////////////////////////////////////////////////////////////////////////////////////

// This is only needed to create a dummy file on the SD card so that you have
// something available to download off the SD card
void createDummyFileOnSD(void) {
  String fname = "dummy.txt";
  Serial.println("SerialManager: creating dummy text file " + fname);

  //begin the SD card support
  if (!(sd.begin(SdioConfig(FIFO_SDIO)))) { Serial.println("SerialManager: cannot open SD.");  return; }

  //open a file
  FsFile file;
  file.open(fname.c_str(),O_RDWR | O_CREAT | O_TRUNC);   //open for writing
  if (!file.isOpen()) { Serial.println("SerialManager: cannot open file for writing: " + fname);  return; }

  //write some text to the file
  unsigned int total_bytes = 0;
  total_bytes += file.println("abcdefghijklmnopqrstuvwxyz");
  total_bytes += file.println("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  total_bytes += file.println("0123456789!@#$%^&*()");
  total_bytes += file.println("Have a great day!");

  //close the file
  file.close();
  Serial.println("SerialManager: wrote " + String(total_bytes) + " bytes to " + fname);
}
