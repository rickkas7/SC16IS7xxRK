#include "SC16IS7xxRK.h"

// Pick a debug level from one of these two:
SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);


// - Connect the SC16IS7xx by SPI
// - Use D2 as the SPI CS
// - Connect SC16IS7xx TX to Particle RX
// - Connect SC16IS7xx RX to Particle TX
SC16IS7x0 extSerial;

const int SERIAL_RESET_PIN = -1; // -1 to disable

typedef struct
{
    int serial;
    int extSerial;
    const char *name;
} OptionsPair;

OptionsPair options[10] = {
    {SERIAL_8N1, SC16IS7xxPort::OPTIONS_8N1, "8N1"}, // 0
    {SERIAL_8E1, SC16IS7xxPort::OPTIONS_8E1, "8E1"}, // 1
    {SERIAL_8O1, SC16IS7xxPort::OPTIONS_8O1, "8O1"}, // 2
    {SERIAL_8N2, SC16IS7xxPort::OPTIONS_8N2, "8N2"}, // 3
    {SERIAL_8E2, SC16IS7xxPort::OPTIONS_8E2, "8E2"}, // 4
    {SERIAL_8O2, SC16IS7xxPort::OPTIONS_8O2, "8O2"}, // 5
    {SERIAL_7E1, SC16IS7xxPort::OPTIONS_7E1, "7E1"}, // 6
    {SERIAL_7O1, SC16IS7xxPort::OPTIONS_7O1, "7O1"}, // 7
    {SERIAL_7E2, SC16IS7xxPort::OPTIONS_7E2, "7E2"}, // 8
    {SERIAL_7O2, SC16IS7xxPort::OPTIONS_7O2, "7O2"}  // 9
};

int bauds[] = {1200, 2400, 4800, 9600, 19200, 57600, 115200};

uint8_t tempBuf[16384];

void runSelfTest();

void setup()
{
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    if (SERIAL_RESET_PIN != -1)
    {
        pinMode(SERIAL_RESET_PIN, OUTPUT);
        digitalWrite(SERIAL_RESET_PIN, HIGH);
    }

    delay(5000);

    Serial1.begin(9600);

    extSerial.withSPI(&SPI, D2, 8); // SPI port, CS line, speed in MHz
    // extSerial.withI2C(&Wire, 0);

    extSerial.begin(9600);
}

void loop()
{
    runSelfTest();
    delay(15000);
}

bool clearAvailable()
{
    unsigned long start = millis();

    while (Serial1.available())
    {
        Serial1.read();
        if (millis() - start > 10000)
        {
            Log.info("clearAvailable Serial1 did not clear in 10 seconds");
            return false;
        }
    }

    start = millis();
    while (extSerial.available())
    {
        extSerial.read();
        if (millis() - start > 10000)
        {
            Log.info("extSerial did not clear in 10 seconds");
            return false;
        }
    }
    return true;
}

bool waitForStream(Stream &stream)
{
    unsigned long startTime = millis();

    while (!stream.available() && millis() - startTime < 1000)
    {
        delay(1);
    }
    return stream.available();
}

bool testSimpleReadWrite()
{
    bool bResult;

    clearAvailable();

    for (int ch = 0; ch < 256; ch++)
    {
        extSerial.write(ch);
        bResult = waitForStream(Serial1);
        if (!bResult)
        {
            Log.error("failed line=%d ch=%d", __LINE__, ch);
            return false;
        }
        int value = Serial1.read();
        if (value != ch)
        {
            Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
            return false;
        }

        int ch2 = ~ch & 0xff;

        Serial1.write(ch2);
        bResult = waitForStream(extSerial);
        if (!bResult)
        {
            Log.error("failed line=%d ch=%d", __LINE__, ch);
            return false;
        }
        value = extSerial.read();
        if (value != ch2)
        {
            Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
            return false;
        }
    }

    Log.info("testSimpleReadWrite passed");

    return true;
}

