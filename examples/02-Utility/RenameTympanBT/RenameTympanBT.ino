/*
*   RenameTympanBT
*
*   Created: Chip Audette, OpenAudio, Mar 2018
*   Purpose: Change the name of the Bluetooth module on the Tympan Rev C to make it 
*       easier for us fellow humans to recognize when connecting via PC, phone, or
*       other device.
*       
*   Assumptions:  This code assumes that you already know how to reprogram your Tympan
*       Using any of the example programs that came with the Tympan_Library.  If so,
*       continue.
*       
*   How to Use:
*       1) Plug your Tympan into your PC via USB, like you're going to reprogram it
*          from here within the Arduino IDE
*       2) Turn on your Tympan
*       3) Open the Serial Monitor (you might first need to go Tools->Port to choose your Teensy)
*       4) Compile and Upload this sketch
*       5) Once the code uploads to the Tympan, watch the text on the Serial Monitor.
*       6) You should see three responses from the BT module: "CMD", "AOK", and "Reboot!"
*
*   Uses Tympan Rev C Audio Adapter.
*
*   Uses AT Command set for the RN42 Bluetooth module.  The AT Command set documentation
*   can be found here: https://cdn.sparkfun.com/datasheets/Wireless/Bluetooth/bluetooth_cr_UG-v1.0r.pdf
*
*   MIT License.  use at your own risk.
*/


char new_BT_name[] = "MyTympan";   //choose whatever name you would like.  Max 20 characters.

#define BT_SERIAL Serial1  // On the Tympan Rev C, this is how the BT is connected to the Teensy

void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  BT_SERIAL.begin(115200); 
  delay(2000);  //anything shorter than 1500 seems to fail
  Serial.print("Changing Bluetooth module name to: "); Serial.println(new_BT_name);

  BT_SERIAL.print("$$$");delay(200);echoIncomingBTSerial();  //should respond "CMD"
  BT_SERIAL.print("SN,");BT_SERIAL.println(new_BT_name); delay(200);echoIncomingBTSerial();  //should response "AOK"
  BT_SERIAL.println("R,1");delay(200);echoIncomingBTSerial();  //should respond "Reboot!"

  Serial.println("Bluetooth renaming complete.");
  Serial.println("");
  Serial.println("If you saw 'CMD', 'AOK', and 'Reboot!', the renaming was successful.");
} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  echoIncomingSerial();     //echo messages from USB Serial over to BT Serial
  echoIncomingBTSerial();  //echo messages from BT serial over to USB Serial

} //end loop();

void echoIncomingSerial(void) {
    while (Serial.available()) BT_SERIAL.write(Serial.read()); //echo messages from USB Serial over to BT Serial
}

void echoIncomingBTSerial(void) {
  while (BT_SERIAL.available()) Serial.write(BT_SERIAL.read());//echo messages from BT serial over to USB Serial
}

