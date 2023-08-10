#include "SC16IS7xxRK.h"

static Logger _uartLogger = Logger("app.uart");


SC16IS7xxBuffer::SC16IS7xxBuffer() {

}
SC16IS7xxBuffer::~SC16IS7xxBuffer() {
}

void SC16IS7xxBuffer::free() {
    if (buf) {
        delete[] buf;
        buf = nullptr;        
    }
}


bool SC16IS7xxBuffer::init(size_t bufSize) {
    bool result = false;

    free();

    buf = new uint8_t[bufSize];
    if (buf) {
        this->bufSize = bufSize;
        readOffset = writeOffset = 0;
        result = true;
    }

    return result;
}

size_t SC16IS7xxBuffer::availableToRead() const {
    size_t result;

    WITH_LOCK(*this) {
        result = writeOffset - readOffset;
    }

    return result;
}

int SC16IS7xxBuffer::read() {
    int result = -1;

    WITH_LOCK(*this) {
        if (readOffset < writeOffset) {
            result = buf[readOffset++ % bufSize];
            if (readOffset == writeOffset) {
                readOffset = writeOffset = 0;
            }
        }
    }

    return result;
}

int SC16IS7xxBuffer::read(uint8_t *buffer, size_t size) {
    int result = -1;

    WITH_LOCK(*this) {
        if (readOffset < writeOffset) {
            result = (int)(writeOffset - readOffset);
            if (result > (int)size) {
                result = (int)size;
            }

            for(int ii = 0; ii < result; ii++) {
                buffer[ii] = buf[readOffset++ % bufSize];
            }

            if (readOffset == writeOffset) {
                readOffset = writeOffset = 0;
            }
        }
    }

    return result;
}

size_t SC16IS7xxBuffer::availableToWrite() const {
    size_t result;

    if (buf) {
        // Does not require a mutex lock here since availableToRead obtains a lock
        // The - 1 factor is required because readOffset == writeOffset means empty buffer, not bufSize bytes
        result = bufSize - availableToRead() - 1;
    }
    else {
        result = 0;
    }

    return result;
}

size_t SC16IS7xxBuffer::write(const uint8_t *buffer, size_t size) {
    
    if (!buf) {
        return 0;
    }

    WITH_LOCK(*this) {
        size_t avail = bufSize - (writeOffset - readOffset) - 1;
        if (size > avail) {
            size = avail;
        }
        for(size_t ii = 0; ii < size; ii++) {
            buf[writeOffset++ % bufSize] = buffer[ii];
        }
    }

    return size;
}

void SC16IS7xxBuffer::writeCallback(std::function<void(uint8_t *buffer, size_t &size)> callback) {
    if (!buf) {
        return;
    }

    WITH_LOCK(*this) {
        size_t avail = bufSize - (writeOffset - readOffset);
        
        while(avail > 0) {
            size_t writeOffsetInBlock = writeOffset % bufSize;
            size_t bytesThisBlock = bufSize - writeOffsetInBlock;
            if (bytesThisBlock > avail) {
                bytesThisBlock = avail;
            }
            size_t callbackSize = bytesThisBlock;
            callback(&buf[writeOffsetInBlock], callbackSize);

            writeOffset += callbackSize;
            avail -= callbackSize;

            if (callbackSize < bytesThisBlock) {
                // Didn't read a full buffer. Also handles the end of data case of callbackSize == 0
                break;
            }
        }
    }


}


SC16IS7xxPort &SC16IS7xxPort::withBufferedRead(size_t bufferSize, pin_t intPin) {
    
    if (!readBuffer) {
        readBuffer = new SC16IS7xxBuffer();
        if (readBuffer) {
            readBuffer->init(bufferSize);

            interface->registerThreadFunction([intPin, this]() {
                // This code is called from the worker thread


                if (intPin != PIN_INVALID) {
                    // Check interrupt
                    if (pinReadFast(intPin)) {
                        // Interrupt is not set
                        return;
                    }

                    // Clear interrupt

                }

                size_t rxAvailable = available();
                if (rxAvailable) {                    
                    readBuffer->writeCallback([this, &rxAvailable](uint8_t *buffer, size_t &size) {
                        if (size > rxAvailable) {
                            size = rxAvailable;
                        }
                        if (size > interface->readInternalMax()) {
                            size = interface->readInternalMax();
                        }                        
                        if (size > 0) {
                            interface->readInternal(channel, buffer, size);
                            rxAvailable -= size;
                        }
                    });
                }

            });

            // Enable interrupts
        }
    }
    

    return *this;
}


