#ifndef __SC16IS7XXRK
#define __SC16IS7XXRK

#include "Particle.h"


class SC16IS7xxInterface; // Forward declaration
class SC16IS7x2; // Forward declaration

/**
 * @brief Class used internally for buffering data
 * 
 * Since the hardware FIFO is only 64 bytes, this class is used to store data in a larger
 * buffer allocated on the heap.
 * 
 * You do not create one of these objects; it's created automatically when using
 * withBufferedRead().
 */
class SC16IS7xxBuffer {
public:
    /**
     * @brief Construct a buffer object. You will normally never have to instantiate one.
     */
    SC16IS7xxBuffer();

    /**
     * @brief Destructor. You should never need to delete one.
     */
    virtual ~SC16IS7xxBuffer();

    /**
     * @brief Allocate the buffer
     * 
     * @param bufSize Size of the buffer in bytes
     * @param threadCallback 
     * @return true 
     * @return false 
     * 
     * This is called from withBufferedRead(), 
     */
    bool init(size_t bufSize);


    // Read API

    /**
     * @brief Return the number of bytes that can be read from the buffer
     * 
     * @return size_t Number of bytes that can be read. 0 = no data can be read now
     */
    size_t availableToRead() const;

    /**
     * @brief Read a single byte
     * 
     * @return int The byte value (0 - 255) or -1 if there is no data available to read.
     */
    int read();

    /**
     * @brief Read multiple bytes of data to a buffer
     * 
     * @param buffer The buffer to store data into
     * @param size The number of bytes requested
     * @return int The number of bytes actually read.
     * 
     * The read() call does not block. It will only return the number of bytes currently
     * available to read, so the number of bytes read will often be smaller than the 
     * requested number of bytes in size. The returned data is not null terminated.
     */
    int read(uint8_t *buffer, size_t size);

    // Write API

    /**
     * @brief The number of bytes available to write into the buffer
     * 
     * @return size_t Number of bytes that can be written
     */
    size_t availableToWrite() const;

    /**
     * @brief Write data to the buffer
     * 
     * @param buffer 
     * @param size 
     * @return size_t The number of bytes written
     * 
     * This call does not block until there is space in the buffer! If there is 
     * insufficient space, only the bytes that will fit are stored and the return value
     * indicates the amount the was stored.
     */
    size_t write(const uint8_t *buffer, size_t size);

    /**
     * @brief Write data into buffer using a non-copy callback
     * 
     * @param callback Function or C++ lambda, see below
     * 
     * This is used internally from the worker thread after reading a block of data
     * from the UART by I2C or SPI. There is no single-byte write because that is never
     * done because it would be extraordinarily inefficient. 
     * 
     * Callback prototype:
     * void(uint8_t *buffer, size_t &size)
     * 
     * If there is room in the buffer to write data, your callback is called during
     * the execution of writeCallback. It's passed a buffer you should write data to
     * and a size which is the maximum number of bytes you can write. 
     */
    void writeCallback(std::function<void(uint8_t *buffer, size_t &size)> callback);

    /**
     * @brief Lock the buffer mutex
     * 
     * A recursive mutex is used to protect access to buf, readOffset, and writeOffset
     * since they can be accessed from two different threads. The buffer is written from
     * the worker thread and read from whatever thread the user is reading from, typically
     * the application loop thread.
     */
    void lock() const { mutex.lock(); }

    /**
     * @brief Attempt to lock the mutex. If already locked from another thread, returns false.
     * 
     * @return true 
     * @return false 
     */
    bool trylock() const { return mutex.trylock(); }

    /**
     * @brief Attempt to lock the mutex. If already locked from another thread, returns false.
     * 
     * @return true 
     * @return false 
     */
    bool try_lock() const { return mutex.trylock(); }
    
    /**
     * @brief Unlock the buffer mutex
     */
    void unlock() const { mutex.unlock(); }

protected:
    /**
     * @brief This class is not copyable
     */
    SC16IS7xxBuffer(const SC16IS7xxBuffer&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxBuffer& operator=(const SC16IS7xxBuffer&) = delete;

    /**
     * @brief Free the allocate buffer buf - used internally
     */
    void free();


    uint8_t *buf = nullptr; //!< Buffer, allocated on heap
	size_t bufSize = 0; //!< Size of buffer in bytes
    size_t readOffset = 0; //!< Where to read from next, may be larger than bufSize
    size_t writeOffset = 0; //!< Where to write to next, may be larger than bufSize
    mutable RecursiveMutex mutex; //!< Mutex to use to access buf, readOffset, or writeOffset
};

/**
 * @brief Class for an instance of a UART. 
 * 
 * There is one of these with the SC16IS7x0 and two with the SC16IS7x2. You will not instantiate
 * one of these directly.
 */
class SC16IS7xxPort : public Stream {
public:
    /**
     * @brief Enable buffered read mode
     * 
     * @param bufferSize Buffer size in bytes. The buffer is allocated on the heap.
     */
    SC16IS7xxPort &withBufferedRead(size_t bufferSize) { this->bufferedReadSize = bufferSize; return *this; };

