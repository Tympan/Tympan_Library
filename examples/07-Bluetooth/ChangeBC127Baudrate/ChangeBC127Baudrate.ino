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

// USER PARAMETERS
const int TARGET_BAUDRATE = 115200;    //try 115200 (fast!) or 9600 (factory original)

// Automatically choose which Tympan and which firmware (you can override!)
#ifdef KINETISK  //this is set by the Arduino IDE when you choose Teensy 3.6
  //REV D...does not work reliably with RevD.  I don't know why
  Tympan myTympan(TympanRev::D);      //sets all the control pins correctly
  const int FIRMWARE_VERSION = 5;     // what Melody firmware we are dealing with
#else
  //REV E
  Tympan myTympan(TympanRev::E);       //sets all the control pins correctly
  const int FIRMWARE_VERSION = 7;      // what Melody firmware we are dealing with
#endif

//define the pins that are used for the hardware reset
const int pin_PIO0 = myTympan.getTympanPins().BT_PIO0;   //RevD = 56, RevE = 5   // Pin # for connection to BC127 PIO0 pin
const int pin_RST = myTympan.getTympanPins().BT_nReset;  //RevD = 34, RevE = 9   // Pin # for connection to BC127 RST pin
//const int pin_VREGEN = myTympan.getTympanPins().BT_REGEN; 

const int baudrate_factory = 9600;       // here is the factory default

// //////////////////////////////////////////// Helper functions


void setSerial1BaudRate(int new_baud) {
  Serial1.flush();  Serial1.end();   Serial1.begin(new_baud);
}

//This function attempts to hold PIO 0 high and then reseting via the reset pin.
//See Melody Guide for v7 "Restoring the Default Configuration".
void hardwareReset() {
  
  //Serial.println("HardwareReset: RST Pin = " + String(pin_RST) + ", PIO0 Pin = " + String(pin_PIO0));
  
  pinMode(pin_PIO0,OUTPUT);    //prepare the PIO0 pin
  digitalWrite(pin_PIO0,HIGH); //normally low.  Swtich high

  pinMode(pin_RST,OUTPUT);    //prepare the reset pin
  digitalWrite(pin_RST,LOW);  //pull low to reset
  delay(20);                  //hold low for at least 5 msec
  digitalWrite(pin_RST,HIGH); //pull high to start the boot

  //wait for boot to proceed far enough before changing anything
  delay(400);                 //V7: works with 200 but not 175.  V5: works with 350 but not 300

  digitalWrite(pin_PIO0,LOW); //pull PIO0 back down low
  pinMode(pin_PIO0,INPUT);    //go high-impedance to make irrelevant

}

void changeBaudRateBC127(int target_rate, int firmware_ver) {
  //Serial.println("setup: Firmware V" + String(firmware_ver) + ": Change baudrate on the BC127 to "  + String(target_rate) + "...");
  if (firmware_ver > 5)  {  
    //On version 7.3, the baud rate shift is instant.
    Serial1.print("SET UART_CONFIG=");Serial1.print(target_rate);Serial1.print(" OFF 0");Serial1.print('\r');
  } else  {
    //On version 5, the baud rate shift is instant.
    Serial1.print("SET BAUD=");Serial1.print(target_rate);Serial1.print('\r');
  }
}

// Echo what's coming in on USB Serial out to Bluetooth Serial
void echoIncomingUSBSerial(void) {
    while (Serial.available())
    {
      char c = Serial.read();
      if (c == '|') {
          Serial.println("Switching Serial1 to 115200...");
          setSerial1BaudRate(115200);
      } else if (c =='\\') { 
          Serial.println("Switching Serial1 to 9600...");
          setSerial1BaudRate(9600);
      } else if (c == ']') {
          Serial.println("Hardware reset...");
          hardwareReset();
      } else {
          Serial1.write(c); //send to Bluetooth
      }
    }
}

