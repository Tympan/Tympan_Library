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
const int CURRENT_BAUD = 9600;  // how fast we are
const int NEW_BAUD = 115200;    // how fast we want to be
const int FIRMWARE_VERSION = 7; // what Melody firmware we are dealing with

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;

void echoIncomingUSBSerial(void)
{
    while (USB_Serial->available())
    {
        BT_Serial->write(USB_Serial->read());
    }
}

void echoIncomingBTSerial(void)
{
    while (BT_Serial->available())
    {
        USB_Serial->write(BT_Serial->read());
    }
}

void setup()
{
    Serial.begin(CURRENT_BAUD);  // Teensy Serial -> PC
    Serial1.begin(CURRENT_BAUD); // BC127 Serial -> Teensy
    char cmdBuffer[100];

    if (FIRMWARE_VERSION > 5)
    {
        sprintf(cmdBuffer, "SET UART_CONFIG=%u OFF 0\rWRITE\rRESTORE\r", NEW_BAUD);
    }
    else
    {
        sprintf(cmdBuffer, "SET BAUD=%u\rWRITE\rRESTORE\r", NEW_BAUD);
    }

    Serial1.write(cmdBuffer);
    Serial.flush();
    Serial.end();
    Serial1.flush();
    Serial1.end();

    Serial.begin(NEW_BAUD);
    Serial1.begin(NEW_BAUD);
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}