    /**
     * @brief Sets the auto RTS hardware flow control levels. Call before begin() to change levels
     * 
     * @param haltLevel Number of characters in receive FIFO to halt transmission. Default: 60.
     * @param resumeLevel Number of characters in receivee FIFO to resume transmission. Default: 30.
     * @return SC16IS7xxPort& 
     */
    SC16IS7xxPort &withTransmissionControlLevels(uint8_t haltLevel, uint8_t resumeLevel);

	/**
	 * @brief Set up the chip. You must do this before reading or writing.
	 *
	 * @param baudRate the baud rate (see below)
	 *
	 * @param options The number of data bits, parity, and stop bits
	 *
	 * You can call begin more than once if you want to change the baud rate. The FIFOs are
	 * cleared when you call begin.
	 *
	 * Available baud rates depend on your oscillator, but with a 1.8432 MHz oscillator, the following are supported:
	 * 50, 75, 110, 134.5, 150, 300, 600, 1200, 1800, 2000, 2400, 3600, 4800, 7200, 9600, 19200, 38400, 57600, 115200
	 *
	 * The valid options in standard number of bits; none=N, even=E, odd=O; number of stop bits format:
	 * OPTIONS_8N1, OPTIONS_8E1, OPTIONS_8O1
	 * OPTIONS_8N2, OPTIONS_8E2, OPTIONS_8O2
	 * OPTIONS_7N1, OPTIONS_7E1, OPTIONS_7O1
	 * OPTIONS_7N2, OPTIONS_7E2, OPTIONS_7O2
     * 
     * It also supports all of the unusual settings supported by the SC16IS7xx. This includes all combinations of 
     * 5, 6, 7, and 8 data bits, 1 or 2 stop bits (1.5 stop bits for 5-bit), and none, even, odd, force 0, or force 1 
     * parity. Use the other OPTIONS_ constants for the unusual combinations, which are the same as the data sheet
     * values.
     * 
     * Unlike the Device OS options, the SC16IS7xx OPTIONS_8N1 value is not 0! If you omit are enabling
     * hardware flow control be sure to set it like 
     * SC16IS7xxPort::OPTIONS_8N1 | SC16IS7xxPort::OPTIONS_FLOW_CONTROL_RTS_CTS
     * If you leave off the OPTIONS_8N1 the output will be 5N1, not 8N1!
	 */
	bool begin(int baudRate, uint32_t options = OPTIONS_8N1);



	/**
	 * @brief Defines what should happen when calls to write()/print()/println()/printlnf() that would overrun the buffer.
	 *
	 * blockOnOverrun(true) - this is the default setting. When there is no room in the buffer for the data to be written,
	 * the program waits/blocks until there is room. This avoids buffer overrun, where data that has not yet been sent over
	 * serial is overwritten by new data. Use this option for increased data integrity at the cost of slowing down realtime
	 * code execution when lots of serial data is sent at once.
	 *
	 * blockOnOverrun(false) - when there is no room in the buffer for data to be written, the data is written anyway,
	 * causing the new data to replace the old data. This option is provided when performance is more important than data
	 * integrity.
	 */
	inline void blockOnOverrun(bool value = true) { writeBlocksWhenFull = value; };

	/**
	 * @brief Returns the number of bytes available to read from the serial port
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int available();

	/**
	 * @brief Returns the number of bytes available to write into the TX FIFO
	 */
    virtual int availableForWrite();

	/**
	 * @brief Read a single byte from the serial port
	 *
	 * @return a byte value 0 - 255 or -1 if no data is available.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int read();

	/**
	 * @brief Read a single byte from the serial port, but do not remove it so it can be read again.
	 *
	 * @return a byte value 0 - 255 or -1 if no data is available.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual int peek();

	/**
	 * @brief Block until all serial data is sent.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual void flush();

	/**
	 * @brief Write a single byte to the serial port.
	 *
	 * @param c The byte to write. Can write binary or text data.
	 *
	 * This is a standard Arduino/Wiring method for Stream objects.
	 */
    virtual size_t write(uint8_t c);

	/**
	 * @brief Write a multiple bytes to the serial port.
	 *
	 * @param buffer The buffer to write. Can write binary or text data.
	 *
	 * @param size The number of bytes to write
	 *
	 * @return The number of bytes written.
	 *
	 * This is faster than writing a single byte at time because up to 31 bytes of data can
	 * be sent or received in an I2C transaction, greatly reducing overhead. For SPI,
     * 64 bytes can be written at a time.
	 */
	virtual size_t write(const uint8_t *buffer, size_t size);

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
	 * be sent or received in an I2C transaction, greatly reducing overhead. For SPI,
     * 64 bytes can be written at a time.
	 */
	virtual int read(uint8_t *buffer, size_t size);


    // Mask 0x3f of options (low 6 bits) are the data bits, parity, and stop bits
    static const uint32_t OPTIONS_BITS_5 = 0x0b00; //!< 5 data bits
    static const uint32_t OPTIONS_BITS_6 = 0x0b01; //!< 6 data bits
    static const uint32_t OPTIONS_BITS_7 = 0x0b10; //!< 7 data bits
    static const uint32_t OPTIONS_BITS_8 = 0x0b11; //!< 8 data bits

    static const uint32_t OPTIONS_STOP_1 = 0b000; //!< 1 stop bit 
    static const uint32_t OPTIONS_STOP_2 = 0b100; //!< 2 stop bits (6, 7, 8 data bits), 1 1/2 stop bits (5 data bits)

