/*
*	EchoBLE
*
*	Created: Ira Ray Jenkins, Creare, July 2020
*	Purpose: Demonstrates BLE functionality of the Tympan BC127 Bluetooth module. Data
		received via BLE will be echo'd back.
*
*/

#include <Arduino.h>

#include <Tympan_Library.h>

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;
String response;

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

void readBTResponse(void)
{
    response = BT_Serial->readString();
}

void setup()
{
    //  myTympan.beginBothSerial();
    Serial.begin(9600);
    Serial1.begin(9600);
    delay(2000);
    USB_Serial = &Serial;
    BT_Serial = &Serial1;
    USB_Serial->println("*** Setup Starting...");

    BLE_Setup();

    USB_Serial->println("*** Setup Complete.");
}

void loop()
{
    /*
 while(BT_Serial->available()) {
    response = BT_Serial->readString();

    USB_Serial->println("Response: " + response.trim());

    if(response.substring(0,8) == "RECV BLE") {
      USB_Serial->println("*** Received: " + response.substring(9));

      BT_Serial->print("SEND Received: " + response.substring(9) + "\r");

      delay(200);

      BT_Serial->readString();

    }
  }
  */

    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}

void BLE_Setup(void)
{
    BC127 ble = BC127(&Serial1);

    BC127::opResult res;

    Serial1.print("$");
    delay(400);
    Serial1.print("$$$");
    delay(2400);

    // get version
    USB_Serial->println("*** BC127 Version");
    res = ble.version();

    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("Module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    USB_Serial->println("*** Configuration");
    res = ble.getConfig();
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    USB_Serial->println("*** RSSI_THRESH Config");
    res = ble.getConfig("RSSI_THRESH");
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    // restore factory defaults
    USB_Serial->println("*** Restoring factory defaults...");
    res = ble.restore();
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    USB_Serial->println("*** Writing config...");
    res = ble.writeConfig();
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    USB_Serial->println("*** Reset device...");
    res = ble.reset();
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    USB_Serial->println("*** Enable ANDROID BLE...");
    res = ble.setConfig("ENABLE_ANDROID_BLE", "ON");
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    // set advertising
    USB_Serial->println("*** Set advertising on...");
    res = ble.advertise();
    if (res == BC127::TIMEOUT_ERROR)
    {
        USB_Serial->println("timeout error");
    }
    else if (res == BC127::MODULE_ERROR)
    {
        USB_Serial->println("module error");
    }
    else
    {
        USB_Serial->println("SUCCESS!\n" + ble.getCmdResponse());
    }

    while (1)
    {
        if (BT_Serial->available())
        {
            readBTResponse();
            USB_Serial->print("*** Response = ");
            USB_Serial->println(response);
            response.trim();

            if (response.equalsIgnoreCase("OPEN_OK BLE"))
            //if (response.indexOf("OPEN_OK BLE") > 0)
            {
                USB_Serial->println("*** Connected.");
                break;
            }
            delay(2000);
        }
    }

    USB_Serial->println("*** BLE Connected.");

    if (!ble.stdCmd("HELP"))
    {
        USB_Serial->println("error");
    }
    else
    {
        USB_Serial->println(ble.getCmdResponse());
    }
}
