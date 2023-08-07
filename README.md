# SC16IS7xxRK

*I2C and SPI UART driver for Particle devices*

From the NXP datasheet:

> The SC16IS740/750/760 is a follower I²C-bus/SPI interface to a single-channel high performance UART. It offers data rates up to 5 Mbit/s and guarantees low operating and sleeping current. The SC16IS750 and SC16IS760 also provide the application with 8 additional programmable I/O pins. The device comes in very small HVQFN24, TSSOP24 (SC16IS750/760) and TSSOP16 (SC16IS740) packages, which makes it ideally suitable for handheld, battery operated applications. This family of products enables seamless protocol conversion from I²C-bus or SPI to and RS-232/RS-485 and are fully bidirectional.

This library also supports the SC16IS752 and SC16IS762 dual UART.

#### Chip features

| Chip      | Ports | IrDA       | GPIO  | SPI Max |
| :-------- | :---: | :--------: | :---: |:------- |
| SC16IS740 | 1     |            |       | 4 Mbps  |
| SC16IS750 | 1     | 115.2 Kbps | 8     | 4 Mbps  |
| SC16IS760 | 1     | 1.152 Mbps | 8     | 15 Mbps | 
| SC16IS752 | 2     | 115.2 Kbps |       | 4 Mbps  |
| SC16IS762 | 2     | 1.152 Mbps |       | 15 Mbps |


#### Chip packages

| Chip         | TSSOP16 | HVQFN24 | TSSOP24 | TSSOP28 | HVQFN32 |
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

