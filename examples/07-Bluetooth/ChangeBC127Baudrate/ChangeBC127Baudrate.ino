/*
*  ChangeBC127Baudrate
*
* Created: Ira Ray Jenkins, Creare, August 2021
* Purpose: Demonstrate changing the baudrate of the BC127 module.
*          The BC127 "RESTORE" command will reset the chip to default settings.
*
*/

#include <Arduino.h>

// USER PARAMETERS
#if 1
  //switch from slow (factory) to fast
  const int CURRENT_BAUD = 9600;  // how fast we are
  const int NEW_BAUD = 115200;    // how fast we want to be
#else
  //switch from fast back to slow (factory)
  const int CURRENT_BAUD = 115200;  // how fast we are
  const int NEW_BAUD = 9600;    // how fast we want to be
#endif
const int FIRMWARE_VERSION = 7; // what Melody firmware we are dealing with

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;

void echoIncomingUSBSerial(void)
{
    while (USB_Serial->available()) BT_Serial->write(USB_Serial->read());
}

void echoIncomingBTSerial(void)
{
    while (BT_Serial->available()) USB_Serial->write(BT_Serial->read());
}

void setup()
{
    Serial.begin(CURRENT_BAUD);  // Teensy Serial -> PC
    Serial1.begin(CURRENT_BAUD); // BC127 Serial -> Teensy

    delay(500);
    Serial.println("ChangeBC127Baudrate...");
    delay(500);
    
    char cmdBuffer[100];

    if (FIRMWARE_VERSION > 5)  {
        sprintf(cmdBuffer, "SET UART_CONFIG=%u OFF 0\rWRITE\rRESTORE\r", NEW_BAUD);
    } else  {
        sprintf(cmdBuffer, "SET BAUD=%u\rWRITE\rRESTORE\r", NEW_BAUD);
    }

    Serial.println("Change baudrate on the BC127...");
    Serial1.write(cmdBuffer);
    delay(500);
    echoIncomingBTSerial();

    Serial.println("Ending and flushing...");
    Serial.flush();
    Serial.end();
    Serial1.flush();
    Serial1.end();
    delay(500);

    Serial.println("Re-opening with new baudrates...(this should not appear?)");
    Serial.begin(NEW_BAUD);
    Serial1.begin(NEW_BAUD);
    Serial.println("Openned with new baudrates...");
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}
