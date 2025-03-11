/*
*   Tympan_BLE_LedButtonService
*
*   Created: Chip Audette, OpenAudio, Feb 2025
*            
*   Purpose: For Tympan Rev F, send values to phone via the nRF LED Button Service.
*
*      You can use the nRF "Blinky" App to interact with a Tympan that is running
*      this sketch.  Once this sketch is loaded onto the Tympan, the Tympan will
*      advertise that it is running the LED Button service.  The nRF Blinky App
*      will see the Tympan and be able to connect to it.
*
*      This sketch will have the Tympan periodically transmit that a button has
*      been pressed (or not pressed).  In reality, of course, there is no button
*      on the Tympan to be pressed.  This is just an example of how you can send
*      values from the Tympan via a standard BLE service.
*
*      Also, you can use the nRF Blinky App to send a value back to the Tympan.
*      The App lets you transmit whether you want the LED to be On or Off.
*      When you send a value from the App, you will see that this value is received
*      by the Tympan and printed to the Serial Monitor.  The nRF Blinky App will
*      only send a zero or a one.  If you use nRF Connect App or the Adafruit
*      Connect App instead of nRF Blinky, you can send any 8-bit value and receive
*      it on the Tympan.
*
*   nRF Blinky App: https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrfblinky
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
int ble_service_id = BLE_nRF52::BLESVC_LEDBUTTON;  //see table at the end of https://github.com/Tympan/Tympan_Rev_F_Hardware/wiki/Bluetooth-and-Tympan-Rev-F
int ble_char_id = 0;                               //assumed that we want the zero'th characteristic for sending our data value (the one'th characteristic is for receiving)
uint8_t button_value = 0;                          //zero is "not pressed", one is "pressed".  You can also send any 8-bit value you want

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  //USB Serial.  This begin() isn't really needed on Teensy.
  myTympan.beginBluetoothSerial();  //should use the correct Serial port and the correct baud rate
  delay(1000);
  Serial.println("Tympan_BLE_LedButtonService: Starting setup()...");

  //enable the BLE battery service
  delay(2000);  //for Tympan RevF (with its nRF52840), it seems to need this much delay to be ready
  if (1) {
    String version;
    ble_nRF52.version(&version);
    Serial.println("setup: BLE firmware version = " + version);
  }                                                      //optional: Must be v0.4.0 or newer!
  ble_nRF52.setBleMac("112233445566");                   //This line seems to be necessary for the next two lines to work?  Should not be required. You can choose any 6 byte Hex-code MAC you'd like
  ble_nRF52.enableServiceByID(ble_service_id, true);     //Enables the LED Button service (the "true" is what enables)
  ble_nRF52.enableAdvertiseServiceByID(ble_service_id);  //Include the LED Button service when advertising (requried for the nRF Blinky App to see the Tympan)

  //begin the BLE module
  ble_nRF52.begin_basic();  //replaces myTympan.setupBLE();

  //setup is complete
  Serial.println("Setup complete.");
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

    //print info about the BLE message
    Serial.print("Loop: Received BLE Message:");
    Serial.print("service_id = " + String(ble_msg.service_id));
    Serial.print(", characteristic_id = " + String(ble_msg.characteristic_id));
    Serial.print(", data = ");  for (uint8_t val : ble_msg.data) Serial.print(val); //general way of printing all of the bytes in the payload
    Serial.println();    

    //light up the LED based on the received BLE message (for fun)
    uint8_t val = ble_msg.data[0];
    myTympan.setRedLED(val);  myTympan.setGreenLED(val);
  }

  //peridically update the battery charge status
  serviceNotifyButtonPress(millis(), 5000UL);  //send updated value every 5 seconds

}  //end loop();


// ///////////////// Servicing routines


//Test to see if enough time has passed to send up updated CPU value to the App
void serviceNotifyButtonPress(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0;     //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) {  //is it time to update the user interface?

    // swap our button state and send
    if (button_value < 1) {
      button_value = 1;
    } else {
      button_value = 0;
    }

    //transmit the new value
    Serial.println("loop: sending updated button value: " + String(button_value));
    ble_nRF52.notifyBle(ble_service_id, ble_char_id, button_value);

    lastUpdate_millis = curTime_millis;
  }  // end if
}  //end serviceUpdateCPUtoGUI();