    static const uint32_t OPTIONS_PARITY_NONE    = 0b000000; //!< No parity
    static const uint32_t OPTIONS_PARITY_ODD     = 0b001000; //!< Odd parity
    static const uint32_t OPTIONS_PARITY_EVEN    = 0b011000; //!< Even parity
    static const uint32_t OPTIONS_PARITY_FORCE_1 = 0b101000; //!< Force parity 1
    static const uint32_t OPTIONS_PARITY_FORCE_0 = 0b111000; //!< Force parity 0

	static const uint32_t OPTIONS_8N1 = 0b000011; //!< 8 data bits, no parity, 1 stop bit
	static const uint32_t OPTIONS_8E1 = 0b011011; //!< 8 data bits, even parity, 1 stop bit
	static const uint32_t OPTIONS_8O1 = 0b001011; //!< 8 data bits, odd parity, 1 stop bit

	static const uint32_t OPTIONS_8N2 = 0b000111; //!< 8 data bits, no parity, 2 stop bits
	static const uint32_t OPTIONS_8E2 = 0b011111; //!< 8 data bits, even parity, 2 stop bits
	static const uint32_t OPTIONS_8O2 = 0b001111; //!< 8 data bits, odd parity, 2 stop bits

	static const uint32_t OPTIONS_7N1 = 0b000010; //!< 7 data bits, no parity, 1 stop bit
	static const uint32_t OPTIONS_7E1 = 0b011010; //!< 7 data bits, even parity, 1 stop bit
	static const uint32_t OPTIONS_7O1 = 0b001010; //!< 7 data bits, odd parity, 1 stop bit

	static const uint32_t OPTIONS_7N2 = 0b000110; //!< 7 data bits, no parity, 2 stop bits
	static const uint32_t OPTIONS_7E2 = 0b011110; //!< 7 data bits, even parity, 2 stop bits
	static const uint32_t OPTIONS_7O2 = 0b001110; //!< 7 data bits, odd parity, 2 stop bits

	static const uint32_t OPTIONS_FLOW_CONTROL_NONE    = 0b00000000; //!< No hardware flow control (default)
	static const uint32_t OPTIONS_FLOW_CONTROL_RTS     = 0b01000000; //!< RTS flow control (/RTS output indicates this side can receive data)
	static const uint32_t OPTIONS_FLOW_CONTROL_CTS     = 0b10000000; //!< CTS flow control (/CTS input indicates the other side can receive data)
	static const uint32_t OPTIONS_FLOW_CONTROL_RTS_CTS = 0b11000000; //!< Hardware flow control in both directions


protected:
    /**
     * @brief Protected constructor; you never construct one of these directly
     */
    SC16IS7xxPort() {};

    /**
     * @brief Protected destructor; you never delete one of these directly
     */
    virtual ~SC16IS7xxPort() {};

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxPort(const SC16IS7xxPort&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxPort& operator=(const SC16IS7xxPort&) = delete;

    /**
     * @brief Handle reading the IIR register and dispatching to the interrupt handler
     * 
     * This is used interally and you cannot call it.
     */
    void handleIIR();


	bool hasPeek = false; //!< There is a byte from the last peek() available
	uint8_t peekByte = 0; //!< The byte that was read if hasPeek == true
	bool writeBlocksWhenFull = true;
    uint8_t channel = 0; //!< Chip channel number for this port (0 or 1)
    uint8_t ier = 0; //!< Value of the IER register, set from begin()
    uint8_t lcr = 0; //<! Value of the LCR register, set from begin()
    uint8_t efr = 0; //<! Value of the EFR register, set from begin()
    uint8_t mcr = 0; //!< Value of the MCR register, set from begin()
    uint8_t tcr = (uint8_t)(30 << 4 | 60); //!< Default TCR value for hardware flow control (resume 30, halt 60)
    uint8_t tlr = 0;
    SC16IS7xxInterface *interface = nullptr; //!< Interface object for this chip

    SC16IS7xxBuffer *readBuffer = nullptr; //!< Buffer object when using withReadBuffer
    size_t bufferedReadSize = 0; //!< Size of buffer for buffered read (0 = buffered read not enabled)
    uint8_t readFifoInterruptLevel = 30; //!< Interrupt when FIFO has 30 characters (or timeout)
    bool readDataAvailable = false; //!< Set from interruptRxTimeout and interruptRHR

    std::function<void()> interruptLineStatus = nullptr; //!< Function to call for a line status interrupt
    std::function<void()> interruptRxTimeout = nullptr; //!< Function to call for stale data in RX FIFO
    std::function<void()> interruptRHR = nullptr; //!< Function to call for RHR FIFO above level
    std::function<void()> interruptTHR = nullptr; //!< Function to call for THR FIFO below level
    std::function<void()> interruptModemStatus = nullptr; //!< Function to call for change in modem input status
    std::function<void()> interruptIO = nullptr; //!< Function to call for GPIO interrupt
    std::function<void()> interruptXoff = nullptr; //!< Function to call for a received Xoff with software flow control, or special bytes
    std::function<void()> interruptCTS_RTS = nullptr; //!< Function to call for CTS or RTS transition low to high

    friend class SC16IS7x2; //!< The SC16IS7x0 derives from this, but the SC16IS7x2 has this ports as member variables
    friend class SC16IS7xxInterface; //!< Allows the interface to call private members of this class, used to call handleIIR()
};

/**
 * @brief Class for managing GPIO attached to the SC16IS7xx.
 * 
 * The GPIO features available depend on the chip.
 * 
 * - SC16IS740 does not support GPIO (does not have the pins)
 * - SC16IS750 and SC16IS760 can support 8 GPIO, or 4 GPIO plus extended flow control (DSR, DTR, CD, RI).
 * - SC16IS752 and SC16IS762 can support 8 GPIO, or extended flow control (DSR, DTR, CD, RI) with no additional GPIO.
 * 
 * In other words, since extended flow control (DSR, DTR, CD, RI) uses 4 GPIO, on dual-port devices, extended flow
 * control will use all available GPIO.
 * 
 * Note that "interrupt mode" is not a true ISR. Since the status of the chip must be checked from SPI or I2C, which
 * cannot be locked from an ISR, these are all handled from the worker thread, not a true ISR. Because the worker thread
 * is shared with buffered reading mode and has a small stack, you still should not do lengthy operations from an
 * interrupt handler. 
 */
class SC16IS7xxGPIO {
public:
    SC16IS7xxGPIO(SC16IS7xxInterface *interface);

