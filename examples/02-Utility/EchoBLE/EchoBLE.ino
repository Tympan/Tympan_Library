/*
*	EchoBLE
*
*	Created: Ira Ray Jenkins, Creare, July 2020
*	Purpose: Demonstrates BLE functionality of the Tympan BC127 Bluetooth module. Data
		received via BLE will be echo'd back.
*
*/
#include <Tympan_Library.h>

Tympan myTympan(TympanRev::D);

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;
String response;

void echoIncomingUSBSerial(void) {
  while (USB_Serial->available()) {
    BT_Serial->write(USB_Serial->read());
  }
}

void echoIncomingBTSerial(void) {
  while(BT_Serial->available()) {
    USB_Serial->write(BT_Serial->read());
  }
}

void readBTResponse(void) {
  response = BT_Serial->readString();
}

void setup() {
  myTympan.beginBothSerial(); delay(2000);
  USB_Serial = myTympan.getUSBSerial();
  BT_Serial = myTympan.getBTSerial();
  USB_Serial->println("*** Setup Starting...");

  BLE_Setup();

  USB_Serial->println("*** Setup Complete.");
  
}

void loop() {
 while(BT_Serial->available()) {
    response = BT_Serial->readString();

    USB_Serial->println("Response: " + response.trim());
    
    if(response.substring(0,8) == "RECV BLE") {
      USB_Serial->println("*** Received: " + response.substring(9));

      BT_Serial->print("SEND Received: " + response.substring(9) + "\r");

      delay(200);
      
      BT_Serial->readString();
      
    }
  }
}

void BLE_Setup(void) {
  // get into command mode
  USB_Serial->println("*** Switching Tympan RevD BT module into command mode...");
  myTympan.forceBTtoDataMode(false); delay(500);
  BT_Serial->print("$"); delay(400);
  BT_Serial->print("$$$"); delay(400);
  delay(2000);
  echoIncomingBTSerial();
  echoIncomingUSBSerial();
  USB_Serial->println("*** Should be in command mode.");

  USB_Serial->println("*** Clearing buffers. Sending carriage return.");
  BT_Serial->print("\r"); delay(500);
  USB_Serial->println("*** Should have gotten 'Error', which is fine here.");

  // restore factory defaults
  USB_Serial->println("*** Restoring factory defaults...");
  BT_Serial->print("RESTORE"); BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("RESET"); BT_Serial->print('\r'); delay(1500);echoIncomingBTSerial();
  USB_Serial->println("*** Reset should be complete.  Did it give two 'OK's, an ID blurb, and then 'READY'?");

  // Is this necessary?
  // set the GP IO pins so that the data mode / command mode can be set by hardware
  USB_Serial->println("*** Setting GPIOCONTROL mode to OFF, which is what we need...");
  BT_Serial->print("SET GPIOCONTROL=OFF");BT_Serial->print('\r'); delay(500);echoIncomingBTSerial();
  BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500); echoIncomingBTSerial();

  // set advertising
  USB_Serial->println("*** Set advertising on...");
  BT_Serial->print("ADVERTISING ON\r"); delay(500); echoIncomingBTSerial();
  USB_Serial->println("*** Advertising on.");

  while(1) {
    if(BT_Serial->available()) {
      readBTResponse();
      USB_Serial->print("*** Response = "); USB_Serial->println(response);
      response.trim();
      
      if(response.equalsIgnoreCase("OPEN_OK BLE")) {
        USB_Serial->println("*** Connected.");
        break;
      }

      delay(2000);
    }
  }

  USB_Serial->println("*** BLE Connected.");
}
