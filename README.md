# SC16IS7xxRK

*I2C and SPI UART driver for Particle devices*

From the NXP datasheet:

> The SC16IS740/750/760 is a follower I²C-bus/SPI interface to a single-channel high performance UART. It offers data rates up to 5 Mbit/s and guarantees low operating and sleeping current. The SC16IS750 and SC16IS760 also provide the application with 8 additional programmable I/O pins. The device comes in very small HVQFN24, TSSOP24 (SC16IS750/760) and TSSOP16 (SC16IS740) packages, which makes it ideally suitable for handheld, battery operated applications. This family of products enables seamless protocol conversion from I²C-bus or SPI to and RS-232/RS-485 and are fully bidirectional.

This library also supports the SC16IS752 and SC16IS762 dual UART.

#### Chip features

| SC16IS7xx | Ports | IrDA       | GPIO  | SPI Max |
| :-------- | :---: | :--------: | :---: |:------- |
| SC16IS740 | 1     |            |       | 4 Mbps  |
| SC16IS750 | 1     | 115.2 Kbps | 8     | 4 Mbps  |
| SC16IS760 | 1     | 1.152 Mbps | 8     | 15 Mbps | 
| SC16IS752 | 2     | 115.2 Kbps |       | 4 Mbps  |
| SC16IS762 | 2     | 1.152 Mbps |       | 15 Mbps |


#### Chip packages

| SC16IS7xx    | TSSOP16 | HVQFN24 | TSSOP24 | TSSOP28 | HVQFN32 |
| :----------- | :-----: | :-----: | :-----: | :-----: | :-----: |
| SC16IS740IPW | &check; |         |         |         |         |
| SC16IS750IBS |         | &check; |         |         |         |
| SC16IS760IBS |         | &check; |         |         |         |
| SC16IS750IPW |         |         | &check; |         |         |
| SC16IS760IPW |         |         | &check; |         |         |
| SC16IS752IPW |         |         |         | &check; |         |
| SC16IS762IPW |         |         |         | &check; |         |
| SC16IS752IBS |         |         |         |         | &check; |
| SC16IS762IBS |         |         |         |         | &check; |


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

The last four flow control signals are rarely used, but if you need them, you can manually control the direction and level of those pins as GPIO. 

Note that the serial outputs are 3.3V and the inputs must be 3.3V or 5V. If you are connecting to RS-232 or RS-485 you need the appropriate driver chip to shift the levels. 

Automatic hardware flow control (CTS/RTS) can

## Interrupts

This library optionally can use the hardware interrupt (IRQ) feature of the SC16IS7xx. Use is optional and not as useful as you'd think. In particular, it will not make data transfer faster or have lower latency!

- It is not possible to start an I2C or SPI transaction from an ISR. This is necessary in order to determine which interrupt triggered.

- Since you can only start transactions from a thread, interrupts don't reduce the latency any more than polling.

- Interrupts do have a small benefit in that the thread would not have to query the chip on every loop, 1000 times per second. This is especially true when using I2C and you are infrequently transferring data.

- Received data interrupts are only used in buffered read mode. In normal mode, the chip is queried on every read anyway, and interrupts would have no benefit.


