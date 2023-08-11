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
SC16IS7x0 extSerial;

// Uncomment this to use SPI and set to the CS pin
#define USE_SPI_CS D4

// Maximum supported baud rate is typically 38400.
int baudRate = 38400;

Thread *sendingThread;
size_t writeIndex = 0;
size_t readIndex = 0;
unsigned long readStart = 0;
bool valueWarned = false;

void sendingThreadFunction(void *param);

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    Serial1.begin(baudRate);

#ifdef USE_SPI_CS
    extSerial.withSPI(&SPI, USE_SPI_CS, 4); // SPI port, CS line, speed in MHz
#else
    extSerial.withI2C(&Wire, 0);
    Wire.setSpeed(CLOCK_SPEED_400KHZ);
#endif

    extSerial.softwareReset();
    extSerial.powerOnCheck();

    extSerial.begin(baudRate);

    // The parameter is the buffer size
    extSerial.withBufferedRead(20000);



}

void loop()
{
    if (!sendingThread && Particle.connected()) {
        // Start sending thread
        sendingThread = new Thread("sending", sendingThreadFunction, (void *)nullptr, OS_THREAD_PRIORITY_DEFAULT, 512);
        Log.info("starting test!");
    }

    uint8_t readBuf[64];
    
    int count = extSerial.read(readBuf, sizeof(readBuf));
    if (count > 0) {
        if (readStart == 0) {
            readStart = millis();
        }
        for(int ii = 0; ii < count; ii++, readIndex++) {
            if (readBuf[ii] != (readIndex & 0xff)) {
                if (!valueWarned) {
                    Log.dump(&readBuf[ii], count - ii);
                    Log.error("value mismatch readIndex=%u got=0x%02x expected=0x%02x count=%d ii=%d", readIndex, readBuf[ii], readIndex & 0xff, count, ii);
                    valueWarned = true;
                }
            }
            if ((readIndex % 10000) == 0) {
                if (!valueWarned) {
                    Log.trace("readIndex=%u writeIndex=%u", readIndex, writeIndex);

                    if (readIndex > 0) {
                        double elapsed = (millis() - readStart) / 1000;
                        double kbytesSec = (double)readIndex / elapsed / 1024;
                        Log.info("%.1lf Kbytes/sec, %.1f sec, %ld Kbytes", kbytesSec, elapsed, readIndex / 1024);
                    }

                }

                // This delay would cause data loss if not buffering
                delay(500);
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
