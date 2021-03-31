#include <Arduino.h>

#include <Tympan_Library.h>

usb_serial_class *USB_Serial = &Serial;
HardwareSerial *BT_Serial = &Serial1;
String response;

// hello
String hello = String("hello");

// 127 bytes
String maxString = String("iRUlJbyP5uZPVhlFNP8PaHUAy282o4JLVSRUIOagoleWLnkUe8m37FOYm1twjM5aTRNMbCf3AoQVeSeeFiXBcRUQQ8r05ggNadovUk3p3a2Zw8cycwYYIWn8abcdefg");

// lots of bytes?
String gettysburg = String("Fourscore and seven years ago our fathers brought forth on this continent a new nation, conceived in liberty and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battlefield of that war. We have come to dedicate a portion of that field as a final resting-place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we cannot dedicate — we cannot consecrate — we cannot hallow — this ground. The brave men, living and dead, who struggled here have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us — that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion — that we here highly resolve that these dead shall not have died in vain — that this nation shall have a new birth of freedom and that government of the people, by the people, for the people, shall not perish from the earth.");

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

    BLETest();

    USB_Serial->println("*** Test Complete... Entering console mode.");
}

void loop()
{
    echoIncomingUSBSerial();
    echoIncomingBTSerial();
}

void BLETest()
{
    BLE ble = BLE(&Serial1);

    if (ble.isConnected())
    {
        USB_Serial->println("*** Connected...");
    }
    else
    {
        USB_Serial->println("*** Not Connected...");
    }

    USB_Serial->println("*** Advertising on...");
    ble.advertise(true);

    if (!ble.waitConnect(60000))
    {
        USB_Serial->println("*** Timed out on Connect.");
    }
    else
    {
        USB_Serial->println("*** Connected...");
    }

    if (ble.isConnected())
    {
        USB_Serial->println("*** Is Connected...");
    }
    else
    {
        USB_Serial->println("*** Is Not Connected...");
    }

    delay(5000);

    size_t sentBytes = 0;

    USB_Serial->println("*** Sending 'h'...");
    sentBytes = ble.sendByte('h');
    if (sentBytes > 0)
    {
        USB_Serial->println("*** Sent " + String(sentBytes) + " byte...");
    }
    else
    {
        USB_Serial->println("*** Failed.");
    }

    USB_Serial->println("*** Sending 'Hello World'");
    sentBytes = ble.sendString("Hello World");
    if (sentBytes > 0)
    {
        USB_Serial->println("*** Sent " + String(sentBytes) + " bytes...");
    }
    else
    {
        USB_Serial->println("*** Failed.");
    }

    USB_Serial->println("*** Sending 127 bytes...");
    sentBytes = ble.sendString(maxString);
    if (sentBytes > 0)
    {
        USB_Serial->println("*** Sent " + String(sentBytes) + " bytes...");
    }
    else
    {
        USB_Serial->println("*** Failed.");
    }

    USB_Serial->println("*** Sending Gettysburg Address...");
    sentBytes = ble.sendString(gettysburg);
    if (sentBytes > 0)
    {
        USB_Serial->println("*** Sent " + String(sentBytes) + " bytes...");
    }
    else
    {
        USB_Serial->println("*** Failed.");
    }

    USB_Serial->println("*** Sending Gettysburg as Message...");
    sentBytes = ble.sendMessage(gettysburg);
    //sentBytes = ble.sendMessage(hello);
    if (sentBytes > 0)
    {
        USB_Serial->println("*** Sent " + String(sentBytes) + " bytes...");
    }
    else
    {
        USB_Serial->println("*** Failed.");
    }

    delay(5000);
    String msg = String("");
    USB_Serial->println("*** Receiving string over BLE...");

    while (1)
    {
        if (ble.available() > 0)
        {
            if (ble.recvBLE(&msg) > 0)
            {
                USB_Serial->println("Received: '" + msg + "'");
            }
            else
            {
                USB_Serial->println("No message received...");
            }

            break;
        }
    }

    msg = "";

    // does not work atm
    //    USB_Serial->println("*** Receiving message over BLE...");
    //    ble.recvMessage(&msg);
}
