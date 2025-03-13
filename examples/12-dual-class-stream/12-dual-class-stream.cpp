// This example is similar to 11-dual-class, but allows a class like SensorA and SensorB
// to take a SC16IS7xx port or a hardware UART port. This requires that the port initialization
// such as baud rate be moved out of the class.

#include "SC16IS7xxRK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

// - Connect the SC16IS7xx by I2C
SC16IS7x2 extSerial;


// EXAMPLE == 1
// - Connect SC16IS7x2 TX Port A to RX Port B
// - Connect SC16IS7x2 RX Port A to TX Port B

// EXAMPLE == 2
// - Connect SC16IS7x2 TX Port A to Particle Device RX
// - Connect SC16IS7x2 RX Port A to Particle Device TX

#define EXAMPLE 2


// The SC16IS752 breakout that I used has this pinout. The order of your board may be different
// Breakout  I2C Mode  SPI Mode    Notes
// VCC       3V3       3V3
// GND       GND       GND
// RESET     3V3       3V3         Active low. Connect to 3V3 if not using, otherwise use pull-up.
// A0/CS     GND       D2          CS pin for SPI
// A1/SI     GND       MOSI
// NC/SO               MISO
// IRQ                             Open collector output
// I2C/SPI   3V3       GND         High = I2C, Low = SPI         
// SCL/SCLK  D1        SCK
// SDA/VSS   D0        GND

// If A0 and A1 are both connected to GND the address is 0x4D


class SensorA {
public:
    SensorA();
    virtual ~SensorA();

    SensorA &withPort(Stream *port) { this->port = port; return *this; };

    void setup();
    void loop();

protected:
    Stream *port = nullptr;
    unsigned long lastSendTime;
    char nextSendChar = ' ';
};

SensorA::SensorA() {
}

SensorA::~SensorA() {    
}

void SensorA::setup() {
}

void SensorA::loop() {
    if (millis() - lastSendTime > 2000) {
        lastSendTime = millis();

        port->write(nextSendChar++);
        if (nextSendChar >= 127)
        {
            nextSendChar = ' ';
        }
    }

    while (port->available())
    {
        int c = port->read();
        if (c >= ' ' && c < 127)
        {
            Log.info("SensorA received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("SensorA received 0x%2x", c);
        }
    }

}


class SensorB {
public:
    SensorB();
    virtual ~SensorB();

    SensorB &withPort(Stream *port) { this->port = port; return *this; };

    void setup();
    void loop();

protected:
    Stream *port = nullptr;
    unsigned long lastSendTime;
    char nextSendChar = ' ';
};

SensorB::SensorB() {
}

SensorB::~SensorB() {    
}

void SensorB::setup() {
}

void SensorB::loop() {
    if (millis() - lastSendTime > 2000) {
        lastSendTime = millis();

        port->write(nextSendChar++);
        if (nextSendChar >= 127)
        {
            nextSendChar = ' ';
        }
        
    }

    while (port->available())
    {
        int c = port->read();
        if (c >= ' ' && c < 127)
        {
            Log.info("SensorB received %c (0x%02x)", c, c);
        }
        else
        {
            Log.info("SensorB received 0x%2x", c);
        }
    }

}

SensorA sensorA;
SensorB sensorB;


void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    extSerial.withI2C(&Wire, 0x4d);
    Wire.setSpeed(CLOCK_SPEED_400KHZ);

    // This is specific to this test program, but you can do this from your
    // code for additional safety and debugging. It's a good way to check if you
    // have working SPI communication.
    extSerial.softwareReset();
    bool powerOnCheck = extSerial.powerOnCheck();
    Log.info("powerOnCheck=%d", (int)powerOnCheck);

    Log.info("Running example %d", EXAMPLE);

    extSerial.a().begin(9600);
    sensorA
        .withPort(&extSerial.a())
        .setup();

#if EXAMPLE == 2
    Serial1.begin(9600);
    sensorB
        .withPort(&Serial1)
        .setup();
#else
    extSerial.b().begin(9600);
    sensorB
        .withPort(&extSerial.b())
        .setup();
#endif
}

void loop()
{
    sensorA.loop();
    sensorB.loop();
}
