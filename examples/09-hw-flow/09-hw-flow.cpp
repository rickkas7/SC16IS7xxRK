#include "SC16IS7xxRK.h"

// Pick a debug level from one of these two, however, TRACE level is basically
// unusable due to the large number of log messages
SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);


// - Connect the SC16IS7xx by SPI
// - Use D2 as the SPI CS
// - Connect SC16IS7xx TX to Particle RX (Channel 0)
// - Connect Particle TX to SC16IS7xx RX (Channel 1)
// - Connect SC16IS7xx /CTS (input) to Particle D2 RTS (output) (channel 3)
// - Connect SC16IS7xx /RTS (output) to Particle D3 CTS (input) (channel 2)
SC16IS7x0 extSerial;

// Uncomment this to use SPI and set to the CS pin
#define USE_SPI_CS D4

int baudRate = 38400;

Thread *sendingThread;
size_t writeIndex1 = 0;
size_t readIndex1 = 0;
size_t writeIndex2 = 0;
size_t readIndex2 = 0;
bool valueWarned1 = false;
bool valueWarned2 = false;

void sendingThreadFunction(void *param);

#if !defined(HAL_PLATFORM_NRF52840)
#error "This example only runs on nRF52 devices as RTL872x devices do not support hardware flow control on Serial1"
#endif

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);
    delay(2000);

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
        sendingThread = new Thread("sending", sendingThreadFunction, (void *)nullptr, OS_THREAD_PRIORITY_DEFAULT, 2048);        
        Log.info("starting test 09-hw-flow-control!");
    }

    uint8_t readBuf[64];
    
    int count = extSerial.read(readBuf, sizeof(readBuf));
    if (count > 0) {
        for(int ii = 0; ii < count; ii++, readIndex1++) {
            if (readBuf[ii] != (readIndex1 & 0xff)) {
                if (!valueWarned1) {
                    valueWarned1 = true;
                    Log.error("value1 mismatch readIndex=%u got=0x%02x expected=0x%02x count=%d ii=%d", readIndex1, readBuf[ii], readIndex1 & 0xff, count, ii);
                }
            }
            if ((readIndex1 % 10000) == 0) {
                if (!valueWarned1) {
                    Log.info("readIndex1=%u writeIndex1=%u", readIndex1, writeIndex1);
                }
            }
        }
    }

    while(Serial1.available()) {
        int c = Serial1.read();
        if (c != (readIndex2 & 0xff)) {
            if (!valueWarned2) {
                valueWarned2 = true;
                Log.error("value2 mismatch readIndex=%u got=0x%02x expected=0x%02x", readIndex2, c, readIndex2 & 0xff);
            }
            if ((readIndex2 % 10000) == 0) {
                if (!valueWarned2) {
                    Log.info("readIndex2=%u writeIndex2=%u", readIndex2, writeIndex2);
                }
            }
            readIndex2++;
        }
    }
}

void sendingThreadFunction(void *param) 
{
    while(true) {
        while(Serial1.availableForWrite() > 10) {
            Serial1.write(writeIndex1++ & 0xff);
        }

        size_t count = extSerial.availableForWrite();
        if (count > 20) {
            uint8_t buf[64];

            count -= 10; // Leave some extra space
            for(size_t ii = 0; ii < count; ii++) {
                buf[ii] = (uint8_t)(writeIndex2++ & 0xff);
            }            
            extSerial.write(buf, count);
        }

        delay(1);
    }
}
