#include "SC16IS7xxRK.h"

// Pick a debug level from one of these two, however, TRACE level is basically
// unusable due to the large number of log messages
SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);


// - Connect the SC16IS7xx by SPI
// - Use D2 as the SPI CS
// - Connect SC16IS7xx TX to Particle RX
// - Connect SC16IS7xx RX to Particle TX
// - Connect SC16IS7xx /CTS (input) to Particle D2 RTS (output)
// - Connect SC16IS7xx /RTS (output) to Particle D3 CTS (input)
SC16IS7x0 extSerial;

// Uncomment this to use SPI and set to the CS pin
#define USE_SPI_CS D4

int baudRate = 38400;

Thread *sendingThread;
size_t writeIndex = 0;
size_t readIndex = 0;
bool valueWarned = false;

void sendingThreadFunction(void *param);

#if !defined(HAL_PLATFORM_NRF52840)
#error This example only runs on nRF52 devices as RTL872x devices don't support hardware flow control on Serial1
#endif

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    // Serial1 D3(CTS) Input. When HW flow control is enabled, must be low for this side to send data.
    // Serial1 D2(RTS) Output. LOW indicates that this side can receive data (buffer not full).
    Serial1.begin(baudRate, SERIAL_FLOW_CONTROL_RTS_CTS);

#ifdef USE_SPI_CS
    extSerial.withSPI(&SPI, USE_SPI_CS, 4); // SPI port, CS line, speed in MHz
#else
    extSerial.withI2C(&Wire, 0);
    Wire.setSpeed(CLOCK_SPEED_400KHZ);
#endif

    extSerial.softwareReset();
    extSerial.powerOnCheck();

    extSerial.begin(baudRate, SC16IS7xxPort::OPTIONS_FLOW_CONTROL_RTS_CTS);


}

void loop()
{
    if (!sendingThread && Particle.connected()) {
        // Start sending thread
        sendingThread = new Thread("sending", sendingThreadFunction, (void *)nullptr, OS_THREAD_PRIORITY_DEFAULT, 512);        
    }

    uint8_t readBuf[64];
    
    int count = extSerial.read(readBuf, sizeof(readBuf));
    if (count > 0) {
        for(int ii = 0; ii < count; ii++, readIndex++) {
            if (readBuf[ii] != (readIndex & 0xff)) {
                if (!valueWarned) {
                    valueWarned = true;
                    Log.error("value mismatch readIndex=%u got=0x%02x expected=0x%02x count=%d ii=%d", readIndex, readBuf[ii], readIndex & 0xff, count, ii);
                }
            }
            if ((readIndex % 10000) == 0) {
                if (!valueWarned) {
                    Log.info("readIndex=%u writeIndex=%u", readIndex, writeIndex);
                }
            }
        }
    }
}

void sendingThreadFunction(void *param) 
{
    while(true) {
        while(Serial1.availableForWrite() > 10) {
            Serial1.write(writeIndex++ & 0xff);
        }
        delay(1);
    }
}
