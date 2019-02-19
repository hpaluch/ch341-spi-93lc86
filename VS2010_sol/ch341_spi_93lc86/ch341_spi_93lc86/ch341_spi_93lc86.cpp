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

static void DumpByteArray(BYTE *b, int n){
	int i=0;
	for(int i=0;i<n;i++){
		printf("Index=%d, Value=0x%x\r\n",i,(unsigned)b[i]);
	}
}

#define READ_CMD_BITS 22

static BYTE cmdBuf[READ_CMD_BITS+1] = {0};

#define PREFIX_BITS 3
#define ADDR_BITS 11
#define DATA_BITS  8

static void PrepareReadCommand(){
	DWORD cmdBits = READ_CMD_BITS;
	DWORD addr = 3;

	DWORD cmd = 0x6; // READ command is 1|10|
	cmd <<= ADDR_BITS;
	cmd |=  addr;
	cmd <<= DATA_BITS;

	for(int i=0;i<READ_CMD_BITS;i++){
		BYTE b = 0;
		BOOL isBitSet = ( cmd & (1 << (READ_CMD_BITS-i-1))) ? TRUE : FALSE;

		b |= HPCH_MASK_CS0; // activate CS0
		if (isBitSet){
			b |= HPCH_MASK_MOSI;
		}
		cmdBuf[i] = b;
	}
	cmdBuf[READ_CMD_BITS] = 0; // hack de-activate CS0 at the end...
}


int _tmain(int argc, _TCHAR* argv[])
{
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

	while(1){
		BYTE bufCs0 = HPCH_MASK_CS0;

		PrepareReadCommand();
		printf("Sending Bit-Stream:\r\n");
		DumpByteArray(cmdBuf,READ_CMD_BITS+1);
		if (!CH341BitStreamSPI(iDevIndex,READ_CMD_BITS+1,cmdBuf)){
			printf("CH341BitStreamSPI failed\r\n");
			ret = EXIT_FAILURE;
			goto exit1;
		}
		printf("SPI returned back data:\r\n");
		DumpByteArray(cmdBuf,READ_CMD_BITS+1);

		Sleep(5);

		/*
		if (!CH341SetDelaymS(iDevIndex,5)){
			printf("CH34SetDelaymS failed\r\n");
			ret = EXIT_FAILURE;
			goto exit1;
		}
		printf("SPI CS0 buf:\r\n");
		DumpByteArray(&bufCs0,1);
		if (!CH341BitStreamSPI(iDevIndex,1,&bufCs0)){
			printf("CH341BitStreamSPI failed\r\n");
			ret = EXIT_FAILURE;
			goto exit1;
		}
		printf("SPI CS0 returned buf:\r\n");
		DumpByteArray(&bufCs0,1);
		if (!CH341SetDelaymS(iDevIndex,2)){
			printf("CH34SetDelaymS failed\r\n");
			ret = EXIT_FAILURE;
			goto exit1;
		}
		*/
		//break; // comment it out to have stream of data - for Logic Analyzer
	}

exit1:
	CH341CloseDevice(iDevIndex);
exit0:
	return ret;

}

