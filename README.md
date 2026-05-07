# :package: MCP23017 HAL Driver Library
HAL Driver Library for the Microchip MCP23017 I/O Expander.

This driver has been developed to be hardware-independent.
This way, the functions present in the `core` can be used as-is on any other microcontroller.
The only functions that need to be adapted are those contained in the `platform` folder, as these are the ones that interface with and depend on the hardware.
Currently, only the `ESP32` implementation is available.

# :warning: Warning
In the datasheet Revision D (June 2022), GPA7 & GPB7 are mentioned as outputs only for MCP23017. But, the register bits do allow for the direction of these I/O pins to be changed to inputs. However, the SDA signal can be corrupted during a reading of these bits if the pin voltage changes during the transmission of the bit. It has also been reported that this SDA corruption can cause some bus hosts to malfunction. In conclusion, GPA7 & GPB7 should only be used as outputs to avoid future issues.

# :bulb: Examples
To get started with the driver, check out the `examples` folder for practical usage references.
This folder contains the following files for each available microcontroller:
- `mcp23017_basic.c`
- `mcp23017_polling.c`
- `mcp23017_interrupt.c`

# :books: References

- [MCP23017 Datasheet](https://ww1.microchip.com/downloads/en/devicedoc/20001952c.pdf)
- [GPA7 & GPB7 Cannot Be Used as Inputs](https://support.microchip.com/s/article/GPA7---GPB7-Cannot-Be-Used-as-Inputs-In-MCP23017)