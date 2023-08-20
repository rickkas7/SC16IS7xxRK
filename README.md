# SC16IS7xxRK

*I2C and SPI UART driver for Particle devices*

From the NXP datasheet:

> The SC16IS740/750/760 is a follower I²C-bus/SPI interface to a single-channel high performance UART. It offers data rates up to 5 Mbit/s and guarantees low operating and sleeping current. The SC16IS750 and SC16IS760 also provide the application with 8 additional programmable I/O pins. The device comes in very small HVQFN24, TSSOP24 (SC16IS750/760) and TSSOP16 (SC16IS740) packages, which makes it ideally suitable for handheld, battery operated applications. This family of products enables seamless protocol conversion from I²C-bus or SPI to and RS-232/RS-485 and are fully bidirectional.

This library also supports the SC16IS752 and SC16IS762 dual UART.

- Github repository: [https://github.com/rickkas7/SC16IS7xxRK](https://github.com/rickkas7/SC16IS7xxRK) 
- [Browseable API docs](https://rickkas7.github.io/SC16IS7xxRK/index.html)
- License: MIT (can be used in open or closed source projects, including commercial products)

#### Chip features

| SC16IS7xx | Ports | IrDA       | GPIO  | SPI Max |
| :-------- | :---: | :--------: | :---: |:------- |
| SC16IS740 | 1     |            |       | 4 Mbps  |
| SC16IS750 | 1     | 115.2 Kbps | 8     | 4 Mbps  |
| SC16IS760 | 1     | 1.152 Mbps | 8     | 15 Mbps | 
| SC16IS752 | 2     | 115.2 Kbps |       | 4 Mbps  |
| SC16IS762 | 2     | 1.152 Mbps |       | 15 Mbps |

- IrDA and GPIO not currently supported by this library.

#### Chip packages

| SC16IS7xx    | TSSOP16 | HVQFN24 | TSSOP24 | TSSOP28 | HVQFN32 |
| :----------- | :-----: | :-----: | :-----: | :-----: | :-----: |
| SC16IS740IPW | x       |         |         |         |         |
| SC16IS750IBS |         | x       |         |         |         |
| SC16IS760IBS |         | x       |         |         |         |
| SC16IS750IPW |         |         | x       |         |         |
| SC16IS760IPW |         |         | x       |         |         |
| SC16IS752IPW |         |         |         | x       |         |
| SC16IS762IPW |         |         |         | x       |         |
| SC16IS752IBS |         |         |         |         | x       |
| SC16IS762IBS |         |         |         |         | x       |


## Serial connections and flow control

The SC16IS7xx supports both hardware and software flow control, though this library only supports optional hardware flow control.

| SC16IS7xx | Direction | Description | 
| :-------- | :-------: | :--- |
| TX        | Output    | UART transmitter output. Connect to other side's RX. |
| RX        | Input     | UART receiver input. Connecvt to other sides' TX. |
| /CTS      | Input     | UART clear to send (active LOW). Other side is ready to receive data when LOW. |
| /RTS      | Output    | UART request to send (active LOW). The SC16IS7xx can receive data when LOW. |
| /DSR      |           | Data set ready. Optional flow control pin manually controlled as GPIO. |
| /DTR      |           | Data terminal ready. Optional flow control pin manually controlled as GPIO. |
| /CD       |           | Carrier detect. Optional flow control pin manually controlled as GPIO. |
| /RI       |           | Ring indictor. Optional flow control pin manually controlled as GPIO. |

Both the RX/TX pair and CTS/RTS pair are typically crossed between the two sides of the UART serial link. In other words, 
RX on one side connects to TX on the other and vice-versa.

The last four flow control signals are rarely used. The library does not currently support setting and reading these pins.

Note that the serial outputs are 3.3V and the inputs must be 3.3V or 5V. If you are connecting to RS-232 or RS-485 you need the appropriate driver chip to shift the levels. 

Automatic hardware flow control (CTS/RTS) is optional and can be enabled on a per-port basis.

## Serial settings

### Baud rates

The chip requires an external crystal, which is typically either 1.8432 MHz or 3.072 MHz. Both are supported by the library:

| Baud Rate | 1.8432 MHz | 3.072 MHz |
| :--- | :---: | :---: |
| 50 | x | x |
| 75 | x | x |
| 110 | 0.026 | 0.026 |
| 134.5 | 0.058 | 0.034 |
| 150 | x | x |
| 300 | x | x |
| 600 | x | x |
| 1200 | x | x |
| 1800 | x | 0.312 |
| 2000 | 0.069 | x |
| 2400 | x | x |
| 3600 | x | 0.628 |
| 4800 | x | x |
| 7200 | x | 1.23 |
| 9600 | x | x |
| 19200 | x | x |
| 38400 | x | x |
| 56000 | 2.86 |  |
| 115200 | x |  |

- A number in the table above indicates the percentage deviation from the baud rate. Because of the large deviation, 56000 baud is not recommended for use with a 1.8432 MHz crystal.
- An empty space in the table above indicates that there is no divisor that can produce this baud rate with that crystal.

When using higher baud rates, using buffered read mode is recommended. At 115200 baud approximately 11,520 bytes per second can be transmitted. Since the FIFO can be serviced 1000 times per second, with a read of up to 64 bytes, this is still within the capability of both SPI and I2C. SPI at 4 Mbit/sec. is recommended, however I2C will work. Using I2C in 400 Kbit/sec mode is recommended over the default of 100 Kbit/sec., however all I2C devices must be able to support 400 Kbit/sec. mode in order to use it.

### Word lengths

The chip and library support 5, 6, 7, and 8 bit word lengths.

### Stop bits

The chip and library support 1 or 2 stop bits at 6, 7, or 8 bit word lengths. At 5 bit word length, 1 or 1 1/2 stop bits are supported.

### Parity

The chip and library support no parity, even, odd, forced 0, and forced 1 parity.

## Buffered read mode

The chip has a 64-byte FIFO. In some situations, you may not be able to read the data out of the FIFO fast enough from the main application loop thread if you have other blocking operations. To solve this problem, buffered read mode can be enabled.

When using buffered read, a worker thread is used to read the chip FIFO. Since the worker thread gets CPU time 1000 times per second, it can easily read data up to the maximum baud rate 

## Interrupts

This library optionally can use the hardware interrupt (IRQ) feature of the SC16IS7xx. Use is optional but not as useful as you'd think. In particular, it will not make data transfer faster or have lower latency!

- It is not possible to start an I2C or SPI transaction from an ISR. However the chip must be queried to to determine which interrupt triggered.

- Since you can only start transactions from a thread, interrupts don't reduce the latency any more than polling.

- Interrupts do have a small benefit in that the thread would not have to query the chip on every loop, 1000 times per second. This is especially true when using I2C and you are infrequently transferring data.

- Received data interrupts are only used in buffered read mode. In normal mode, the chip is queried on every read anyway, and interrupts would have no benefit.


## Usage overview

The examples show how to use most of the library features, however a detailed description is provided below.

### Declare an object

For the SC16IS740, SC16IS750, or SC16IS760, instantiate an object for the chip. This is typically done as a global object, but can be allocated on the heap using `new`. The constructor does very little and is safe as a global object.

```cpp
SC16IS7x0 extSerial;
```

For the SC16IS752 and SC16IS762, declare:

```cpp
SC16IS7x2 extSerial;
```

The object is not a singleton because you can have multiple chips. Using the A0 and A1, up to 16 chips can be added to an I2C bus. The number of chips connected to a single GPIO is limited only by the number of available GPIO as each chip must have a unique CS line.

### Set SPI or I2C mode

Typically from global `setup()` you set the options for the chip.


#### SPI 

To use SPI, you specify which SPI port (typically `SPI`), the CS pin, and the speed in MHz. The speed is optional, and defaults to 4 MHz.

```cpp
// SPI port, CS line
extSerial.withSPI(&SPI, D4, 4);
```

The SC16IS760 and SC16IS762 can run at SPI bus speeds up to 15 MHz but all other chips are limited to 4 MHz. Unlike SPI, each device on the SPI bus can run at a different speed and when an SPI transaction begins, the speed, bit order, and mode are set.

#### I2C

To use I2C mode, use `withI2C()` specifying which I2C interface (typically `Wire`) and the index or I2C address:

```cpp
extSerial.withI2C(&Wire, 0);
```

| A1 | A0 | Index | I2C Address |
| :---: | :---: | :---: | :--- |
| VDD | VDD |  0 |    0x48 |
| VDD | GND |  1 |    0x49 |
| VDD | SCL |  2 |    0x4a |
| VDD | SDA |  3 |    0x4b |
| GND | VDD |  4 |    0x4c |
| GND | GND |  5 |    0x4d |
| GND | SCL |  6 |    0x4e |
| GND | SDA |  7 |    0x4f |
| SCL | VDD |  8 |    0x50 |
| SCL | GND |  9 |    0x51 |
| SCL | SCL | 10 |    0x52 |
| SCL | SDA | 11 |    0x53 |
| SDA | VDD | 12 |    0x54 |
| SDA | GND | 13 |    0x55 |
| SDA | SCL | 14 |    0x56 |
| SDA | SDA | 15 |    0x57 |

When using I2C, it's recommended that you use 400 KHz mode, with a caveat: With I2C, every device must be able to support 400 KHz mode in order to enable it. Unlike SPI, all chips must support the higher data rate and it is enabled globally for all devices.

```cpp
Wire.setSpeed(CLOCK_SPEED_400KHZ);
```

### Set other options

#### withOscillatorFrequency()

The default is 1843200 (1.8432 MHz). If you are using a 3.072 MHz crystal, set the oscillator frequency to 3072000. This is necessary to the baud rate is correctly set.

```cpp
extSerial.withOscillatorFrequency(3072000);
```

The setting made be set before calling `begin()`.

#### withIRQ

If you want to use hardware IRQ mode, use `withIRQ`. 

```cpp
extSerial.withIRQ(D3); 
```

Using hardware IRQ is optional, and does *not* reduce latency or make the data transfer faster! 

IRQ mode must be enabled before buffered read mode, and before calling `begin()`.

#### softwareReset

This does a software reset of the chip. Since hardware reset or `System.reset()` does not reset the chip, using `softwareReset()` during `setup()` is a good practice.

```cpp
extSerial.softwareReset();
```

This is typically called at startup before calling `begin()`. 

#### powerOnCheck

This call is optional but you may want to call it after calling `softwareReset()`, especially during development.

It examines several registers to make sure they're in the expected state. If you do not use `softwareReset` the chip may be incorrectly flagged as failing powerOnCheck because the registers have already been programmed to a non-default state.

```cpp
extSerial.powerOnCheck();
```

### Per-port settings

The SC16IS740, SC16IS750, and SC16IS760 are single-port devices and the `SC16IS7x0` object is derived from `SC16IS7xxPort` so all per-port options can be used with your `extSerial` object directly.

```cpp
extSerial.begin(9600);
```

Since the SC16IS752 and SC16IS762 have two ports, you need to specify which port. You can use `a()` and `b()` to access port A or port B:

```cpp
extSerial.a().begin(9600);
extSerial.b().begin(9600);
```

Alternatively, you can use the `[]` operator. This is useful if you want to specify which port from a variable.

```cpp
extSerial[0].begin(9600);
extSerial[1].begin(9600);
```

Additionally, even though it's not required, on the single-port chips you can use `a()` or `[0]` to access the single port so your code can be the same for the SC16IS7x0 and SC16IS7x2.

```cpp
// Optional, but works on the SC16IS7x0 for consistency with the SC16IS7x2.
extSerial.a().begin(9600);
extSerial[0].begin(9600);
```

#### withBufferedRead

Buffered read mode uses a thread to read the chip, reducing the likelihood of FIFO overrun. The parameter is the size of the buffer
to allocate on the heap.
 
```cpp
extSerial.withBufferedRead(1024);     // SC16IS7x0 only
extSerial.a().withBufferedRead(1024); // SC16IS7x0 or SC16IS7x2
extSerial.b().withBufferedRead(1024); // SC16IS7x2 only
```

The setting made be set before calling `begin()`. This setting is per-port


#### begin

Finally you must call `begin()`. This sets the baud rate, stop bits, parity, and optionally enables hardware flow control. The default is 8N1 (8 bits, no parity, one stop bit), and flow control disabled.

```cpp
extSerial.begin(9600);     // SC16IS7x0 only
extSerial.a().begin(9600); // SC16IS7x0 or SC16IS7x2
extSerial.b().begin(9600); // SC16IS7x2 only
```

You can call begin more than once if you want to change the baud rate. The FIFOs are cleared when you call begin.

Available baud rates depend on your oscillator, but with a 1.8432 MHz oscillator, the following are supported:
50, 75, 110, 134.5, 150, 300, 600, 1200, 1800, 2000, 2400, 3600, 4800, 7200, 9600, 19200, 38400, 57600, 115200

The valid options in standard number of bits; none=N, even=E, odd=O; number of stop bits format:
OPTIONS_8N1, OPTIONS_8E1, OPTIONS_8O1, 
OPTIONS_8N2, OPTIONS_8E2, OPTIONS_8O2, 
OPTIONS_7N1, OPTIONS_7E1, OPTIONS_7O1, 
OPTIONS_7N2, OPTIONS_7E2, OPTIONS_7O2

```cpp
extSerial.begin(9600, SC16IS7xxPort::OPTIONS_7E1);
```

Unlike the Device OS options, the SC16IS7xx OPTIONS_8N1 value is not 0! If you omit are enabling
hardware flow control be sure to set it like:

```cpp
extSerial.begin(9600, SC16IS7xxPort::OPTIONS_8N1 | SC16IS7xxPort::OPTIONS_FLOW_CONTROL_RTS_CTS);
```

If you leave off the `OPTIONS_8N1` the output will be 5N1, not 8N1!

