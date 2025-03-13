#include "SC16IS7xxRK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

// - Connect the SC16IS7xx by SPI
// - Use D2 as the SPI CS
// - Connect SC16IS7x2 TX Port A to RX Port B
// - Connect SC16IS7x2 RX Port A to TX Port B
SC16IS7x2 extSerial;

// Uncomment this to use SPI and set to the CS pin
// #define USE_SPI_CS D4

// The SC16IS752 breakout that I used has this pinout. The order of your board may be different
// Breakout  I2C Mode  SPI Mode    Notes
// VCC       3V3       3V3
// GND       GND       GND
// RESET                           Active low. Connect to 3V3 if not using, otherwise use pull-up.
// A0/CS     GND       D2          CS pin for SPI
// A1/SI     GND       MOSI
// NC/SO     
// IRQ                             Open collector output
// I2C/SPI   3V3       GND         
// SCL/SCLK  D1        SCK
// SDA/VSS   D0        GND

// If A0 and A1 are both connected to GND the address is 0x4D with the SC16IS752


void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

#ifdef USE_SPI_CS
    extSerial.withSPI(&SPI, USE_SPI_CS, 4); // SPI port, CS line, speed in MHz
#else
    extSerial.withI2C(&Wire, 0x4d);
    Wire.setSpeed(CLOCK_SPEED_400KHZ);
#endif

    // This is specific to this test program, but you can do this from your
    // code for additional safety and debugging. It's a good way to check if you
    // have working SPI communication.
    extSerial.softwareReset();
    extSerial.powerOnCheck();

    // Since this is a dual UART device you must begin both ports, which can
    // be different baud rates
    extSerial.a().begin(9600);
    extSerial.b().begin(9600);
}

void loop()
{
    static unsigned long lastSendTime1 = 0;
    static char nextSendChar1 = ' ';
    if (millis() - lastSendTime1 > 2000)
    {
        lastSendTime1 = millis();

        extSerial.b().write(nextSendChar1++);
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

        extSerial.a().write(nextSendChar2++);
        if (nextSendChar2 >= 127)
        {
            nextSendChar2 = ' ';
        }
    }

    while (extSerial.a().available())
    {
        int c = extSerial.a().read();
        if (c >= ' ' && c < 127)
        {
            Log.info("ext a received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("ext a received 0x%2x", c);
        }
    }

    while (extSerial.b().available())
    {
        int c = extSerial.b().read();
        if (c >= ' ' && c < 127)
        {
            Log.info("ext b received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("ext b received 0x%2x", c);
        }
    }

}
