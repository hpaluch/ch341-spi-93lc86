// ch341_spi_93lc86.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "hpch_93c.h"

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
			printf("\n0x%04x",i);
		}
		printf(" %02x",buf[i]);
	}
	printf("\n");
}

BOOL MyWriteAndReadBack(ULONG iDevIndex,DWORD addr, DWORD data){
	DWORD outData = 0;

	// test write
	if (!HpCh_93c_Write(iDevIndex,addr,data)){
		return FALSE;
	}

	// read back value
	if (!HpCh_93c_Read(iDevIndex,addr,&outData)){
		return FALSE;
	}
	if (outData != data){
		fprintf(stderr,"Written data mismatch at addr 0x%x: 0x%x <> 0x%x\n", addr, data,outData);
		return FALSE;
	}
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	const DWORD MY_TEST_ADDR = 1024;
	const DWORD MY_TEST_DATA = 0x34;
	const DWORD MY_TEST_DATA2 = 0xAB;

	DWORD outData = 0;
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

	// write and read back sample data at sample address
	if (!MyWriteAndReadBack(iDevIndex,MY_TEST_ADDR,MY_TEST_DATA)){
			ret = EXIT_FAILURE;
			goto exit1;		
	}
	// write and read back another value - to verify that data were really WRITTEN
	if (!MyWriteAndReadBack(iDevIndex,MY_TEST_ADDR,MY_TEST_DATA2)){
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

