#include "SC16IS7xxRK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

// - Connect the SC16IS7xx by SPI
// - Use D2 as the SPI CS
// - Connect SC16IS7xx TX to Particle RX
// - Connect SC16IS7xx RX to Particle TX
SC16IS7x0 extSerial;

// Uncomment this to use SPI and set to the CS pin
// #define USE_SPI_CS D2

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    Serial1.begin(9600);

#ifdef USE_SPI_CS
    extSerial.withSPI(&SPI, USE_SPI_CS, 8); // SPI port, CS line, speed in MHz
#else
    extSerial.withI2C(&Wire, 0);
    Wire.setSpeed(CLOCK_SPEED_400KHZ);
#endif

    // This is specific to this test program, but you can do this from your
    // code for additional safety and debugging. It's a good way to check if you
    // have working SPI communication.
    extSerial.softwareReset();
    extSerial.powerOnCheck();

    // Some of the examples chain begin() right after withSPI() which is fine
    // as long as you are not doing a softwareReset.
    extSerial.begin(9600);
}

void loop()
{
    static unsigned long lastSendTime1 = 0;
    static char nextSendChar1 = ' ';
    if (millis() - lastSendTime1 > 2000)
    {
        lastSendTime1 = millis();

        Serial1.write(nextSendChar1++);
        if (nextSendChar1 >= 127)
        {
            nextSendChar1 = ' ';
        }
    }

    static unsigned long lastSendTime2 = 0;
    static char nextSendChar2 = ' ';
    if (millis() - lastSendTime2 > 2000)
    {
        lastSendTime2 = millis();

        extSerial.write(nextSendChar2++);
        if (nextSendChar2 >= 127)
        {
            nextSendChar2 = ' ';
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
}
