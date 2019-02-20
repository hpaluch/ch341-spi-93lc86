// ch341_spi_93lc86.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

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


// max total bits in bit-stream = bits in DWORD (this program limitation)
#define HPCH_93C_MAX_BITS 32

//
// Send 93LC86 command and returns response
// DWORD iIndex - CH341A device SN, or 0 for 1st device
// DWORD inData - input data bits
// DWORD nBits  - number of bits to send in inData
// DWORD csBits - number of CS off bits to send (can be 0 to leave CS ON)
// DWORD outData - data bits returned from 93LC86 - only nBits inserted...
BOOL HpCh_93c_SendCommand(ULONG iIndex, DWORD inData,DWORD nBits, DWORD csBits,DWORD *outData){
	BOOL ret = FALSE;
	int i=0;
	static BYTE ioBuf[HPCH_93C_MAX_BITS];

	if (outData == NULL ){
		fprintf(stderr,"Parameter outData may not be NULL\n");
		return FALSE;
	}

	if (nBits + csBits > HPCH_93C_MAX_BITS){
		fprintf(stderr,"nBits %u + csBits %u = %u exceeds limit %u\n",nBits,csBits,nBits+csBits, HPCH_93C_MAX_BITS);
		return FALSE;
	}
	// ensure consistent data in buffer
	memset(ioBuf,0,sizeof(ioBuf));

	for(i=0;i<(int)nBits;i++){
		BYTE b = 0;
		BOOL isBitSet = ( inData & (1 << (nBits-i-1))) ? TRUE : FALSE;

		b |= HPCH_MASK_CS0; // activate CS0
		if (isBitSet){
			b |= HPCH_MASK_MOSI; // MOSI data output if bit is set
		}
		ioBuf[i] = b;
	}

#if 0
	// these data are alrady off
	for(i=0;i<csBits;i++){
		ioBuf[i+nBits] = 0; // just for reference - CS off, MOSI off
	}
#endif
	if (!CH341BitStreamSPI(iIndex,nBits+csBits,ioBuf)){
		fprintf(stderr,"CH341BitStreamSPI() failed\n");
		return FALSE;
	}

	// now transfer MISO in Bits back to *outData - only nBits transferred
	*outData = 0;
	for(i=0;i<(int)nBits;i++){
		*outData <<= 1;
		BYTE b = ioBuf[i];
		if ( b & HPCH_MASK_MISO ){
			*outData |= 1;
		}
	}
	return TRUE;
}

BOOL HpCh_93c_Read(ULONG iIndex,DWORD addr,DWORD *outData){
	DWORD cmd = 0;

	if (outData == NULL ){
		fprintf(stderr,"Parameter outData may not be NULL\n");
		return FALSE;
	}
	// READ command is 1|10| => 0x6
	cmd = 0x6 << ( ADDR_BITS + DATA_BITS ); 
	if ( addr & ~ ADDR_MASK ){
		fprintf(stderr,"addr 0x%x is too large. Maximum allowed is: 0x%x\n",addr, ADDR_MASK);
		return FALSE;
	}
	cmd |= (addr << DATA_BITS);
	// we use 2 CS off bits to padd data to 2-bit for USB adapter
	if (!HpCh_93c_SendCommand(iIndex, cmd,(ADDR_BITS+DATA_BITS+PREFIX_BITS), 2, outData)){
		return FALSE;
	}
	// verify that A0 MISO is 0
	if ( *outData & ( 1 << DATA_BITS) ){
		fprintf(stderr,"MISO response on A0 address bit is not 0\n");
		return FALSE;
	}
	*outData &= DATA_MASK; // cut off all non-data bits from input

	return TRUE;
}

void HpCh_DumpBuf(BYTE *buf, int n){
	const int VALUES_PER_LINE = 16;

	int i=0;
	printf("Dump of buffer at 0x%p,  bytes %u\n",buf,n);
	for(i=0;i<n;i++){
		if ( i % VALUES_PER_LINE == 0){
			// dump also ASCII values
			if (i >= VALUES_PER_LINE ){
				int j=0;
				// XXX: puts(3) always append \n
				putc(' ',stdout);
				for(j=0;j<VALUES_PER_LINE;j++){
					BYTE b = buf[i-VALUES_PER_LINE+j];
					if (b>=32 && b<127){
						putc(b,stdout);
					} else {
						putc('.',stdout);
					}
				}				
			}
			printf("\n%03x",i);
		}
		printf(" %02x",buf[i]);
	}
	printf("\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	int addr = 0;
	BYTE dataBuf[ADDR_MASK+1];
	int ret = EXIT_SUCCESS;
	HANDLE h341 = NULL;
	ULONG iDevIndex = 0; // first device

	printf("CH341 SPI shift register example\r\n");

	printf("CH341 version: %lu\r\n", CH341GetVersion( ));

	printf("Opening device# %lu\r\n", iDevIndex);
	h341 = CH341OpenDevice( iDevIndex);
	if (h341 == NULL){
		printf("OpenDevice(iDevIndex=%lu) failed\r\n",iDevIndex);
		ret = EXIT_FAILURE;
		goto exit0;
	}

	// set CS0, CLK and MOSI as output
	// all pins active on high
	if (!CH341Set_D5_D0(iDevIndex,HPCH_MASK_CS0|HPCH_MASK_CLK|HPCH_MASK_MOSI,0 )){
			printf("CH341_D5_D0 failed\r\n");
			ret = EXIT_FAILURE;
			goto exit1;
	}

	// the while is here only to have repeated output to Logic Analyzer
	while(1){
		memset(dataBuf,0,sizeof(dataBuf));
		printf("Reading %u bytes from 93LC86...\n",sizeof(dataBuf));
		for(addr=0; addr<=ADDR_MASK;addr++){
			DWORD b = 0;
			if (!HpCh_93c_Read(iDevIndex,addr,&b)){
				ret = EXIT_FAILURE;
				goto exit1;
			}
			// currently we expect 8-bit data (organization of 93LC86)
			if (b & ~0xff){
				fprintf(stderr,"Got too large value %u, maximum byte %u expected\n",b,255);
				ret = EXIT_FAILURE;
				goto exit1;
			}
			dataBuf[addr] = (BYTE)b;
		}
		printf("Done. Data dump follows:\n");
		HpCh_DumpBuf(dataBuf,sizeof(dataBuf));
		Sleep(5);
		break; // comment it out to have stream of data - for Logic Analyzer
	}

exit1:
	CH341CloseDevice(iDevIndex);
exit0:
	return ret;

}

