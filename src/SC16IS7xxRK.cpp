#include "SC16IS7xxRK.h"

static Logger _uartLogger = Logger("app.uart");


bool SC16IS7xxPort::begin(int baudRate, uint8_t options) {

	// My test board uses this oscillator
	// KC3225K1.84320C1GE00
	// OSC XO 1.8432MHz CMOS SMD $1.36
	// https://www.digikey.com/product-detail/en/avx-corp-kyocera-corp/KC3225K1.84320C1GE00/1253-1488-1-ND/5322590
	// Another suggested frequency from the data sheet is 3.072 MHz

	// The divider devices the clock frequency to 16x the baud rate
	int div = interface->oscillatorFreqHz / (baudRate * 16);

	interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, SC16IS7xxInterface::LCR_SPECIAL_START); // 0x80
	interface->writeRegister(channel, SC16IS7xxInterface::DLL_REG, div & 0xff);
	interface->writeRegister(channel, SC16IS7xxInterface::DLH_REG, div >> 8);
	interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, SC16IS7xxInterface::LCR_SPECIAL_END); // 0xbf

	interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, options & 0x3f);

	// Enable FIFOs
	interface->writeRegister(channel, SC16IS7xxInterface::FCR_IIR_REG, 0x07); // Enable FIFO, Clear RX and TX FIFOs

    // Also MCR?
	
	return true;
}

int SC16IS7xxPort::available() {
	return interface->readRegister(channel, SC16IS7xxInterface::RXLVL_REG);
}

int SC16IS7xxPort::availableForWrite() {
	return interface->readRegister(channel, SC16IS7xxInterface::TXLVL_REG);
}


int SC16IS7xxPort::read() {
	if (hasPeek) {
		hasPeek = false;
		return peekByte;
	}
	else {
		if (available()) {
			return interface->readRegister(channel, SC16IS7xxInterface::RHR_THR_REG);
		}
		else {
			return -1;
		}
	}
}

int SC16IS7xxPort::peek() {
	if (!hasPeek) {
		peekByte = read();
		hasPeek = true;
	}
	return peekByte;
}

void SC16IS7xxPort::flush() {
	while(availableForWrite() < 64) {
		delay(1);
	}
}

size_t SC16IS7xxPort::write(uint8_t c) {

	if (writeBlocksWhenFull) {
		// Block until there is room in the buffer
		while(!availableForWrite()) {
			delay(1);
		}
	}

	interface->writeRegister(channel, SC16IS7xxInterface::RHR_THR_REG, c);

	return 1;
}

size_t SC16IS7xxPort::write(const uint8_t *buffer, size_t size) {
	size_t written = 0;
	bool done = false;

	while(size > 0 && !done) {
		size_t count = size;
		if (count > interface->writeInternalMax()) {
			count = interface->writeInternalMax();
		}

		if (writeBlocksWhenFull) {
			while(true) {
				int avail = availableForWrite();
				if (count <= (size_t) avail) {
					break;
				}
				delay(1);
			}
		}
		else {
			int avail = availableForWrite();
			if (count > (size_t) avail) {
				count = (size_t) avail;
				done = true;
			}
		}

		if (count == 0) {
			break;
		}

		if (!interface->writeInternal(channel, buffer, count)) {
			// Failed to write
			break;
		}
		buffer += count;
		size -= count;
		written += count;
	}

	return written;
}

/**
 * @brief Read a multiple bytes to the serial port.
 *
 * @param buffer The buffer to read data into. It will not be null terminated.
 *
 * @param size The maximum number of bytes to read (buffer size)
 *
 * @return The number of bytes actually read or -1 if there are no bytes available to read.
 *
 * This is faster than reading a single byte at time because up to 32 bytes of data can
 * be sent or received in an I2C transaction, greatly reducing overhead.
 */
int SC16IS7xxPort::read(uint8_t *buffer, size_t size) {
	int avail = available();
	if (avail == 0) {
		// No data to read
		return -1;
	}
	if (size > (size_t) avail) {
		size = (size_t) avail;
	}
	if (size > interface->readInternalMax()) {
		size = interface->readInternalMax();
	}
	if (!interface->readInternal(channel, buffer, size)) {
		return -1;
	}

	return (int) size;
}



SC16IS7xxInterface &SC16IS7xxInterface::withI2C(TwoWire *wire, uint8_t addr) {
    this->wire = wire;
    this->i2cAddr = addr;
    if (this->i2cAddr < 0x10) {
        this->i2cAddr += 0x48;
    }
    this->spi = nullptr;
    return *this;
}

SC16IS7xxInterface &SC16IS7xxInterface::withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz) {
    this->wire = nullptr;
    this->spi = spi;
    this->csPin = csPin;
    // The SC16IS7xx all require MSBFIRST  and MODE0, so 
    this->spiSettings = SPISettings(speedMHz*MHZ, MSBFIRST, SPI_MODE0);

    pinMode(this->csPin, OUTPUT);
    digitalWrite(this->csPin, HIGH);

    return *this;
}


void SC16IS7xxInterface::beginTransaction() {
    if (spi) {
        spi->beginTransaction(spiSettings);
        pinResetFast(csPin);
    }
    else 
    if (wire) {
        wire->lock();
    }
}