    virtual ~SC16IS7xxGPIO();

	/**
	 * @brief Sets the SC16IS7xxGPIO GPIO pin mode
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The mode, one of the following:
	 * 
	 * - `INPUT` 
	 * - `INPUT_PULLUP`
	 * - `OUTPUT`
	 * 
	 * Note that the SC16IS7xxGPIO does not support input with pull-down or open-drain outputs
	 * for its GPIO.
	 */
	void pinMode(uint16_t pin, PinMode mode);

	/**
	 * @brief Gets the currently set pin mode
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @return One of these constants:
	 * 
	 * - `INPUT` 
	 * - `OUTPUT`
     * - 
	 * 
     * Note: The SC16IS7xx does not support pull-up or pull-down modes (internal pull).
	 */
	PinMode getPinMode(uint16_t pin);

	/**
	 * @brief Returns true if the pin exists on the device
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @return true if pin is 0 <= pin < 8, otherwise false
	 */
	bool pinAvailable(uint16_t pin) const;

	/**
	 * @brief Sets the output value of the pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param value The value to set the pin to. Typically you use one of:
	 * 
	 * - 0, false, or LOW
	 * - 1, true, or HIGH
	 * 
	 * You must have previously set the pin to OUTPUT mode before using this method.
	 * 
	 * This performs and I2C transaction, so it will be slower that MCU native digitalWrite.
	 */
	void digitalWrite(uint16_t pin, uint8_t value);

	/**
	 * @brief Reads the input value of a pin
	 * 
	 * @return The value, 0 or 1:
	 * 
	 * - 0, false, or LOW
	 * - 1, true, or HIGH
	 * 
	 * This performs and I2C transaction, so it will be slower that MCU native digitalRead.
	 */
	int32_t digitalRead(uint16_t pin);

	/**
	 * @brief Reads all of the pins at once
	 * 
	 * @return A bit mask of the pin values
	 * 
	 * The bit mask is as follows:
	 * 
	 * | Pin   | Mask              |
	 * | :---: | :---------------- |
	 * | 0     | 0b00000001 = 0x01 |
	 * | 1     | 0b00000010 = 0x02 |
	 * | 2     | 0b00000100 = 0x04 |
	 * | 3     | 0b00001000 = 0x08 |
	 * | 4     | 0b00010000 = 0x10 |
	 * | 5     | 0b00100000 = 0x20 |
	 * | 6     | 0b01000000 = 0x40 |
	 * | 7     | 0b10000000 = 0x80 |
	 * 
	 * In other words: `bitMask = (1 << pin)`.
	 * 
	 * This performs and I2C transaction, but all pins are read with a single I2C transaction
	 * so it is faster than calling digitalRead() multiple times. 
	 */
	uint8_t readAllPins();

	/**
	 * @brief Enables SC16IS7xxGPIO interrupt mode
	 * 
	 * @param mcuInterruptPin The MCU pin (D2, A2, etc.) the SC16IS7xxGPIO INT pin is connected
	 * to. This can also be `PIN_INVALID` if you want to use the latching change mode
	 * but do not have the INT pin connected or do not have a spare MCU GPIO.
	 * 
	 * @param outputType The way the INT pin is connected. See MCP23008InterruptOutputType. 
	 * 
	 * Interrupt mode uses the INT output of the SC16IS7xxGPIO to connect to a MCU GPIO pin.
	 * This allows a change (RISING, FALLING, or CHANGE) on one or more GPIO connected
	 * to the expander to trigger the INT line. This is advantageous because the MCU
	 * can poll the INT line much more efficiently than making an I2C transaction to
	 * poll the interrupt register on the SC16IS7xxGPIO.
	 * 
	 * Note that this is done from a thread, not using an MCU hardware interrupt. This
	 * is because the I2C transaction requires getting a lock on the Wire object, which
	 * cannot be done from an ISR. Also, because the SC16IS7xxGPIO INT output is latching,
	 * there is no danger of missing it when polling.
	 * 
	 * It's possible for multiple SC16IS7xxGPIO to share a single MCU interrupt line by
	 * using open-drain mode to logically OR them together with no external gate required.
	 * You can also use separate MCU interrupt lines, if you prefer.
	 * 
	 * If you pass `PIN_INVALID` for mcuInterruptPin, outputType is ignored. This mode is
	 * used when you still want to be able to handle latching RISING, FALLING, or CHANGE
	 * handlers but do not have the INT pin connected or do not have spare MCU GPIOs.
	 * This requires an I2C transaction on every thread timeslice (once per millisecond)
	 * so it's not as efficient as using a MCU interrupt pin, but this mode is supported.
	 */
	void enableInterrupts(pin_t mcuInterruptPin, MCP23008InterruptOutputType outputType);

