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

int baudRate = 38400;

uint8_t tempBuf[16384];
Thread *sendingThread;
size_t writeIndex = 0;
size_t readIndex = 0;

void sendingThreadFunction(void *param);

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    Serial1.begin(baudRate);

    extSerial.withSPI(&SPI, D2, 8); // SPI port, CS line, speed in MHz
    // extSerial.withI2C(&Wire, 0);

    extSerial.softwareReset();
    extSerial.powerOnCheck();

    extSerial.begin(baudRate);

    // Random test data
    // tempBuf is currently 16384 bytes 
    srand(0);
    for (size_t ii = 0; ii < sizeof(tempBuf); ii++)
    {
        tempBuf[ii] = rand();
    }
    
    // Start sending thread
    sendingThread = new Thread("sending", sendingThreadFunction, (void *)nullptr, OS_THREAD_PRIORITY_DEFAULT, 512);        


}

void loop()
{
    uint8_t readBuf[64];
    
    int count = extSerial.read(readBuf, sizeof(readBuf));
    if (count > 0) {
        for(int ii = 0; ii < count; ii++, readIndex++) {
            if (readBuf[ii] != tempBuf[readIndex % sizeof(tempBuf)]) {
                Log.error("value mismatch readIndex=%u got=0x%02x expected=0x%02x count=%d ii=%d", readIndex, readBuf[ii], tempBuf[readIndex % sizeof(tempBuf)], count, ii);
            }
            if ((readIndex % 10000) == 0) {
                Log.info("readIndex=%u writeIndex=%u", readIndex, writeIndex);
            }
        }
    }
}

void sendingThreadFunction(void *param) 
{
    while(true) {
        while(Serial1.availableForWrite() > 10) {
            Serial1.write(tempBuf[writeIndex++ % sizeof(tempBuf)]);
        }
        delay(1);
    }
}
