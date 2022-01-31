/*
	Example code that demonstrates the use of the BC127 Library.

	Created: Ira Ray Jenkins, Creare
	License: MIT License.  Use at your own risk.
*/

#include <Arduino.h> // has serial classes

#include <Tympan_Library.h> // has bc127 class

usb_serial_class *USB_Serial = &Serial;
HardwareSerial *BT_Serial = &Serial1;
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
    USB_Serial->begin(9600);
    BT_Serial->begin(9600);
    delay(2000);

    USB_Serial->println("*** Setup Complete.");

    BC127Test();
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}

void BC127Test()
{

    BC127 bc127 = BC127(&Serial1);
    BC127::opResult res;

    // get version
    USB_Serial->println("*** BC127 Version");
    res = bc127.version();

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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Configuration");
    res = bc127.getConfig();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** RSSI_THRESH Config");
    res = bc127.getConfig("RSSI_THRESH");
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    // restore factory defaults
    USB_Serial->println("*** Restoring factory defaults...");
    res = bc127.restore();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Writing config...");
    res = bc127.writeConfig();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Reset device...");
    res = bc127.reset();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Power cycle the device...");
    res = bc127.power(false);
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Power off...");

    res = bc127.power();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Power on...");

    USB_Serial->println("*** Enable ANDROID bc127...");
    res = bc127.setConfig("ENABLE_ANDROID_BLE", "ON");
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    // set advertising
    USB_Serial->println("*** Set advertising on...");
    res = bc127.advertise();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Waiting for connection...");
    while (1)
    {
        if (BT_Serial->available())
        {
            readBTResponse();
            USB_Serial->print("*** Response = ");
            USB_Serial->println(response);
            response.trim();

            if (response.equalsIgnoreCase("OPEN_OK BLE"))
            {
                USB_Serial->println("*** Connected.");
                break;
            }
            delay(2000);
        }
    }

    USB_Serial->println("*** BLE Connected.");

    USB_Serial->println("*** Get status...");
    res = bc127.status();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Send hello...");
    res = bc127.send("hello");
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }

    USB_Serial->println("*** Close connection...");
    res = bc127.close();
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
        USB_Serial->println("SUCCESS!\n" + bc127.getCmdResponse());
    }
}