	/**
	 * @brief Attaches an interrupt handler for a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The interrupt handler mode, one of: `RISING`, `FALLING`, or `CHANGE`.
	 * 
	 * @param handler A function or C++11 lambda with the following prototype:
	 * 
	 * ```
	 * void handlerFunction(bool newState);
	 * ```
	 * 
	 * You must call enableInterrupts() before making this call! 
	 * 
	 * Note that the handler will be called from a thread, not an ISR, so it's not a true
	 * interrupt handler. The reason is that in order to handle the SC16IS7xxGPIO interrupt, more
	 * than one I2C transaction is required. This cannot be easily done at ISR time because
	 * an I2C lock needs to be acquired. It can, however, be easily done from a thread, which
	 * is what is done here. enableInterrupts() starts this thread.
	 * 
	 * Even though handler is called from a thread and not an ISR you should still avoid any
	 * lengthy operations during it, as the thread handles all interrupts on all SC16IS7xxGPIO
	 * and blocking it will prevent all other interrupts from being handled.
	 * 
	 * There is also a version of this method that takes a class member function below.
	 * 
	 * If you need to pass additional data ("context") you should instead use a C++11 lambda.
	 * 
	 * You must not call attachInterrupt() from an interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.
	 */
	void attachInterrupt(uint16_t pin, InterruptMode mode, std::function<void(bool)> handler);

	/**
	 * @brief Attaches an interupt handler in a class member function to a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * @param mode The interrupt handler mode, one of: `RISING`, `FALLING`, or `CHANGE`.
	 * 
	 * @param callback A C++ class member function with following prototype:
	 * 
	 * ```
	 * void MyClass::handler(bool newState);
	 * ```
	 * 
	 * @param instance The C++ object instance ("this") pointer.
	 * 
	 * You typically use it like this:
	 * 
	 * ```
	 * attachInterrupt(2, FALLING, &MyClass::handler, this);
	 * ```
	 * 
	 * You must call enableInterrupts() before making this call! 
	 * 
	 * Note that the handler will be called from a thread, not an ISR, so it's not a true
	 * interrupt handler. The reason is that in order to handle the SC16IS7xxGPIO interrupt, more
	 * than one I2C transaction is required. This cannot be easily done at ISR time because
	 * an I2C lock needs to be acquired. It can, however, be easily done from a thread, which
	 * is what is done here. enableInterrupts() starts this thread.
	 * 
	 * Even though handler is called from a thread and not an ISR you should still avoid any
	 * lengthy operations during it, as the thread handles all interrupts on all SC16IS7xxGPIO
	 * and blocking it will prevent all other interrupts from being handled.
	 * 
	 * There is also a version of this method that takes a class member function below.
	 * 
	 * If you need to pass additional data ("context") you should instead use a C++11 lambda.
	 * 
	 * You must not call attachInterrupt() from an interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.	 
	 */
	template <typename T>
    void attachInterrupt(uint16_t pin, InterruptMode mode, void (T::*callback)(bool), T *instance) {
        return attachInterrupt(pin, mode, std::bind(callback, instance, std::placeholders::_1));
    };

