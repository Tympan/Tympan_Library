/*
*   Tympan_BLE_BatteryService
*
*   Created: Chip Audette, OpenAudio, Feb 2025
*            
*   Purpose: For Tympan Rev F, send values to phone via the BLE Battery Service.
*
*      You can use the Adafruit Connect App to see the battery status of any device
*      that is running the BLE battery service.  So, put this example sketch on your
*      Tympan and then connect via the Adafruit App.  In the app, you can then watch
*      the apparent battery level drop.  It is the Tympan that is sending the new
*      values.  
*
*      Be aware, however, that the value of the battery level that we're sending
*      here does NOT reflect the Tympan's actual battery level.  This skwtch is just 
*      an example of how the Tympan can use the standard BLE Battery reporting service.
*      If you did have a measurement of the Tympan's actual battery level (somehow),
*      you could use this BLE Battery Service to report the value to your phone.
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

// Instantiate myTympan
Tympan myTympan(TympanRev::F);  //only TympanRev::F has the nRF52480

// Instantiate the BLE class
BLE_nRF52& ble_nRF52 = myTympan.getBLE_nRF52();  //get BLE object for the nRF52840 (RevF only!)

// Specify our BLE parameters to be for the BLE Battery Service
int ble_service_id = BLE_nRF52::BLESVC_BATT;  //see table at the end of https://github.com/Tympan/Tympan_Rev_F_Hardware/wiki/Bluetooth-and-Tympan-Rev-F
int ble_char_id = 0;                          //The BLE Battery service only has one characteristic (which we'll call "0")
uint8_t battery_level_percent = 100;          //the BLE Battery service says that level is sent as an 8-bit unsigned inteter, 0-100.

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBluetoothSerial();  //should use the correct Serial port and the correct baud rate
  delay(1000);
  Serial.println("Tympan_BLE_BatteryService: Starting setup()...");

  //enable the BLE battery service
  delay(2000); //for Tympan RevF (with its nRF52840), it seems to need this much delay to be ready
  if (1) { String version; ble_nRF52.version(&version);  Serial.println("setup: BLE firmware version = " + version); } //optional: Must be v0.4.0 or newer!
  ble_nRF52.setBleMac("112233445566");                 //This line seems to be necessary for the next two lines to work?  Should not be required. You can choose any 6 byte Hex-code MAC you'd like
  ble_nRF52.enableServiceByID(ble_service_id, true);   //this enables the battery service (the "true" is what enables)

  //begin the BLE module
  ble_nRF52.begin_basic();  //replaces myTympan.setupBLE();
  
  //setup is complete
  Serial.println("Setup complete.");
}  //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //look for in-coming serial messages via USB
  if (Serial.available()) { Serial.println("Received from USB Serial: " + String((char)Serial.read())); }

  //respond to in coming serial messages via BLE
  if (ble_nRF52.available() > 0) { String msgFromBle;  ble_nRF52.recvBLE(&msgFromBle); Serial.println("Received from BLE: " + msgFromBle); }

  //peridically update the battery charge status
  serviceNotifyBatteryLevel(millis(), 5000UL);  //send updated value every 5 seconds

}  //end loop();


// ///////////////// Servicing routines


//Test to see if enough time has passed to send up updated CPU value to the App
void serviceNotifyBatteryLevel(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0;     //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) {  //is it time to do something?

    //decrement our battery value
    battery_level_percent -= 5;
    if (battery_level_percent < 5) battery_level_percent = 100;

    //send the battery status value
    Serial.println("loop: sending updated battery level: " + String(battery_level_percent));
    ble_nRF52.notifyBle(ble_service_id, ble_char_id, battery_level_percent);

    lastUpdate_millis = curTime_millis;
  }  // end if
}  //end serviceUpdateCPUtoGUI();
