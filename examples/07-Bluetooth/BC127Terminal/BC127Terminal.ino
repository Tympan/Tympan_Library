/*
*  BC127Terminal
*
* Created: Ira Ray Jenkins, Creare, December 2020
* Purpose: Demonstrates a serial terminal with the BC127 module.
*
*/

//The BC127 ends its lines with a carriage return, not a newline.  The Arduino Serial Monitor doesn't like this.
#define TRANSLATE_CR_FOR_ARDUINO_SERIALMONITOR true   //set to true to convert in-coming carriage returns into newlines

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
      char c = BT_Serial->read();
      if (TRANSLATE_CR_FOR_ARDUINO_SERIALMONITOR) {
          if (c == '\r') c = '\n'; //replace carriage return with newline to make pretty in Arduino Serial Monitor
      }
      USB_Serial->write(c);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600);
    delay(1000);
    
    USB_Serial = &Serial;
    BT_Serial = &Serial1;
 
    Serial.println("Type commands to send to the BC127");
    Serial.println("Try HELP, or try STATUS, or try ADVERTISING ON...");
    Serial.println("Be sure to use carriage return, not newline...");
    Serial.println();
    Serial.println("Sending VERSION command...");
    Serial.println();   
    BT_Serial->print("VERSION");BT_Serial->print('\r'); //terminate all commands with carriage return
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}