bool testFifo1()
{
    bool bResult;

    clearAvailable();

    int numToTest = 62;

    for (int ch = 0; ch < numToTest; ch++)
    {
        extSerial.write(ch);
    }

    for (int ch = 0; ch < numToTest; ch++)
    {
        bResult = waitForStream(Serial1);
        if (!bResult)
        {
            Log.error("failed line=%d ch=%d", __LINE__, ch);
            return false;
        }
        int value = Serial1.read();
        if (value != ch)
        {
            Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
            return false;
        }
    }

    for (int ch = 0; ch < numToTest; ch++)
    {
        Serial1.write(ch);
    }

    for (int ch = 0; ch < numToTest; ch++)
    {
        bResult = waitForStream(extSerial);
        if (!bResult)
        {
            Log.error("failed line=%d ch=%d", __LINE__, ch);
            return false;
        }
        int value = extSerial.read();
        if (value != ch)
        {
            Log.error("failed line=%d ch=%d value=%d expected=%d", __LINE__, ch, value, ch);
            return false;
        }
    }

    // Success is logged by the caller

    return true;
}

bool testFifoBlock1(bool is7bit)
{
    bool bResult;

    clearAvailable();

    size_t numToTest = 60;

    srand(0);

    // Random test data
    for (size_t ii = 0; ii < numToTest; ii++)
    {
        tempBuf[ii] = rand();
        if (is7bit)
        {
            tempBuf[ii] &= 0x7f;
        }
    }

    uint8_t *buf2 = &tempBuf[256];

    extSerial.write(tempBuf, numToTest);

    for (int ch = 0; ch < (int)numToTest; ch++)
    {
        bResult = waitForStream(Serial1);
        if (!bResult)
        {
            Log.error("failed line=%d ch=%d", __LINE__, ch);
            return false;
        }
        int value = Serial1.read();
        if (value != tempBuf[ch])
        {
            Log.error("failed line=%d ch=%d value=%02x expected=%02x", __LINE__, ch, value, tempBuf[ch]);
            return false;
        }
    }

    for (int ch = 0; ch < (int)numToTest; ch++)
    {
        Serial1.write(tempBuf[ch]);
    }

    for (int ch = 0; ch < (int)numToTest;)
    {
        int count = extSerial.read(buf2, 64);
        if (count > 0)
        {
            for (size_t jj = 0; jj < (size_t)count; jj++)
            {
                if (buf2[jj] != tempBuf[ch + jj])
                {
                    Log.error("failed line=%d ch=%d jj=%u value=%d", __LINE__, ch, jj, buf2[jj]);
                    return false;
                }
            }
            ch += count;
        }
    }

    // Success is logged by the caller

    return true;
}
bool testLarge1()
{
    srand(0);

    // Random test data
    for (size_t ii = 0; ii < sizeof(tempBuf); ii++)
    {
        tempBuf[ii] = rand();
    }

    clearAvailable();

    int readIndex = 0;

    unsigned long start = millis();

    for (size_t ii = 0; ii < sizeof(tempBuf) || readIndex < sizeof(tempBuf);)
    {
        // Don't fill the entire send FIFO as data may be lost because the send and receive FIFOs are
        // both 64 bytes
        while (ii < sizeof(tempBuf) && Serial1.availableForWrite() > 32)
        {
            Serial1.write(tempBuf[ii]);
            ii++;
        }

        while (extSerial.available())
        {
            int c = extSerial.read();
            if (c != tempBuf[readIndex])
            {
                Log.error("testLargefailed line=%d ii=%u got=%02x expected=%02x", __LINE__, ii, c, tempBuf[readIndex]);
                return false;
            }
            readIndex++;
        }
        if (millis() - start >= 30000)
        {
            Log.error("testLargefailed line=%d timeout ii=%u readIndex=%u", __LINE__, ii, readIndex);
            return false;
        }
    }

    Log.info("testLarge passed line=%d", __LINE__);

    return true;
}

