#ifndef __SC16IS7XXRK
#define __SC16IS7XXRK

#include "Particle.h"


class SC16IS7x2; // Forward declaration

/**
 * @brief Class for an instance of a UART. 
 * 
 * There is one of these with the SC16IS7x0 and two with the SC16IS7x2. You will not insantiate
 * one of these directly.
 */
class SC16IS7xxPort {
public:

protected:
    SC16IS7xxPort() {};
    virtual ~SC16IS7xxPort() {};

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxPort(const SC16IS7xxPort&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxPort& operator=(const SC16IS7xxPort&) = delete;

    friend class SC16IS7x2;
};

/**
 * @brief Class that manages the SPI or I2C interface. 
 * 
 * You will not instantiate one of these directly.
 */
class SC16IS7xxInterface {
public:
    /**
     * @brief Chip is connected by I2C
     * 
     * @param wire The I2C port, typically &Wire but could be &Wire1, etc.
     * @param addr I2C address (see note below)
     * @return SC16IS7xxInterface& 
     * 
     * A1   A0   Index  I2C Address
     * VDD  VDD   0     0x48
     * VDD  GND   1     0x49
     * VDD  SCL   2     0x4a
     * VDD  SDA   3     0x4b
     * GND  VDD   4     0x4c
     * GND  GND   5     0x4d
     * GND  SCL   6     0x4e
     * GND  SDA   7     0x4f
     * SCL  VDD   8     0x50
     * SCL  GND   9     0x51
     * SCL  SCL  10     0x52
     * SCL  SDA  11     0x53
     * SDA  VDD  12     0x54
     * SDA  GND  13     0x55
     * SDA  SCL  14     0x56
     * SDA  SDA  15     0x57
     * 
     * You can pass either the index (0 - 15) or the I2C address in the address field.
     * Note that the NXP datasheet addresses must be divided by 2 because they include the
     * R/W bit in the address, and Particle and Arduino do not. That's why the datasheet
     * lists the starting address as 0x90 instead of 0x48.
     */
    SC16IS7xxInterface &withI2C(TwoWire *wire, uint8_t addr = 0);

    /**
     * @brief Chip is connected by SPI
     * 
     * @param spi The SPI port. Typically &SPI but could be &SPI1, etc.
     * @param csPin The pin used for chip select
     * @param speedMHz The SPI bus speed in MHz.
     * @return SC16IS7xxInterface& 
     */
    SC16IS7xxInterface &withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz);

    void beginTransaction();

    void endTransaction();


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

};

/**
 * @brief Class for SC16IS740, SC16IS750, SC16IS760 single I2C or SPI UART
 * 
 */
class SC16IS7x0 : public SC16IS7xxPort, public SC16IS7xxInterface {
public:
    /**
     * @brief Default constructor
     * 
     * You typically allocate one of these per 
     */
    SC16IS7x0() {};
    virtual ~SC16IS7x0() {};

    /**
     * @brief Convenience accessor for the port object
     * 
     * @return SC16IS7xxPort& 
     * 
     * This isn't needed because this class derives from SC16IS7xxPort. However, you may want
     * to use this when accessing the port if you will be supporting both the single and dual
     * versions in your code, since the dual version requires that you specify which port
     * you want to work with.
     */
    inline SC16IS7xxPort& a() { return *this; };
    
    /**
     * @brief Convenience accessor for the port object
     * 
     * @param index The port index, must be 0 (though it's ignored)
     * 
     * @return SC16IS7xxPort& 
     * 
     * This isn't needed because this class derives from SC16IS7xxPort. However, you may want
     * to use this when accessing the port if you will be supporting both the single and dual
     * versions in your code, since the dual version requires that you specify which port
     * you want to work with.
     */
    inline SC16IS7xxPort& operator[](size_t index) { return *this; };

protected:
    /**
     * @brief This class is not copyable
     */
    SC16IS7x0(const SC16IS7x0&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7x0& operator=(const SC16IS7x0&) = delete;

};

/**
 * @brief Class for SC16IS852 amd SC16IS862 dual SPI or I2C UART
 * 
 */
class SC16IS7x2 : public SC16IS7xxInterface {
public:
    /**
     * @brief Default constructor
     * 
     * You typically allocate one of these per 
     */
    SC16IS7x2() {};
    virtual ~SC16IS7x2() {};

    /**
     * @brief Gets the port object for port A. This is required to access UART features.
     * 
     * @return SC16IS7xxPort& 
     */
    inline SC16IS7xxPort& a() { return ports[0]; };

    /**
     * @brief Gets the port object for port B. This is required to access UART features.
     * 
     * @return SC16IS7xxPort& 
     */
    inline SC16IS7xxPort& b() { return ports[1]; };

    /**
     * @brief Gets the port object by index
     * 
     * @param index Must be 0 (port A) or 1 (port B). Other values will crash.
     * 
     * @return SC16IS7xxPort& 
     */
    inline SC16IS7xxPort& operator[](size_t index) { return ports[index]; };

protected:
    /**
     * @brief This class is not copyable
     */
    SC16IS7x2(const SC16IS7x2&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7x2& operator=(const SC16IS7x2&) = delete;


    SC16IS7xxPort ports[2];
};


#endif // __SC16IS7XXRK
