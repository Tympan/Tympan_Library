#include "ble.h"

BLE::BLE(Stream *sp) : BC127(sp)
{
    restore();
    writeConfig();
    reset();
}

size_t BLE::sendByte(char c)
{
    String s = String("").concat(c);
    if (send(s))
        return 1;

    return 0;
}

size_t BLE::sendString(const String &s)
{
    if (send(s))
        return s.length();

    return 0;
}

size_t BLE::sendMessage(const String &s)
{
    size_t sentBytes = 0;
    int len = s.length();
    const int payloadLen = 19;

    String header;
    header = "\xab\xad\xc0\xde"; // ABADCODE, message preamble
    header.concat('\xff');       // message type

    // message length
    if (len >= 0x4000) {
        Serial.println("Message is too long!!! Aborting.");
        return 0;
    }
    int lenBytes = (len<<1) | 0x8001;
    header.concat((char)highByte(lenBytes));
    header.concat((char )lowByte(lenBytes));

    Serial.println("Header (" + String(header.length()) + " bytes): '" + header + "'");

    Serial.println("Message: '" + s + "'");

    char buf[8];

    //sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6));

    Serial.println(buf);
    int a = sendString(header);
    if (a != 7)
    {
        Serial.println("Error in sending header... Sent: '" + String(a) + "'");
    }

    int numPackets = ceil(s.length() / (float)payloadLen);

    for (int i = 0; i < numPackets; i++)
    {
        String bu = (char)(0xF8 | i);
        bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
        sentBytes += (sendString(bu)-1);
        delay(10);
    }

    if (s.length() == sentBytes)
    {
        return sentBytes;
    }

    return 0;
}

size_t BLE::recvMessage(String *s)
{
    int msgSize = 0;
    int bytesRecvd = 0;

    while (1)
    {
        if (available() > 0)
        {
            if (recvBLE(s) > 0)
            {
                if (s->startsWith("\xab\xad\xc0\xde\xff"))
                {
                    msgSize = word(s->charAt(5), s->charAt(6));
                    Serial.println("Length of message: '" + String(msgSize) + "'");

                    char buf[8];

                    sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", s->charAt(0), s->charAt(1), s->charAt(2), s->charAt(3), s->charAt(4), s->charAt(5), s->charAt(6));

                    Serial.println(buf);

                    int numPackets = ceil(msgSize / 20.0);

                    for (int i = 0; i < numPackets; i++)
                    {
                        bytesRecvd += recvBLE(s);
                    }

                    if (bytesRecvd == msgSize)
                    {
                        return bytesRecvd;
                    }
                }
                continue;
            }

            break;
        }
    }

    return 0;
}

size_t BLE::recvBLE(String *s)
{
    String tmp = String("");

    // get our start time
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + _timeout) > millis())
    {
        if (recv(&tmp) > 0)
        {
            if (tmp.startsWith("RECV BLE "))
            {
                s->concat(tmp.substring(9).trim());
                return tmp.substring(9).length();
            }

            tmp = "";
        }
    }

    return 0;
}

bool BLE::isConnected()
{
    if (status() > 0)
    {
        return getCmdResponse().startsWith("STATE CONNECTED");
    }

    return false;
}

bool BLE::waitConnect(int time)
{
    // some output has multiple lines
    String line = String("");

    // decide how long we're willing to wait
    int timeout = time > 0 ? time : _timeout;

    // get our start time
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + timeout) > millis())
    {
        if (_serialPort->available() > 0)
        {
            line.concat((char)_serialPort->read());
        }

        if (line.endsWith(EOL))
        {
            if (line.startsWith("OPEN_OK BLE"))
            {
                return true;
            }

            // move on to next line
            line = "";
        }
    }

    return false;
}