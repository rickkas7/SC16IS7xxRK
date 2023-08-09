#include "SC16IS7xxRK.h"

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SC16IS7x0 extSerial;

SYSTEM_THREAD(ENABLED);

void setup() {
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

    Serial1.begin(9600);

	extSerial
        .withSPI(&SPI, D2, 8)   // SPI port, CS line, speed in MHz
        .begin(9600);
    
    extSerial.softwareReset();
    extSerial.powerOnCheck();
}

void loop() {
    static unsigned long lastSendTime1 = 0;
    static char nextSendChar1 = ' ';
    if (millis() - lastSendTime1 > 2000) {
        lastSendTime1 = millis();

        Serial1.write(nextSendChar1++);
        if (nextSendChar1 >= 127) {
            nextSendChar1 = ' ';
        }
    }

    static unsigned long lastSendTime2 = 0;
    static char nextSendChar2 = ' ';
    if (millis() - lastSendTime2 > 2000) {
        lastSendTime2 = millis();

        extSerial.write(nextSendChar2++);
        if (nextSendChar2 >= 127) {
            nextSendChar2 = ' ';
        }
    }

    while(extSerial.available()) {
		int c = extSerial.read();
        if (c >= ' ' && c < 127) {
    		Log.info("ext received %c (0x%02x)", c, c);
        }
        else {
    		Log.info("ext received 0x%2x", c);
        }
	}

    if (Serial1.available()) {
        int c = Serial1.read();
        Serial.write(c);

        if (c >= ' ' && c < 127) {
    		Log.info("Serial1 received %c (0x%02x)", c, c);
        }
        else {
    		Log.info("Serial received 0x%2x", c);
        }

    }

}
