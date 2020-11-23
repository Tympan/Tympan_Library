/*
*   Upgrad Firmware TympanBT
*
*   ***** THIS CODE IS UNDER TEST  *****
*   ***** CHECK THE DOCS FOLDER FOR A TUTORIAL *****
*
*   Created: Joel Murphy, Fall 2020
*   Purpose: Upgrade the Melody firmware version that runs on the BC127 Bluetooth Radio Module.
*             Downloads available from Sierra Wireless https://source.sierrawireless.com/
*             Tutorial located in the READ_ME that accompanies this code repository on github
*             <url>
*       
*   Assumptions:  This code assumes that you already know how to reprogram your Tympan
*       Using any of the example programs that came with the Tympan_Library.  If so,
*       continue.
*       
*   How to Use:
*       1) Know whether you have a Tympan RevC or RevD:
*           * Tympan RevC has two circuit boards stacked on top of each other.  BT is RN42 module.
*           * Tympan RevD has one circuit board.  BT is BC127 module
*       2) Plug your Tympan into your PC via USB, like you're going to reprogram it
*          from here within the Arduino IDE
*       3) Turn on your Tympan
*       4) Select the correct serial port from the Arduino Tools drop down menu 
*       5) Compile and Upload this sketch
*       6) Tympan will now blink alternating LEDs.
*       7) Connect via a serial port set to 9600 BAUD to communicate with the BC127 Module
*
*   Works with Tympan Rev D
*       * Rev D uses BC127: https://learn.sparkfun.com/tutorials/understanding-the-bc127-bluetooth-module/bc127-basics
*
*   MIT License.  use at your own risk. WYSIWYG
*/

#include <Tympan_Library.h>

  Tympan     myTympan(TympanRev::D); 

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;
String response;

int blinkDelay = 400;
unsigned long blinkTimer;
boolean LEDstate = true;

void echoIncomingUSBSerial(void) {
  while (USB_Serial->available()) {
    BT_Serial->write(USB_Serial->read()); //echo messages from USB Serial over to BT Serial
  }
}

void echoIncomingBTSerial(void) {
  while (BT_Serial->available()) {
    USB_Serial->write(BT_Serial->read());//echo messages from BT serial over to USB Serial
  }
}

void readBTResponse(void) {
   response = BT_Serial->readString();
}

#include "upgradeMelody_RevD.h"

// Arduino setup() function, which is run once when the device starts
void setup() {
  blinkTimer = millis();
  myTympan.setAmberLED(LEDstate); myTympan.setRedLED(!LEDstate); LEDstate = !LEDstate;
  // Start the serial ports on the Tympan
  myTympan.beginBothSerial(9600,9600);  // set the baud for the BC127 and the Firmware Upgrade Tool (PC)
  delay(2000); // why?
  USB_Serial = myTympan.getUSBSerial();
  BT_Serial = myTympan.getBTSerial();
  USB_Serial->println("*** TypmanBT Serial 9600 Pasthrough Starting...");

  // Issue the commands to change the BT name (and set other BT settings)
  if (myTympan.getTympanRev() == TympanRev::C) {
    USB_Serial->println("!!        Connected to Tympan Rev C        !!");
    USB_Serial->println("!! Do Not Attempt Melody Firmware Upgrade  !!");
  } else {
    enterCommandMode_RevD(); 
  }
  blinkTimer = millis();
}

void loop() {
  echoIncomingUSBSerial();     //echo messages from USB Serial over to BT Serial
  echoIncomingBTSerial();      //echo messages from BT serial over to USB Serial
  blinkTympanLEDs();
} 


void blinkTympanLEDs(){
  if(millis() - blinkTimer > blinkDelay){
    blinkTimer = millis();
    myTympan.setAmberLED(LEDstate); myTympan.setRedLED(!LEDstate);
    LEDstate = !LEDstate;
  }
}
