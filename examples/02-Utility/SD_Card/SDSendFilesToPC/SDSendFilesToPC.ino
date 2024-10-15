/*
  SDSendFilesToPC
   
  Created: Chip Audette, OpenHearing, Oct 2024
  Purpose: Transfer files from the SD to your PC via the Serial link.  To receive the files,
    you will want to use some sort of program (Python script included here) to receive the
    data from the Tympan and write it to your PC's disk.  The Arduino Serial Monitor will
    not help you save the files.  Sorry.

  HOW TO USE: Obviously, you need a Tympan and you need to put an SD card in your Tympan.
    Since this example will be transfering files off the SD card back to your PC, your
    SD card needs some example files on it.  So, with the Tympan plugged into the PC
    via USB and turned on, you can use your serial communicaiton program to:

    1) Send 'L' to list the files on the SD card
    2) Send 'o' to ask to open a file
    3) When prompted, send the filename you want (ending with a newline character)
    4) Send 't' to transfer all the data bytes of the file over serial to the PC

    In many cases, it is easier for your serial communication program to receive the
    file if it knows how many bytes are in the file.  So, in that case, youu would 
    insert the following step:

    3.5) Send 's' to ask for the size of the file in bytes

    The file will be automatically closed when the data has all been transferred.

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
Tympan          myTympan(TympanRev::F);      //use TympanRev::D or E or F
SdFs            sd;                          //This is the SD card.  SdFs is part of the Teensy install
SDtoSerial      SD_to_serial(&sd, &Serial);  //transfers raw bytes of files on the sd over to Serial (part of Tympan Library)
SerialManager   serialManager;               //create the serial manager for real-time control (via USB)


// ///////////////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //myTympan.beginBothSerial(); //only needed if using bluetooth
  delay(500);  while ((millis() < 2000) && !Serial) delay(10);  //stall a bit to see if Serial is connected
  Serial.println("SDSendFilesToPC: setup():...");

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
