# SPI example for 93LC86 EEPROM with CH341 USB adapter 

Here is project of accessing 93LC86 EEPROM
from [CH341A USB to UART/IIC/SPI/TTL/ISP adapter EPP/MEM Parallel converter]
using SPI mode.

NOTE: Microchip cleverly omits word `SPI` from its data sheet and
rather uses `Industry Standard 3-Wire Serial I/O` phrase. But
the device resembles `SPI` with 3 notable exceptions:

* `CS` pin is active on HIGH  (typical SPI has `/SS` - slave select
  active in Low)
* after each command the `CS` pin must be deactivated and activated
  again - otherwise following commands will be ignored
* after any programming command the `DO` (or `MISO`) pin 
  is `READY/BUSY` pin which can be pooled (even without clock).
  However `CS` may not be deactived before this pin comes
  to `READY` state (otherwise this pin function is lost).


> WARNING!
>
> This project is work in progress.


Circuit schematic is below:

![Schematic of SPI w 93LC86](https://github.com/hpaluch/ch341-spi-93lc86/blob/master/ExpressPCB/ch341-spi-93lc86.png?raw=true)


# Requirements

Hardware:
* you
  need [CH341A USB to UART/IIC/SPI/TTL/ISP adapter EPP/MEM Parallel converter]
  I got my from Amazon.de as [DollaTek CH341A USB zu UART/IIC/SPI/TTL/ISP Adapter EPP/MEM Parallelwandler]. If you never used this module
  please see my article [Getting started with LC CH341A USB conversion module]
* EEPROM 93LC86, 100nF ceramic capacitor.

Software:

* Windows OS - I tested this project on Windows XP SP3 guest in VirtualBox
* Visual Studio 2010 (This it the last version supported on XP SP3)


# Setup

The CH341A adapter must be setup following way:
* jumper set to `I2C/SPI` mode
* voltage set to 5V TTL logic (this is because used CD4013 D Flip-Flop
  would be near by required minimum voltage in case of 3.3V and too slow)
* please see picture below for correct configuration:

![USB CH341A adapter configuration](https://github.com/hpaluch/ch341-spi-93lc86//blob/master/images/ch341-spi-5v.jpg?raw=true)


Software setup:
*  Download and install [CH341PAR.ZIP] - USB driver for CH341 chip
   in Parallel mode (EPP, MEM). This driver is valid 
   also for **I2C mode and SPI mode** (yes - even when it is marked parallel).
*  install VisualSutdio 2010

Create environment variable `CH341_SDK` that should point to extracted
`CH341PAR.ZIP` header and library files. For example
if you have extracted file:

```
C:\CH341_DRIVER\LIB\C\CH341DLL.H 
```
Then your `CH341_SDK` should be set to `C:\CH341_DRIVER\LIB\C`.

Open and rebuild solution `VS2010_sol/ch341_spi_93lc86/ch341_spi_93lc86.sln`
in VisualStudio 2010. There should be no errors.

Connect your `CH341A USB module` to target circuit. Following pins are used:

|PIN Name|Direction|Description|
|--------|---------|-----------|
|GND|N/A|Common ground|
|VCC|N/A|5V supply|
|MISO|Input|master in slave out - SPI|
|MOSI|Output|master out slave in - SPI|
|SCK|Output|master clock - SPI|
|CS0|Output|Chip select 0, active in high (93LC86 NOT SPI compatible)|

----

NOTE: Direction is from `CH341A USB Module` "view".

Connect your `CH341 USB module` to your PC. There should
be permanently lighting red LED on USB module.

TODO: example program is work in progress

# Bitstream mode

The `ch341dll.h` API offers two interfaces for SPI:

* byte oriented `CH341SetStream()` + `CH341StreamSPI4()` called
  for each byte. This is convenient API, but
  may not be flexible enough in complex scenarios (like this EEPROM)
* bit-stream oriented `CH341Set_D5_D0()` + `CH341BitStreamSPI()` called
  for each bit-set. Each byte represents (roughly) set of bits
  `D7` to `D0` that have following meaning:

|Bit|Direction|Description|
|---|---------|-----------|
|D7|In|MISO - master in slave out data|
|D6|In|SPI 5-wire pin?|
|D5|Out|MOSI - master out slave in data|
|D4|Out|SPI 5-wire pin?|
|D3|Out|Clock (automatic?)|
|D2|Out|CS2|
|D1|Out|CS1|
|D0|Out|CS0|

WARNING: I'm unable to find reliable documentation on SPI 5-wire
standard (common is SPI 4-wire).

WARNING: I did not verify above table (yet).


# Output

TODO

[CH341PAR.ZIP]: http://www.wch.cn/downloads/file/7.html
[Getting started with LC CH341A USB conversion module]:  https://github.com/hpaluch/hpaluch.github.io/wiki/Getting-started-with-LC-CH341A-USB-conversion-module
[CH341A USB to UART/IIC/SPI/TTL/ISP adapter EPP/MEM Parallel converter]:http://www.chinalctech.com/index.php?_m=mod_product&_a=view&p_id=1220
[DollaTek CH341A USB zu UART/IIC/SPI/TTL/ISP Adapter EPP/MEM Parallelwandler]:https://www.amazon.de/gp/product/B07DJZDRKG/