// Echo what's coming in on Bluetooth Serial out to USB Serial
void echoIncomingBTSerial(void) {
  static char prev_char;
  while (Serial1.available()) {
    char orig_c = Serial1.read();
    char c = orig_c;
    if ((c == '\n') && (prev_char == '\r')) {
      //don't send
    } else if ((c == '\r') && (prev_char == '\n')) {
      //don't send
    } else {
      //send the character (perhaps modified)
      if (c == '\r') c = '\n'; //translate carriage return to newline
      Serial.write(c);
    }
    prev_char = orig_c;
  }
}


// ///////////////////////////////////////////////////  Arduino setup() and loop()

void setup() {

  //////////////////////////////////////////////// Factory reset to known baud rate (9600)

  Serial.println("setup(): Setting Teensy Serial1 at " + String(baudrate_factory));
  setSerial1BaudRate(baudrate_factory);

  //clear out buffers
  //while (Serial.available()) Serial.read();
  //while (Serial1.available()) Serial1.read();

  //Force a hardware reset...which should drop baudrate to factory setting
  Serial.println("setup(): hardware reset...");
  hardwareReset();  
  delay(2000);            //delay of 1250 seems to work for V5 but 1000 occasionally fails.  1250 failed once.
  echoIncomingBTSerial(); //should get Melody ID text (end with READY or OK)
  Serial.println();

  ///////////////////////////////////////////////// Change baud rate on the BC127

  //Switch the baudrate on the BC127
  Serial.print("setup(): commanding BC127 to change baud rate to "); Serial.println(TARGET_BAUDRATE);
  changeBaudRateBC127(TARGET_BAUDRATE, FIRMWARE_VERSION);
 
  //Switch Teensy serial to the matching speed
  setSerial1BaudRate(TARGET_BAUDRATE);
  Serial.println("setup(): switching Teensy Serial1 to new baud rate " + String(TARGET_BAUDRATE));

  //Listen for response to the change in baud rate (or, at least, clear out the serial
  delay(500);              //500 seems to work on V5
  echoIncomingBTSerial();  //clear out the serial

  //send carriage return to induce "ERROR" message to clear out the serial link both ways
  Serial.println("setup(): issuing Carriage Return to clear serial...should return ERROR...");
  Serial1.print('\r');  //should cause response of "ERROR"
  delay(100);  echoIncomingBTSerial();  //should get error

  //send WRITE to save the baud rate and send another writes just to clear up the Serial link
  Serial.println("setup(): issuing WRITE command to save the new rate");
  Serial1.print("WRITE");Serial1.print('\r');
  delay(200);  echoIncomingBTSerial();  //should get OK
  
  //Serial.println("setup(): issuing second WRITE command to save the new rate to ensure we're good");
  //Serial1.print("WRITE");Serial1.print('\r');
  //delay(200);  echoIncomingBTSerial();  //should get OK
  
  ///////////////////////////////////////////////// Check to make sure that we're good

  Serial.println("setup(): Asking baud rate of BC127...");
  if (FIRMWARE_VERSION > 6) {
    Serial1.print("GET UART_CONFIG");Serial1.print('\r');
  } else {
    Serial1.print("GET BAUD");Serial1.print('\r');
  }
  delay(200); echoIncomingBTSerial();  //should give the value of the new baud rate

  Serial.println();
  Serial.println("We are done!  Now echoing commands with the BC127...");
  Serial.println();
  Serial.println("To change BC127 baud rate: for V5, send: SET BAUD=115200   for V7: send: SET UART_CONFIG=115200 OFF 0");
  Serial.println("To change Tympan baud rate, send '|' for 115200 or send '\\' for 9600");
  Serial.println("To force a hardware reset of the BC127, send ']'");
  Serial.println();
  Serial.println("Try BC127 commands like STATUS or VERSION or ADVERTISING ON...(with carriage return!)");
  Serial.println();  
}

void loop() {
  echoIncomingUSBSerial();
  echoIncomingBTSerial();
  delay(5);
}
