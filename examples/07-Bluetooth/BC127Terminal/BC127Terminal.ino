/*
*	BC127Terminal
*
*	Created: Ira Ray Jenkins, Creare, December 2020
*	Purpose: Demonstrates a serial terminal with the BC127 module.
*
*/

#include <Arduino.h>

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
    //  myTympan.beginBothSerial();
    Serial.begin(9600);
    Serial1.begin(9600);
    USB_Serial = &Serial;
    BT_Serial = &Serial1;
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}