bool SC16IS7xxPort::begin(int baudRate, uint8_t options) {

	// My test board uses this oscillator
	// KC3225K1.84320C1GE00
	// OSC XO 1.8432MHz CMOS SMD $1.36
	// https://www.digikey.com/product-detail/en/avx-corp-kyocera-corp/KC3225K1.84320C1GE00/1253-1488-1-ND/5322590
	// Another suggested frequency from the data sheet is 3.072 MHz

	// The divider devices the clock frequency to 16x the baud rate
	int div = interface->oscillatorFreqHz / (baudRate * 16);

    _uartLogger.trace("baudRate=%d div=%d options=0x%02x", baudRate, div, options);

	interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, SC16IS7xxInterface::LCR_SPECIAL_ENABLE_DIVISOR_LATCH); // 0x80
	interface->writeRegister(channel, SC16IS7xxInterface::DLL_REG, div & 0xff);
	interface->writeRegister(channel, SC16IS7xxInterface::DLH_REG, div >> 8);
	interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, options & 0x3f); // Clears LCR_SPECIAL_ENABLE_DIVISOR_LATCH

    // options set break, parity, stop bits, and word length

	// Enable FIFOs
	interface->writeRegister(channel, SC16IS7xxInterface::FCR_IIR_REG, 0x07); // Enable FIFO, Clear RX and TX FIFOs

    // Also MCR? This is only accessible when LCR[7] = 0

    // This is how you access the enhanced register set
	// interface->writeRegister(channel, SC16IS7xxInterface::LCR_REG, SC16IS7xxInterface::LCR_ENABLE_ENHANCED_FEATURE_REG); // 0xbf

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
        if (!readBuffer) {
            if (available()) {
                return interface->readRegister(channel, SC16IS7xxInterface::RHR_THR_REG);
            }
            else {
                return -1;
            }
        }
        else {
            return readBuffer->read();
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
    if (!readBuffer) {

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
    else {
        return readBuffer->read(buffer, size);
    }
}



SC16IS7xxInterface &SC16IS7xxInterface::withI2C(TwoWire *wire, uint8_t addr) {
    this->wire = wire;
    this->i2cAddr = addr;
    if (this->i2cAddr < 0x10) {
        this->i2cAddr += 0x48;
    }
    this->spi = nullptr;

    wire->begin();

    return *this;
}

SC16IS7xxInterface &SC16IS7xxInterface::withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz) {
    this->wire = nullptr;
    this->spi = spi;
    this->csPin = csPin;
    // The SC16IS7xx all require MSBFIRST  and MODE0, so 
    this->spiSettings = SPISettings(speedMHz*MHZ, MSBFIRST, SPI_MODE0);

    spi->begin(csPin);

    pinMode(this->csPin, OUTPUT);
    digitalWrite(this->csPin, HIGH);

    return *this;
}


SC16IS7xxInterface &SC16IS7xxInterface::softwareReset() {
    writeRegister(0, SC16IS7xxInterface::IOCONTROL_REG, 0x08); // Bit 3 = SRESET
    return *this;
}

typedef struct {
    uint8_t reg;
    uint8_t value;
} ExpectedRegister;

ExpectedRegister expectedRegisters[] = {
    { SC16IS7xxInterface::IER_REG, 0x00 },
    { SC16IS7xxInterface::FCR_IIR_REG, 0x01}, 
    { SC16IS7xxInterface::LCR_REG, 0b00011101}, // 0x1D
    // TODO: Add more registers here, see page 13 of datasheet
};
const size_t numExpectedRegisters = sizeof(expectedRegisters) / sizeof(expectedRegisters[0]);

bool SC16IS7xxInterface::powerOnCheck() {
    bool good = true;
    
    for(size_t ii = 0; ii < numExpectedRegisters; ii++) {
        uint8_t value = readRegister(0, expectedRegisters[ii].reg);
        if (value != expectedRegisters[ii].value) {
            _uartLogger.info("powerOnCheck register=0x%02x value=%02x expected=%02x",
                expectedRegisters[ii].reg, value, expectedRegisters[ii].value);

            good = false;
        }
    }
    if (good) {
        _uartLogger.info("powerOnCheck passed");
    }
    return good;
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

    switch(reg) {
        case RXLVL_REG:
        case TXLVL_REG:
            // Don't log these because they're called continuously
            break;

        default:
        	_uartLogger.trace("readRegister chan=%d reg=%d value=%d", channel, reg, value);
            break;
    }

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
        // bit 7 (0x80) = read
    	spi->transfer(0x80 | RHR_THR_REG << 3 | channel << 1);
    	spi->transfer(NULL, buffer, size, NULL);
        _uartLogger.trace("readInternal %d bytes", size);
        result = true;
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

void SC16IS7xxInterface::registerThreadFunction(std::function<void()> fn) {
    if (!workerThread) {
        workerThread = new Thread("uart", threadFunctionStatic, (void *)this, OS_THREAD_PRIORITY_DEFAULT, 512);        
    }
    threadFunctions.push_back(fn);
}


void SC16IS7xxInterface::threadFunction() {
	while(true) {
        for(auto fn : threadFunctions) {
            fn();
        }
		delay(1);
	}
}

// [static]
void SC16IS7xxInterface::threadFunctionStatic(void *param) {
	SC16IS7xxInterface *This = (SC16IS7xxInterface *)param;

	This->threadFunction();
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