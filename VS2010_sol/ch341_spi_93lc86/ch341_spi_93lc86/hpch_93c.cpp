// hpch_93.cpp - Henryk Paluch's  routines for 93LC86 EEPROM connected using CH341A adapter 
#include"stdafx.h"
#include"hpch_93c.h"


// from: https://stackoverflow.com/a/41862592
/* Windows sleep in 100ns units */
BOOLEAN nanosleep(LONGLONG ns){
    /* Declarations */
    HANDLE timer;   /* Timer handle */
    LARGE_INTEGER li;   /* Time defintion */
    /* Create timer */
    if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
        return FALSE;
    /* Set timer properties */
    li.QuadPart = -ns;
    if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
        CloseHandle(timer);
        return FALSE;
    }
    /* Start & wait for timer */
    WaitForSingleObject(timer, INFINITE);
    /* Clean resources */
    CloseHandle(timer);
    /* Slept without problems */
    return TRUE;
}
//
// Send 93LC86 command and returns response
// DWORD iIndex - CH341A device SN, or 0 for 1st device
// DWORD inData - input data bits
// DWORD nBits  - number of bits to send in inData
// DWORD csBits - number of CS off bits to send (can be 0 to leave CS ON)
// DWORD outData - data bits returned from 93LC86 - only nBits inserted...
static BOOL HpCh_93c_SendCommand(ULONG iIndex, DWORD inData,DWORD nBits, DWORD csBits,DWORD *outData){
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
	// typically 1 CS OFF bit is enough
	if (!HpCh_93c_SendCommand(iIndex, cmd,(ADDR_BITS+DATA_BITS+PREFIX_BITS), 1, outData)){
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

BOOL HpCh_93c_Ewen(ULONG iIndex){
	BOOL  ret = FALSE;
	DWORD outData = 0;
	// 1|00|11 where last 2-bit are taken from ADDRESS
	// = 0x13
	DWORD cmd = 0x13 << ( ADDR_BITS - 2);
	// NOTE: otuData is ignored - there is no meaningful feedback from 93LC86 on this command
	ret =  HpCh_93c_SendCommand(iIndex, cmd,(ADDR_BITS+PREFIX_BITS), 1, &outData);
	if (!ret){
		fprintf(stderr,"%s failed\n",__FUNCTION__);
	}
	return ret;
}

BOOL HpCh_93c_Ewds(ULONG iIndex){
	BOOL  ret = FALSE;
	DWORD outData = 0;
	// 1|00|00 where last 2-bit are taken from ADDRESS
	// = 0x10
	DWORD cmd = 0x10 << ( ADDR_BITS - 2);
	// NOTE: otuData is ignored - there is no meaningful feedback from 93LC86 on this command
	ret =  HpCh_93c_SendCommand(iIndex, cmd,(ADDR_BITS+PREFIX_BITS), 1, &outData);
	if (!ret){
		fprintf(stderr,"%s failed\n",__FUNCTION__);
	}
	return ret;
}

BOOL HpCh_93c_Write(ULONG iIndex,DWORD addr,DWORD inData){
	BOOL  ret = FALSE;
	DWORD outData = 0;
	DWORD cmd = 0;
	int i = 0;

	if ( addr & ~ ADDR_MASK ){
		fprintf(stderr,"addr 0x%x is too large. Maximum allowed is: 0x%x\n",addr, ADDR_MASK);
		goto exit0;
	}

	if ( inData & ~ DATA_MASK ){
		fprintf(stderr,"inData value 0x%x is too large. Maximum allowed is: 0x%x\n",inData, DATA_MASK);
		goto exit0;
	}

	// enable write to EEPROM
	if (!HpCh_93c_Ewen(iIndex)){
		goto exit0;
	}

	// WRITE command is 1|01| => 0x5
	cmd = 0x5 << ( ADDR_BITS + DATA_BITS ); 
	cmd |= (addr << DATA_BITS);
	cmd |= inData;

	// NOTE: csBits must be 0 to keep MISO READ/BUSY status function!!!
	if (!HpCh_93c_SendCommand(iIndex, cmd,(ADDR_BITS+DATA_BITS+PREFIX_BITS), 0, &outData)){
		goto exit1; // try to disable writes first...
	}

	// now we need to poll for results
	for(int i=0;i< HPCH_93C_WRITE_CMD_TIMEOUT_MS;i++){
		cmd = 0;
		outData = 0;
		nanosleep( 1 * 1000 * 1000 ); // 1ms
		if (!HpCh_93c_SendCommand(iIndex, cmd, 1, 0, &outData)){
			goto exit1; // try to disable writes first...
		}
		if (outData &1){
			printf("DEBUG: WRITE succeed after %u ms wait\n",i+1);
			ret = TRUE;
			break;
		}
	}
	if (!ret){
		fprintf(stderr,"ERROR: Timeout %u ms after WRITE\n",i);
		// try to lower CS and disable WRITes....
		cmd = 0; outData = 0;
		HpCh_93c_SendCommand(iIndex, cmd, 1, 1, &outData);
		goto exit1; // just for reference...
	}

	// try to disable write on any circumstances
exit1:
	if (!HpCh_93c_Ewds(iIndex)){
		ret = FALSE;
	}
exit0:
	if (!ret){
		fprintf(stderr,"%s failed\n",__FUNCTION__);
	}
	return ret;
}