bool testBlockRead(bool is7bit)
{
    srand(0);

    // Random test data
    for (size_t ii = 0; ii < sizeof(tempBuf); ii++)
    {
        tempBuf[ii] = rand();
        if (is7bit)
        {
            tempBuf[ii] &= 0x7f;
        }
    }

    clearAvailable();

    size_t writeIndex = 0;
    size_t readIndex = 0;

    unsigned long start = millis();

    while (writeIndex < sizeof(tempBuf) || readIndex < sizeof(tempBuf))
    {
        // Don't fill the entire send FIFO as data may be lost because the send and receive FIFOs are
        // both 64 bytes
        while (writeIndex < sizeof(tempBuf) && Serial1.availableForWrite() > 32)
        {
            Serial1.write(tempBuf[writeIndex++]);
        }

        while (true)
        {
            uint8_t buf[64];
            int count = extSerial.read(buf, sizeof(buf));
            if (count <= 0)
            {
                break;
            }

            for (int jj = 0; jj < count; jj++)
            {
                if (buf[jj] != tempBuf[readIndex])
                {
                    Log.error("testBlockRead line=%d readIndex=%u jj=%u got=%02x expected=%02x", __LINE__, readIndex, jj, buf[jj], tempBuf[readIndex]);
                    return false;
                }
                readIndex++;
            }
        }

        if (millis() - start >= 45000)
        {
            Log.error("testBlockRead line=%d timeout readIndex=%u writeIndex=%u", __LINE__, readIndex, writeIndex);
            return false;
        }
    }

    // Log.info("testBlockRead passed line=%d", __LINE__);

    return true;
}

void runSelfTest()
{

    Log.info("runSelfTest");

    // Hardware reset if the pin has been assigned.
    if (SERIAL_RESET_PIN >= 0)
    {
        digitalWrite(SERIAL_RESET_PIN, LOW);
        delay(1);
        digitalWrite(SERIAL_RESET_PIN, HIGH);
        delay(100);
    }

    testSimpleReadWrite();

    bool bResult = testFifo1();
    if (bResult)
    {
        Log.info("testFifo1 passed line=%d", __LINE__);
    }

    testLarge1();

    for (size_t ii = 0; ii < sizeof(bauds) / sizeof(bauds[0]); ii++)
    {

        for (size_t jj = 0; jj < sizeof(options) / sizeof(options[0]); jj++)
        {

            const char *name = options[jj].name;

            Serial1.begin(bauds[ii], options[jj].serial);
            extSerial.begin(bauds[ii], options[jj].extSerial);

            bool is7bit = (options[jj].extSerial & 0b11) == 0b10;

            delay(10);

            bool bResult = testFifo1();
            if (bResult)
            {
                Log.info("testFifo passed for baud=%d options %s", bauds[ii], name);
            }
            else
            {
                Log.error("testFifo failed line=%d for baud=%d options %s", __LINE__, bauds[ii], name);
            }

            bResult = testFifoBlock1(is7bit);
            if (bResult)
            {
                Log.info("testFifoBlock1 passed for baud=%d options %s", bauds[ii], name);
            }
            else
            {
                Log.error("testFifoBlock1 failed line=%d for baud=%d options %s", __LINE__, bauds[ii], name);
            }

            // This test takes 3 minutes to run at 1200 baud so only run it at 9600 and greater
            if (bauds[ii] >= 9600)
            {
                bResult = testBlockRead(is7bit);
                if (bResult)
                {
                    Log.info("testBlockRead passed for baud=%d options %s", bauds[ii], name);
                }
                else
                {
                    Log.error("testBlockRead failed line=%d for baud=%d options %s", __LINE__, bauds[ii], name);
                }
            }
        }
    }

    Serial1.begin(9600);
    extSerial.begin(9600);
    Log.info("runSelfTest completed");
}
