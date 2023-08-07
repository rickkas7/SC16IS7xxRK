#include "SC16IS7xxRK.h"



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



protected:
    SC16IS7xxInterface() {};
    virtual ~SC16IS7xxInterface() {};

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxInterface(const SC16IS7xxInterface&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxInterface& operator=(const SC16IS7xxInterface&) = delete;


    TwoWire *wire = nullptr;
    uint8_t i2cAddr = 0;
    SPIClass *spi = nullptr;
    pin_t csPin;
    SPISettings spiSettings;