	/**
	 * @brief Detaches an interrupt handler for a pin
	 * 
	 * @param pin The GPIO pin 0 - 7.
	 * 
	 * You must not call detachInterrupt() from your interrupt handler! If you do so, this function
	 * will deadlock the thread and all interrupt-related functions will stop working.
	 * 
	 * You should avoid attaching and detaching an interrupt excessively as it's a relatively expensive 
	 * operation. You should only attach and detach from loop().
	 */
	void detachInterrupt(uint16_t pin);


private:

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxGPIO(const SC16IS7xxGPIO&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxGPIO& operator=(const SC16IS7xxGPIO&) = delete;

	/**
	 * @brief Reads an MCP23008 register and masks off a specific bit
	 * 
	 * @param reg The register number, typically one of the constants like `REG_GPIO` (9).
	 * 
	 * @param pin The pin 0 - 7
	 * 
	 * @return the value boolean value
	 * 
	 * For registers that contain a bitmask of all 8 values (like REG_GPIO), reads all
	 * registers and then performs the necessary bit manipulation.
	 * 
	 * There is no way to know if this actually succeeded.
	 */
	bool readRegisterPin(uint8_t reg, uint16_t pin);

	/**
	 * @brief Reads an MCP23008 register, masks off a specific bit, and writes the register back
	 * 
	 * @param reg The register number, typically one of the constants like `REG_GPIO` (9).
	 * 
	 * @param pin The pin 0 - 7
	 * 
	 * @param value The bit value to set (true or false)
	 * 
	 * @return true if the I2C operation completed successfully or false if not.
	 * 
	 * For registers that contain a bitmask of all 8 values (like REG_GPIO), reads all
	 * registers, performs the necessary bit manipulation, and writes the register back.
	 * 
	 * The read/modify/write cycle is done within a single I2C lock()/unlock() pair to
	 * minimize the chance of simultaneous modification.
	 */
	bool writeRegisterPin(uint8_t reg, uint16_t pin, bool value);


    /**
     * @brief Lock the buffer mutex
     * 
     * A recursive mutex is used to protect access to buf, readOffset, and writeOffset
     * since they can be accessed from two different threads. The buffer is written from
     * the worker thread and read from whatever thread the user is reading from, typically
     * the application loop thread.
     */
    void lock() const { mutex.lock(); }

    /**
     * @brief Attempt to lock the mutex. If already locked from another thread, returns false.
     * 
     * @return true 
     * @return false 
     */
    bool trylock() const { return mutex.trylock(); }

    /**
     * @brief Attempt to lock the mutex. If already locked from another thread, returns false.
     * 
     * @return true 
     * @return false 
     */
    bool try_lock() const { return mutex.trylock(); }
    
    /**
     * @brief Unlock the buffer mutex
     */
    void unlock() const { mutex.unlock(); }

	// These are just for reference so you can see which way the pin numbers are laid out
	// (1 << pin) is also a good way to map pin numbers to their bit
	// Do not use these constants for any uint16_t pin variable, those functions require 0 - 7
	// not a bitmask!
	static const uint8_t PIN_0 = 0b00000001; 	//!< bit mask for GP0
	static const uint8_t PIN_1 = 0b00000010; 	//!< bit mask for GP1
	static const uint8_t PIN_2 = 0b00000100; 	//!< bit mask for GP2
	static const uint8_t PIN_3 = 0b00001000; 	//!< bit mask for GP3
	static const uint8_t PIN_4 = 0b00010000; 	//!< bit mask for GP4
	static const uint8_t PIN_5 = 0b00100000; 	//!< bit mask for GP5
	static const uint8_t PIN_6 = 0b01000000; 	//!< bit mask for GP6
	static const uint8_t PIN_7 = 0b10000000; 	//!< bit mask for GP7

	static const uint16_t NUM_PINS = 8; 		//!< Number of GPIO pins

    /**
     * @brief Thread function called from FreeRTOS. Never returns!
     */
    void threadFunction();

    /**
     * @brief Static thread function, called from FreeRTOS
     *
     * Note: param must be a pointer to this. threadFunction is called from this function.
     * Never returns!
     */
    static void threadFunctionStatic(void *param);

    Thread *workerThread = nullptr; //!< Worker thread, created if interrupts are used

    mutable RecursiveMutex mutex; //!< Mutex to use to access handlers

    SC16IS7xxInterface *interface;

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
     * @param wire The I2C port, typically &Wire but could be &Wire1, etc. Default is Wire (primary I2C on D0/D1).
     * @param addr I2C address (see note below). Default is both 0x48, A0=HIGH, A1=HIGH.
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
     * 
     * The most common scenarios are both high (addr = 0x48 or 0x00) or both low (addr = 0x4d or 0x05)
     * 
     * Because of the way I2C works, the speed is selected per bus and not per I2C slave
     * device like SPI. If all of your I2C devices work at 400 kHz, using `Wire.setSpeed(CLOCK_SPEED_400KHZ)`
     * is recommended for better performance.
     * 
     * This call must be make before any begin() calls. It will have no effect after begin().
     */
    SC16IS7xxInterface &withI2C(TwoWire *wire = &Wire, uint8_t addr = 0);

    /**
     * @brief Chip is connected by SPI
     * 
     * @param spi The SPI port. Typically &SPI but could be &SPI1, etc.
     * @param csPin The pin used for chip select
     * @param speedMHz The SPI bus speed in MHz.
     * @return SC16IS7xxInterface& 
     * 
     * This call must be make before any begin() calls. It will have no effect after begin().
     */
    SC16IS7xxInterface &withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz);

    /**
     * @brief Set the oscillator frequency for the chip
     * 
     * @param freqHz Typically 1843200 (default, 1.8432 MHz), or 3072000 (3.072 MHz).
     * @return * SC16IS7xxInterface& 
     * 
     * This call must be make before any begin() calls. It will have no effect after begin().
     */
    SC16IS7xxInterface &withOscillatorFrequency(int freqHz) { this->oscillatorFreqHz = freqHz; return *this; };


    /**
     * @brief Enable GPIO mode on SC16IS750, SC16IS760, SC16IS752, SC16IS762
     * 
     * This call must be make before any begin() calls. It will have no effect after begin().
     */
    SC16IS7xxInterface &withEnableGPIO(bool enable = true) { this->enableGPIO = enable; return *this; }
    
    /**
     * @brief Sets the pin used for IRQ (hardware interrupts). This is optional.
     * 
     * @param irqPin Pin to use (D3, D4, ...)
     * @param mode The pin mode. Default is INPUT_PULLUP. Could be INPUT instead.
     * @return SC16IS7xxInterface& 
     * 
     * This call must be make before any begin() calls. It will have no effect after begin().
     */
    SC16IS7xxInterface &withIRQ(pin_t irqPin, PinMode mode = INPUT_PULLUP);

