/*
  ChangeBC127Baudrate

  Created: Ira Ray Jenkins, Creare, August 2021 
           Extended by Chip Audette, August 2021
           
  Purpose: Demonstrate changing the baudrate of the BC127 module.

  WARNING: Be aware that this change in baud rate will persist on the BC127, even
  after cycling the power. Being at a different baud rate will prevent you from a
  ccessing the Bluetooth module from any other Tympan program.

  Therefore, if you change the BC127 baud rate, be sure that your other programs are
  set to use the new speed.  Or, if you want to change back to the "normal" BC127
  speed, use this program again to change the baud rate to the factory default of 9600.

  MIT License.  Use at your own risk.  Have fun!

*/

#include <Arduino.h>
#include <Tympan_Library.h>
BLE        ble(&Serial1);

// USER PARAMETERS
const int TARGET_BAUDRATE = 115200;    //try 115200 (fast!) or 9600 (factory original)
const int FIRMWARE_VERSION = 7;        // what Melody firmware we are dealing with
const bool USE_HARDWARE_RESET = true;  //try to use the BC127 GPIO Pins to force a factory reset

const int pin_PIO0 = 5; //RevD = 56, RevE = 5   // Pin # for connection to BC127 PIO0 pin
const int pin_RST = 9;  //RevD = 34, RevE = 9   // Pin # for connection to BC127 RST pin


// //////////////////////////////////////////// Helper functions

const int baudrate_factory = 9600;       // here is the factory default
const int baudrate1 = baudrate_factory;  //here is one possible starting baudrate for the BLE module
const int baudrate2 = 115200;            //here is the other possible starting baudrate for the BLE module
int cur_baudrate;  // this is the baudrate that we've set Serial1 to be
void setSerial1BaudRate(int new_baud) {
  Serial1.flush();
  Serial1.end();
  delay(500);
  Serial1.begin(new_baud);
  delay(500);
  cur_baudrate = new_baud;
}


//This function attempts to hold PIO 0 high and then reseting via the reset pin.
//See Melody Guide for v7 "Restoring the Default Configuration".
void hardwareReset() {
  //Set the reset pin.  "Normal" is high impedance with an external pull-up.  Let's
  //set our default to high-impedance ("INPUT") with also an internal pullup, then
  //transition to low-impedance (ie, "OUTPUT") set high
  pinMode(pin_RST, INPUT);
  digitalWrite(pin_RST, HIGH);
  pinMode(pin_RST, OUTPUT);
  digitalWrite(pin_RST, HIGH);
  
  //now set the PIO0 pin.  Normal is PIO0
  pinMode(pin_PIO0,OUTPUT);
  digitalWrite(pin_PIO0, LOW);

  //stall for a tiny bit (for no good reason)
  delay(50);

  //Now we follow the instructions in the Melody User Guide
  //
  // The default configuration can be restored using either of the following equivalent 
  // methods:
  //   * Maintain PIO 0 high (press the VOL UP button on a BC127-DISKIT) while 
  //     resetting the module
  //   * Use RESTORE
  digitalWrite(pin_PIO0,HIGH);
  delay(5); //let it setting
  
  //And, from the Datasheet, reset if low for more than 5msec
  digitalWrite(pin_RST,LOW);  //pull the pin low to start reset timer
  delay(10);                  //resets if low for more than 5 msec
  digitalWrite(pin_RST,LOW);  //return to normal

  //did it reset?  It should report its ID info to the Serial

  //return to normal
  delay(10);
  digitalWrite(pin_PIO0,LOW); //return to normal
}

int hardwareResetWithChecking(void) {
  Serial.println("setup(): Attempting a hardware reset...");
    
  //if successfull, we'll end up in the factory baudrate.  So, let's switch
  //so that we can get its boot-up message
  setSerial1BaudRate(baudrate_factory);

  //do the hardware reset
  hardwareReset();

  //check for the boot message
  delay(500);
  echoIncomingBTSerial();
  Serial.println("Did you see the boot-up info from the BC127??");

  //check BT status (another way to confirm that we have the right speed
  ret_val = checkStatusBLE();
  if (ret_val != BC127::SUCCESS) {
    Serial.println("setup(): Hardware reset FAILED to restore factory baudrate.");
    return -1; //fail
  } else {
    Serial.println("setup(): Hardware reset SUCCESSS in restoring factory baudrate.");
  } 
  return 0; //assume success 
}

