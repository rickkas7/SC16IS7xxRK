#include "SC16IS7xxRK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

SC16IS7x0 extSerial;

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    Serial1.begin(9600);

    extSerial.withSPI(&SPI, D2, 8); // SPI port, CS line, speed in MHz
    // extSerial.withI2C(&Wire, 0);

    extSerial.begin(9600);
}

void loop()
{
    if (Serial.available())
    {
        int c = Serial.read();
        Serial1.write(c);

        if (c >= ' ' && c < 127)
        {
            Log.info("usb received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("usb received 0x%2x", c);
        }
    }
    if (Serial1.available())
    {
        int c = Serial1.read();
        Serial.write(c);

        if (c >= ' ' && c < 127)
        {
            Log.info("Serial1 received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("Serial received 0x%2x", c);
        }
    }

    while (extSerial.available())
    {
        int c = extSerial.read();
        if (c >= ' ' && c < 127)
        {
            Log.info("ext received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("ext received 0x%2x", c);
        }

        extSerial.print(c);
    }
}