    /**
     * @brief Do a software reset of the device
     */
    SC16IS7xxInterface &softwareReset();

    /**
     * @brief Do a check to see if the device is set to the expected power-on values 
     * 
     */
    bool powerOnCheck();

    /**
     * @brief Read a register
     *
     * @param reg The register number to read. Note that this should be the register 0 - 16, before shifting for channel.
     */
	uint8_t readRegister(uint8_t channel, uint8_t reg);

	/**
     * @brief Write a register
     *
     * @param reg The register number to write. Note that this should be the register 0 - 16, before shifting for channel.
     *
     * @param value The value to write
     */
	bool writeRegister(uint8_t channel, uint8_t reg, uint8_t value);


    /**
     * @brief Calls a callback for each port on this chip
     * 
     * @param callback 
     */
    virtual void forEachPort(std::function<void(SC16IS7xxPort *port)> callback) = 0;

	static const uint8_t RHR_THR_REG = 0x00; //!< Receive Holding Register (RHR) and Transmit Holding Register (THR)
	static const uint8_t IER_REG = 0x01;  //!< Interrupt Enable Register (IER)
	static const uint8_t FCR_IIR_REG = 0x02; //!< Interrupt Identification Register (IIR) and FIFO Control Register (FCR)
	static const uint8_t LCR_REG = 0x03; //!< Line Control Register (LCR)
	static const uint8_t MCR_REG = 0x04; //!< Modem Control Register (MCR)
	static const uint8_t LSR_REG = 0x05; //!< Line Status Register (LSR)
	static const uint8_t MSR_REG = 0x06; //!< Modem Status Register (MSR)
	static const uint8_t TCR_REG = 0x06; //!< Transmission control register (TCR) when MCR[2] = 1 and EFR[4] = 1
	static const uint8_t SPR_REG = 0x07; //!< Scratchpad Register (SPR) - if MCR[2] = 1 and EFR[4] = 1
	static const uint8_t TLR_REG = 0x07; //!< Transmission level register (TLR) - if MCR[2] = 1 and EFR[4] = 1
	static const uint8_t TXLVL_REG = 0x08; //!< Transmit FIFO Level register
	static const uint8_t RXLVL_REG = 0x09; //!< Receive FIFO Level register
	static const uint8_t IODIR_REG = 0x0a; //!< I/O pin Direction register
	static const uint8_t IOSTATE_REG = 0x0b; //!< I/O pins State register
	static const uint8_t IOINTENA_REG = 0x0c; //!< I/O Interrupt Enable register
	static const uint8_t IOCONTROL_REG = 0x0e; //!< I/O pins Control register
	static const uint8_t EFCR_REG = 0x0f; //!< Extra Features Control Register

    // IIR

	// Special register block
    static const uint8_t LCR_DEFAULT = 0x1D; //!< Power-on default value of LCR
	static const uint8_t LCR_SPECIAL_ENABLE_DIVISOR_LATCH = 0x80; //!< 
	static const uint8_t LCR_ENABLE_ENHANCED_FEATURE_REG = 0xbf; //!< 
	static const uint8_t DLL_REG = 0x00; //!< Divisor Latch LSB (DLL)
	static const uint8_t DLH_REG = 0x01; //!< Divisor Latch MSB (DLH)

