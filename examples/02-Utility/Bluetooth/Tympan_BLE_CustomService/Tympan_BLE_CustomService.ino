/*
*   Tympan_BLE_CustomService
*
*   Created: Chip Audette, OpenAudio, Mar 2025
*            
*   Purpose: For Tympan Rev F, send values to/from the phone by defining your own
*      custom BLE service and custom BLE characteristics.  Use the nRF Connect
*      app to see your custom service/characteristics and to send and receive values.
*
*      You can also use the Adafruit Bluefruit Connect App, but I'm not sure that
*      their app lets you send values from the phone to the BLE device.
*
*   nRF Connect App: https://play.google.com/store/search?q=nrf+connect+ble&c=apps
*   Adafruit Bluefruit Connect App: https://play.google.com/store/apps/details?id=com.adafruit.bluefruit.le.connect
*   
*   For Tympan RevF, which requires you to set the Arduino IDE to compile for Teensy 4.1
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"

//create audio library objects for handling the audio
Tympan myTympan(TympanRev::F);  //only TympanRev::F has the nRF52480

// Create classes for controlling the system
#include "SerialManager.h"
BLE_nRF52& ble_nRF52 = myTympan.getBLE_nRF52();  //get BLE object for the nRF52840 (RevF only!)
SerialManager serialManager(&ble_nRF52);         //create the serial manager for real-time control (via USB or App)

// Define BLE parameters for the BLE Battery Service
int ble_service_id = BLE_nRF52::BLESVC_GENERIC_1;     //see table at the end of https://github.com/Tympan/Tympan_Rev_F_Hardware/wiki/Bluetooth-and-Tympan-Rev-F
BLE_nR52_CustomService myBleService(&ble_nRF52, ble_service_id); //create a custom service.  We will define it later

// Define a simple version of our custom service..one characteristic
void createCustomBleService_simple(void) {
  myBleService.setServiceUUID("FFEEDDCCBBAA99887766554433221100");
  myBleService.setServiceName("MyBleService");

  //Note: "READ"/"NOTIFY" is data going from the Tympan to the phone.
  //      "WRITE" is data going from the phone to the Tympan
  Serial.println("createCustomBleService_simple: setting up characteristic 0...");
  myBleService.addCharacteristic("FFEEDDCCBBAA99887766554433221101"); //this creates the first characteristic (0th characteristic)
  myBleService.setCharProperties(0, "00011010"); // set as READ (bit 1) and WRITE (bit 3) and NOTIFY (bit 4)
  myBleService.setCharName(0,"MyReadWrite");
  myBleService.setCharNBytes(0, 1);  //characteristic will send or receive 1 bytes
}

//define more complex version of our custom service...three characteristics
void createCustomBleService_complex(void) {
  myBleService.setServiceUUID("FFEEDDCCBBAA99887766554433221108");
  myBleService.setServiceName("MyBleService");

  //Note: "READ"/"NOTIFY" is data going from the Tympan to the phone.
  //      "WRITE" is data going from the phone to the Tympan
  Serial.println("createCustomBleService_complex: setting up characteristic 0...");
  myBleService.addCharacteristic("FFEEDDCCBBAA99887766554433221109"); //this creates the first characteristic (characteristic "0")
  myBleService.setCharProperties(0, "00011010"); // set as READ (bit 1) and WRITE (bit 3) and NOTIFY (bit 4)
  myBleService.setCharName(0,"MyReadWrite");  
  myBleService.setCharNBytes(0, 1);  //characteristic will send or receive 1 byte

  Serial.println("createCustomBleService_complex: setting up characteristic 1...");
  myBleService.addCharacteristic("FFEEDDCCBBAA9988776655443322110A"); //this creates the first characteristic (characteristic "1")
  myBleService.setCharProperties(1, "00010010"); // set as READ (bit 1) and NOTIFY (bit 4)
  myBleService.setCharName(1,"MyRead1");  
  myBleService.setCharNBytes(1, 4);  //characteristic will send  4 bytes

  Serial.println("createCustomBleService_complex: setting up characteristic 2...");
  myBleService.addCharacteristic("FFEEDDCCBBAA9988776655443322110B"); //this creates the first characteristic (characteristic "2")
  myBleService.setCharProperties(2, "00001010"); // set as READ (bit 1) and Write (bit 3)
  myBleService.setCharName(2,"MyWrite1");    //confusingly, for the phone to get this name, the READ bit (above) must be true, even if you just want WRITE
  myBleService.setCharNBytes(2, 4);  //characteristic will receive 4 bytes
}

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  //USB Serial.  This begin() isn't really needed on Teensy.
  myTympan.beginBluetoothSerial();  //should use the correct Serial port and the correct baud rate
  delay(1000);
  Serial.println("Tympan_BLE_GenericService: Starting setup()...");

  //enable the BLE battery service
  delay(2000);  //for Tympan RevF (with its nRF52840), it seems to need this much delay to be ready
  if (1) {
    String version;
    ble_nRF52.version(&version);
    Serial.println("setup: BLE firmware version = " + version);
  }                                                      //optional: Must be v0.4.0 or newer!
 
  //setup our BLE service
  ble_nRF52.setBleMac("112233445566");                   //This line seems to be necessary for the next two lines to work?  Should not be required. You can choose any 6 byte Hex-code MAC you'd like
  if (1) {
    //simple
    createCustomBleService_simple();
  } else {
    //complex
    createCustomBleService_complex();
  }
  ble_nRF52.enableAdvertiseServiceByID(ble_service_id);  //Include the LED Button service when advertising (requried for the nRF Blinky App to see the Tympan)

  //begin the BLE module
  ble_nRF52.begin_basic();  //replaces myTympan.setupBLE();

  //setup is complete
  Serial.println("Setup complete.");
  serialManager.printHelp();
}  //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //look for in-coming serial messages via USB
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());  //USB Serial

  //respond to in coming serial messages via BLE
  if (ble_nRF52.available() > 0) {
    String msgFromBle;
    ble_nRF52.recvBLE(&msgFromBle);                                                                     //get BLE messages (removing non-payload messages)
    for (unsigned int i = 0; i < msgFromBle.length(); i++) serialManager.respondToByte(msgFromBle[i]);  //interpet each character of the message
  }

  //check to see if any BLE messages have been received
  if (serialManager.isBleDataMessageAvailable()) {  //this method is in SerialManagerBase.h
    BleDataMessage ble_msg = serialManager.getBleDataMessage(); // BleDataMessage is in BLE/BleTypes.h

    //print the message to the Serial Monitor
    Serial.print("Received BLE Msg: ");
    Serial.print("service_id = "); Serial.print(String(ble_msg.service_id));
    Serial.print(", char_id = "); Serial.print(ble_msg.characteristic_id);
    Serial.print(", n bytes = "); Serial.print(ble_msg.data.size());
    if (ble_msg.data.size() == 1) {
      //single byte
      uint8_t val = ble_msg.data[0];
      Serial.print(", data (uint8) = ");  
      Serial.print(val); Serial.print(" (0x"); Serial.print(val,HEX); Serial.print(")");
    } else if (ble_msg.data.size() == 2) {
      //4 bytes, assume uint16...little endian
      uint16_t val16 = ble_msg.data[0] + (ble_msg.data[1]<<8);
      Serial.print(", data (uint16, LSB) = ");  Serial.print(val16);
      Serial.print(" (");
      for (uint8_t val : ble_msg.data) { Serial.print(" 0x"); Serial.print(val, HEX); }
      Serial.print(")");
    } else if (ble_msg.data.size() == 4) {
      //4 bytes, assume uint32...little ending
      uint32_t val32 = ble_msg.data[0] + (ble_msg.data[1]<<8) + (ble_msg.data[2]<<16) + (ble_msg.data[3]<<24);
      Serial.print(", data (uint32, LSB) = ");  Serial.print(val32);
      Serial.print(" (");
      for (uint8_t val : ble_msg.data) { Serial.print(" 0x"); Serial.print(val, HEX); }
      Serial.print(")");
    } else {
      //other number of bytes
      Serial.print(", data = ");
      for (uint8_t val : ble_msg.data) { Serial.print(" 0x"); Serial.print(val, HEX); }
    }
    Serial.println();    

  }

}  //end loop();