//If the baudrate to the BLE modue matches that baudrate that the BLE module expects,
//we should be able to ask its status and get a valid reply.  So, we don't get a valid
//reply, the baudrate of Serial1 probably does not match.
int checkStatusBLE(void) {
  int ret_val = ble.status(true);
  Serial.println("checkStatusBLE: reply code to status = " + String(ret_val));
  if (ret_val != BC127::SUCCESS) {
    Serial.println("checkStatusBLE: status() returned error: " + String(ret_val));
  }
  return ret_val;
}

// Echo what's coming in on USB Serial out to Bluetooth Serial
void echoIncomingUSBSerial(void) {
  while (Serial.available()) Serial1.write(Serial.read());
}

// Echo what's coming in on Bluetooth Serial out to USB Serial
void echoIncomingBTSerial(void) {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\r') c = '\n'; //translate carriage return to newline
    Serial.write(c);
  }
}


// ///////////////////////////////////////////////////  Arduino setup() and loop()

void setup() {
  //Serial.begin(115200);  //on Teensy (and Tympan), you don't need to do this.

  //tell the BLE class what firmware we think that is actually on the BC127
  ble.set_BC127_firmware_ver(FIRMWARE_VERSION);

  //Start Serial1 to the BT module
  Serial.println("setup(): Starting Serial1 at " + String(baudrate1));
  setSerial1BaudRate(baudrate1);

  //check on ble module...
  int ret_val = checkStatusBLE();
  if (ret_val != BC127::SUCCESS) {

    //try a different baud rate
    Serial.println("setup(): Trying again but at " + String(baudrate2));
    setSerial1BaudRate(baudrate2);
    cur_baudrate = baudrate2;

    //check on ble module again...
    ret_val = checkStatusBLE();
    if (ret_val == BC127::SUCCESS) {

      // we were successful at the new baud rate, so stick with it!
      Serial.println("Success! The module was at " + String(baudrate2));

    } else {
      //Failed.  So, return Serial1 to original baud rate
      Serial.println("Returning to Serial1 to original baud rate...this whole thing probably will not work");
      setSerial1BaudRate(baudrate1);
      cur_baudrate = baudrate1;
    }
  }

  //try forcing a hardware reset
  if (USE_HARDWARE_RESET) hardwareResetWithChecking();

  //switch to desired baudrate
  if (cur_baudrate != TARGET_BAUDRATE) {
    //let's try commanding the BC127 to the new baud rate anyway
    Serial.println("Change baudrate on the BC127 to "  + String(TARGET_BAUDRATE) + "...");
    char cmdBuffer[100];
    if (FIRMWARE_VERSION > 5)  {
      sprintf(cmdBuffer, "SET UART_CONFIG=%u OFF 0\rWRITE\rRESTORE\r", TARGET_BAUDRATE);  //restore? or do we want reset?
    } else  {
      sprintf(cmdBuffer, "SET BAUD=%u\rWRITE\rRESTORE\r", TARGET_BAUDRATE);  //restore?  or do we want reset?
    }
    Serial1.write(cmdBuffer);  delay(500);
    echoIncomingBTSerial();

    Serial.println("Switching Serial1 to new baud rate..." + String(TARGET_BAUDRATE));
    setSerial1BaudRate(TARGET_BAUDRATE);
    cur_baudrate = TARGET_BAUDRATE;
  }

  Serial.println("Begin echoing commands with BC127.  Try BC127 commands like STATUS or VERSION...(carriage return!)");
}

void loop() {
  echoIncomingUSBSerial();
  echoIncomingBTSerial();
  delay(5);
}