	// Enhanced register set
	static const uint8_t EFR_REG = 0x02; //!< Enhanced Features Register (EFR)
	static const uint8_t XON1_REG = 0x04; //!< Xon1 word
	static const uint8_t XON2_REG = 0x05; //!< Xon2 word
	static const uint8_t XOFF1_REG = 0x06; //!< Xoff1 word
	static const uint8_t XOFF2_REG = 0x07; //!< Xoff2 word

protected:
    SC16IS7xxInterface() {}; //!< You cannot instantiate this directly
    virtual ~SC16IS7xxInterface() {}; //!< You cannot delete this directly

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxInterface(const SC16IS7xxInterface&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7xxInterface& operator=(const SC16IS7xxInterface&) = delete;

    /**
     * @brief Begins a SPI or I2C transaction
     * 
     * A transaction is a group of related calls. Within the transaction other devices are prohibited
     * from accessing the bus, but thread swapping is still enabled.
     */
    void beginTransaction();

    /**
     * @brief Ends a SPI or I2C transaction
     * 
     * A transaction is a group of related calls. Within the transaction other devices are prohibited
     * from accessing the bus, but thread swapping is still enabled.
     */
    void endTransaction();


	/**
	 * @brief Internal function to read data
	 *
	 * It can only read 32 bytes at a time, the maximum that will fit in a 32 byte I2C transaction with
	 * the register address in the first byte.
	 */
	bool readInternal(uint8_t channel, uint8_t *buffer, size_t size);


	/**
	 * @brief Internal function to write data
	 *
	 * It can only write 31 bytes at a time, the maximum that will fit in a 32 byte I2C transaction with
	 * the register address in the first byte.
	 */
	virtual bool writeInternal(uint8_t channel, const uint8_t *buffer, size_t size);

	/**
	 * @brief Maximum number of bytes that can be read by readInternal
	 */
	size_t readInternalMax() const;

	/**
	 * @brief Maximum number of bytes that can be written by writeInternal
	 */
	size_t writeInternalMax() const;

    void registerThreadFunction(std::function<void()> fn);

    /**
     * @brief Thread function called from FreeRTOS. Never returns!
     */
    void threadFunction();

    /**
     * @brief Static thread function, called from FreeRTOS
     *
     * Note: param must be a pointer to this. threadFunction is called from this function.
     * Never returns!
     */
    static void threadFunctionStatic(void *param);


    TwoWire *wire = nullptr; //!< When using I2C, the Wire object, typically Wire, but could be Wire1.
    uint8_t i2cAddr = 0; //!< When using I2C, the I2C address of the SC16IS7xx chip.
    SPIClass *spi = nullptr; //!< When using SPI, the SPI object (typically SPI or SPI1).
    pin_t csPin = PIN_INVALID; //!< When using SPI, the CS pin (required for SPI)
    pin_t irqPin = PIN_INVALID; //!< Hardware IRQ from SC16IS7xx, optional.
    size_t irqFifoLevel = 30; //!< Number of bytes to trigger IRQ
    SPISettings spiSettings; //!< When using SPI, the SPISettings (bit rate, bit order, mode).
    bool enableGPIO; //!< Enable GPIO mode
    int oscillatorFreqHz = 1843200; //!< Oscillator frequency. Default is 1.8432 MHz, can also be 3072000 (3.072 MHz).
    Thread *workerThread = nullptr; //!< Worker thread, created if registerThreadFunction() is called.
    std::vector<std::function<void()>> threadFunctions; //!< Functions to call from the worker thread, added using registerThreadFunction()


    friend class SC16IS7xxPort; //!< The port object calls the interface object and uses the register contents
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
     * You typically allocate one of these per chip as global variables.
     */
    SC16IS7x0();

    /**
     * @brief Since the object is typically a global variable, it is not intended to be deleted
     * 
     * If you delete this object and are using the worker thread, the device will probably crash since the
     * thread won't be stopped.
     */
    virtual ~SC16IS7x0() {};

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
     *
     * All models support speeds up to 400 Kbit/sec over I2C.
     */
    SC16IS7x0 &withI2C(TwoWire *wire, uint8_t addr = 0) { SC16IS7xxInterface::withI2C(wire, addr); return *this; };

    /**
     * @brief Chip is connected by SPI
     * 
     * @param spi The SPI port. Typically &SPI but could be &SPI1, etc.
     * @param csPin The pin used for chip select. This is required.
     * @param speedMHz The SPI bus speed in MHz. The default is 4 Kbit/sec., which works with all chips.
     * @return SC16IS7xxInterface& 
     * 
     * The SC16IS740 and SC16IS750 supports SPI speeds up to 4 Mbit/sec. The SC16IS760 supports SPI speeds up to 15 Mbit/sec.
     */
    SC16IS7x0 &withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz = 4) { SC16IS7xxInterface::withSPI(spi, csPin, speedMHz); return *this; };

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

    /**
     * @brief Call callback for each port
     * 
     * For the SC16IS7x0 there is only one port, but this makes operation consistent across chips.
     */
    void forEachPort(std::function<void(SC16IS7xxPort *port)> callback) { callback(this); };

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
     * You typically allocate one of these per chip as global variables.
     */
    SC16IS7x2();

    /**
     * @brief Since the object is typically a global variable, it is not intended to be deleted
     * 
     * If you delete this object and are using the worker thread, the device will probably crash since the
     * thread won't be stopped.
     */
    virtual ~SC16IS7x2() {};

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
     * 
     * The SC16IS752 and SC16IS762 support up to 400 Kbit/sec I2C.
     */
    SC16IS7x2 &withI2C(TwoWire *wire, uint8_t addr = 0) { SC16IS7xxInterface::withI2C(wire, addr); return *this; };

    /**
     * @brief Chip is connected by SPI
     * 
     * @param spi The SPI port. Typically &SPI but could be &SPI1, etc.
     * @param csPin The pin used for chip select. This is required.
     * @param speedMHz The SPI bus speed in MHz. Default is 4.
     * @return SC16IS7xxInterface& 
     * 
     * The SC16IS752 supports SPI speeds up to 4 Mbit/sec. The SC16IS762 supports SPI speeds up to 15 Mbit/sec.
     */
    SC16IS7x2 &withSPI(SPIClass *spi, pin_t csPin, size_t speedMHz = 4) { SC16IS7xxInterface::withSPI(spi, csPin, speedMHz); return *this; };

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

    /**
     * @brief Call callback for each port
     * 
     * For the SC16IS7x2 there are two ports
     */
    void forEachPort(std::function<void(SC16IS7xxPort *port)> callback);

protected:
    /**
     * @brief This class is not copyable
     */
    SC16IS7x2(const SC16IS7x2&) = delete;

    /**
     * @brief This class is not copyable
     */
    SC16IS7x2& operator=(const SC16IS7x2&) = delete;


    SC16IS7xxPort ports[2]; //!< The port objects. The SC16IS7x2 has the port as member variables but the SC16IS7x0 derives from port since there is only one.
};


#endif // __SC16IS7XXRK
