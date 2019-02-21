#ifndef HPCH_93C_H
#define HPCH_93C_H
// hpch_93c.h -  Henryk Paluch's  routines for 93LC86 EEPROM connected using CH341A adapter 

// bit numbers for D7-D0 in bit-stream modes
#define HPCH_BIT_CS0  0
#define HPCH_BIT_CS1  1
#define HPCH_BIT_CS2  2
#define HPCH_BIT_CLK  3
#define HPCH_BIT_MOSI 5
#define HPCH_BIT_MISO 7

#define HPCH_MASK_CS0  (1 << HPCH_BIT_CS0)
#define HPCH_MASK_CLK  (1 << HPCH_BIT_CLK)
#define HPCH_MASK_MOSI (1 << HPCH_BIT_MOSI)
#define HPCH_MASK_MISO (1 << HPCH_BIT_MISO)


#define PREFIX_BITS 3
#define ADDR_BITS 11
#define ADDR_MASK 0x7ff
#define DATA_BITS  8
#define DATA_MASK 0xff

// WRITE command time is 5ms in data sheet - using 10ms
#define HPCH_93C_WRITE_CMD_TIMEOUT_MS 10

// max total bits in bit-stream = bits in DWORD (this program limitation)
#define HPCH_93C_MAX_BITS 32

// READ command - Read one byte from 93LC86 from address addr to *outData
// ULONG iIndex - CH341A adapter SN or 0
// DWORD addr   - EEPROM address to read data from
// DWORD *outData - pointer where to write data from EEPROM (currently only 8-bit = 1 byte)
BOOL HpCh_93c_Read(ULONG iIndex,DWORD addr,DWORD *outData);

// EWEN command - enable write operations on 93LC86 EEPROM
// ULONG iIndex - CH341A adapter SN or 0
BOOL HpCh_93c_Ewen(ULONG iIndex);

// EWDS command - disable write operations on 93LC86 EEPROM
BOOL HpCh_93c_Ewds(ULONG iIndex);

// WRITE command - writes inData to 93LC86 EEPROM
// ULONG iIndex - CH341A adapter SN or 0
// DWORD addr   - address to write data to
// DWORD inData - data to write at specified address (currently only 1 byte used)
BOOL HpCh_93c_Write(ULONG iIndex,DWORD addr,DWORD inData);


#endif