void SC16IS7xxInterface::endTransaction() {
    if (spi) {
        pinSetFast(csPin);
        spi->endTransaction();
    }
    else 
    if (wire) {
        wire->unlock();
    }

}




// Note: reg is the register 0 - 15, not the shifted value with the channel select bits. Channel is always 0
// on the SC16IS7xxInterface.
uint8_t SC16IS7xxInterface::readRegister(uint8_t channel, uint8_t reg) {
    uint8_t value = 0;

    beginTransaction();

    if (spi) {
        spi->transfer(0x80 | reg << 3 | channel << 1);
        value = (uint8_t) spi->transfer(0);

        endTransaction();
    }
    else
    if (wire) {

        wire->beginTransmission(i2cAddr);
        wire->write(reg << 3 | channel << 1);
        wire->endTransmission(false);

        wire->requestFrom(i2cAddr, 1, true);
        value = (uint8_t) wire->read();
    }

	_uartLogger.trace("readRegister chan=%d reg=%d value=%d", channel, reg, value);

    endTransaction();

	return value;
}

// Note: reg is the register 0 - 15, not the shifted value with the channel select bits
bool SC16IS7xxInterface::writeRegister(uint8_t channel, uint8_t reg, uint8_t value) {
    bool result = false;

    beginTransaction();

    if (spi) {
        spi->transfer(reg << 3 | channel << 1);
        spi->transfer(value);

        _uartLogger.trace("writeRegister chan=%d reg=%d value=%d", channel, reg, value);
    }
    else
    if (wire) {
        wire->beginTransmission(i2cAddr);
        wire->write(reg << 3 | channel << 1);
        wire->write(value);

        int stat = wire->endTransmission(true);

        // stat:
        // 0: success
        // 1: busy timeout upon entering endTransmission()
        // 2: START bit generation timeout
        // 3: end of address transmission timeout
        // 4: data byte transfer timeout
        // 5: data byte transfer succeeded, busy timeout immediately after

        _uartLogger.trace("writeRegister chan=%d reg=%d value=%d stat=%d", channel, reg, value, stat);

        result = (stat == 0);
    }

    endTransaction();

	return result;
}


bool SC16IS7xxInterface::readInternal(uint8_t channel, uint8_t *buffer, size_t size) {
    bool result = false;

    beginTransaction();

    if (spi) {
    	spi->transfer(NULL, buffer, size, NULL);
    }
    else
    if (wire) {
        wire->beginTransmission(i2cAddr);
        wire->write(RHR_THR_REG << 3 | channel << 1);
        wire->endTransmission(false);

        uint8_t numRcvd = wire->requestFrom(i2cAddr, size, (uint8_t)true);
        if (numRcvd == size) {
            for(size_t ii = 0; ii < size; ii++) {
                buffer[ii] = (uint8_t) wire->read();
            }

            _uartLogger.trace("readInternal %d bytes", size);

            result = true;
        }
        if (numRcvd < size) {
            _uartLogger.info("readInternal failed numRcvd=%u size=%u", numRcvd, size);
        }
    }


    endTransaction();

	return result;
}


bool SC16IS7xxInterface::writeInternal(uint8_t channel, const uint8_t *buffer, size_t size) {
    bool result = true;

    beginTransaction();

    if (spi) {
        spi->transfer(RHR_THR_REG << 3 | channel << 1);
    	spi->transfer(const_cast<uint8_t *>(buffer), NULL, size, NULL);

        _uartLogger.trace("writeInternal size=%d", size);
    }
    else
    if (wire) {
        wire->beginTransmission(i2cAddr);
        wire->write(RHR_THR_REG << 3 | channel << 1);
        wire->write(buffer, size);

        int stat = wire->endTransmission(true);

        // stat:
        // 0: success
        // 1: busy timeout upon entering endTransmission()
        // 2: START bit generation timeout
        // 3: end of address transmission timeout
        // 4: data byte transfer timeout
        // 5: data byte transfer succeeded, busy timeout immediately after

        _uartLogger.trace("writeInternal size=%d stat=%d", size, stat);

        result = (stat == 0);
    }

	return result;
}

size_t SC16IS7xxInterface::readInternalMax() const {
    size_t value = 0;

    if (spi) {
        // SPI does allow a large value here, but the FIFO is limited to this size.
        value = 64;
    }
    else
    if (wire) {
        // Newer version of Device OS can change the size of the I2C buffer on some
        // platforms, but this should be big enough for most I2C use cases.
        value = 32;
    }
    return value;
}

size_t SC16IS7xxInterface::writeInternalMax() const {
    size_t value = 0;

    if (spi) {
        // SPI does allow a large value here, but the FIFO is limited to this size.
        value = 64;
    }
    else
    if (wire) {
        // Newer version of Device OS can change the size of the I2C buffer on some
        // platforms, but this should be big enough for most I2C use cases.
        // Note that this is not 32!
        value = 31;
    }
    return value;
}

SC16IS7x0::SC16IS7x0() {
    interface = this;
}

SC16IS7x2::SC16IS7x2() {
    for(size_t ii = 0; ii < (sizeof(ports) / sizeof(ports[0])); ii++) {
        ports[ii].channel = ii;
        ports[ii].interface = this;
    }
}