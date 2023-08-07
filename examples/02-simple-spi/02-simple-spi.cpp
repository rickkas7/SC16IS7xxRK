#include "SC16IS7xxRK.h"

// Pick a debug level from one of these two:
SerialLogHandler logHandler;
// SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SC16IS7x0 extSerial;

char out = ' ';

void setup() {
    // If you want to see the log messages at startup, uncomment the following line
    waitFor(Serial.isConnected, 10000);

	extSerial
        .withSPI(&SPI, D2, 8)   // SPI port, CS line, speed in MHz
        .begin(9600);
}

void loop() {
	while(extSerial.available()) {
		int c = extSerial.read();
		Log.info("received %d", c);
	}

	extSerial.print(out);
	if (++out >= 127) {
		out = ' ';
	}
	delay(100);
}
