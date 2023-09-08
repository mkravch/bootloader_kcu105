/******************************************************************************
*
* Copyright (C) 2012 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/******************************************************************************
*
* @file xspi_numonyx_flash_quad_example.c
*
* This file contains a design example using the SPI driver (XSpi) and axi_qspi
* device with a Numonyx quad serial flash device in the interrupt mode.
* This example erases a Sector, writes to a Page within the Sector, reads back
* from that Page and compares the data.
*
* This example  has been tested with an N25Q128 device on KC705 and ZC770
* board. The bytes per page (PAGE_SIZE) in N25Q128 is 256.
*
* @note
*
* None.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a bss  08/08/12 First release
* 3.04a bss  02/11/13 Modified to use ScuGic in case of Zynq (CR#683510)
* 4.2   ms   01/23/17 Added xil_printf statement in main function to
*                     ensure that "Successfully ran" and "Failed" strings
*                     are available in all examples. This is a fix for
*                     CR-965028.
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xparameters.h"	/* EDK generated parameters */
#include "xspi.h"		/* SPI device driver */
#include "xil_exception.h"

#ifdef XPAR_INTC_0_DEVICE_ID
#include "xintc.h"
#include <stdio.h>
#else  /* SCU GIC */
#include "xscugic.h"
#include "xil_printf.h"
#endif

#include "xspi_numonyx_flash_quad_example.h"
#include "xparameters.h"	/* EDK generated parameters */

/************************** Variable Definitions *****************************/

/*
 * The instances to support the device drivers are global such that they
 * are initialized to zero each time the program runs. They could be local
 * but should at least be static so they are zeroed.
 */
XSpi spi;
INTC InterruptController;
//XIntc Intc;
/*
 * The following variables are shared between non-interrupt processing and
 * interrupt processing such that they must be global.
 */
volatile static int TransferInProgress;

/*
 * The following variable tracks any errors that occur during interrupt
 * processing.
 */
static int ErrorCount;

/*
 * Buffers used during read and write transactions.
 */
static u8 ReadBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES + 4];
static u8 WriteBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES];

/*
 * Byte offset value written to Flash. This needs to be redefined for writing
 * different patterns of data to the Flash device.
 */
static u8 TestByte = 0x20;

void wait_on_power_up(int pause_val)
{
	int i;
	//xil_printf("Waiting peripheral start up:");
	for (i=0; i<pause_val; i++)
	{
		if(i%3==0)
		{
			xil_printf("%c", 0x2F);// "/"
			xil_printf("%c", 0x08);// backspace
		}
		else if(i%3==1)
		{
			xil_printf("%c", 0x7C);// "|"
			xil_printf("%c", 0x08);// backspace
		}
		else
		{
			xil_printf("%c", 0x5C);// "\"
			xil_printf("%c", 0x08);// backspace
		}
	}
	xil_printf("\n\r");
}


int SpiMicronFlashWriteToExtAddrReg(XSpi *SpiPtr, u8 Register)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = INTEL_COMMAND_EXTADDR_WRITE;
	WriteBuffer[BYTE2] = Register;

	/*
	 * Initiate the Transfer.
	 */
//	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				STATUS_WRITE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	//while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}
/************************** Function Definitions *****************************/

int Spi_init(void)
{
	int Status;
	u32 Index;
	u32 Address;
	XSpi_Config *ConfigPtr;	/* Pointer to Configuration data */

	/*
	 * Initialize the SPI driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h.
	 */

	xil_printf("Spi numonyx flash quad Example\r\n");
	ConfigPtr = XSpi_LookupConfig(SPI_DEVICE_ID);
	if (ConfigPtr == NULL) {
		return XST_DEVICE_NOT_FOUND;
	}

	xil_printf("XSpi_CfgInitialize\r\n");
	Status = XSpi_CfgInitialize(&spi, ConfigPtr,
				  ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}
/*****************************************************************************/
/**
*
* Main function to run the quad flash example.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int Spi_config(XIntc * Intc)
{
	int Status;
	u32 Index;
	u32 Address;
	XSpi_Config *ConfigPtr;	/* Pointer to Configuration data */

	//XIntc Intc;

	xil_printf("XSpi_SetStatusHandler\r\n");
	XSpi_SetStatusHandler(&spi, &spi, (XSpi_StatusHandler)SpiHandler);




	SetupInterruptSystemSPI(Intc);



	/*
	 * Set the SPI device as a master and in manual slave select mode such
	 * that the slave select signal does not toggle for every byte of a
	 * transfer, this must be done before the slave select is set.
	 */
	xil_printf("XSpi_SetOptions\r\n");
	Status = XSpi_SetOptions(&spi, XSP_MASTER_OPTION |
				 XSP_MANUAL_SSELECT_OPTION);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Select the quad flash device on the SPI bus, so that it can be
	 * read and written using the SPI bus.
	 */
	xil_printf("XSpi_SetSlaveSelect\r\n");
	Status = XSpi_SetSlaveSelect(&spi, SPI_SELECT);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the SPI driver so that interrupts and the device are enabled.
	 */
	xil_printf("XSpi_Start\r\n");
	Status = XSpi_Start(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	return XST_SUCCESS;
}

XStatus XSpi_Example()
{
	XStatus Status;
	u32 Address;
	u32 Index, i;
	char tmp_data[PAGE_SIZE+4], tmp_data_rd[PAGE_SIZE+4];

	for(i=0;i<PAGE_SIZE;i++)
		tmp_data[i] = i;

	/*
	 * Specify address in the Quad Serial Flash for the Erase/Write/Read
	 * operations.
	 */

	xil_printf("Spi numonyx flash quad Example\r\n");


	Address = FLASH_TEST_ADDRESS;

	/*
	 * Perform the Write Enable operation.
	 */
	xil_printf("SpiFlashWriteEnable\r\n");
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform the Sector Erase operation.
	 */
	xil_printf("SpiFlashSectorErase\r\n");
	Status = SpiFlashSectorErase(&spi, Address);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Perform the Write Enable operation.
	 */
	xil_printf("SpiFlashWriteEnable\r\n");
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Write the data to the Page using Page Program command.
	 */
	xil_printf("SpiFlashWrite COMMAND_PAGE_PROGRAM\r\n");
	Status = SpiFlashWrite(&spi, Address, PAGE_SIZE, COMMAND_PAGE_PROGRAM, tmp_data);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Clear the read Buffer.
	 */
	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES; Index++) {
		ReadBuffer[Index] = 0x0;
	}

	/*
	 * Read the data from the Page using Random Read command.
	 */
	xil_printf("SpiFlashRead COMMAND_RANDOM_READ\r\n");
	Status = SpiFlashRead(&spi, Address, PAGE_SIZE, COMMAND_RANDOM_READ, tmp_data_rd);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Compare the data read against the data written.
	 */
	xil_printf("Compare the data read against the data written\r\n");
	for(Index = 0; Index < PAGE_SIZE; Index++) {
		if((u8)tmp_data[Index] !=
					(u8)tmp_data_rd[Index])
		{
			return XST_FAILURE;
		}

		//xil_printf("%02x %02x\n\r", tmp_data[Index], tmp_data_rd[Index]);
	}

	xil_printf("Successfully ran Spi numonyx flash quad Example\r\n");
	return XST_SUCCESS;

//
//	/*
//	 * Clear the Read Buffer.
//	 */
//	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES +
//	    DUAL_READ_DUMMY_BYTES; Index++) {
//		ReadBuffer[Index] = 0x0;
//	}
//
//
//	/*
//	 * Read the data from the Page using Dual Output Fast Read command.
//	 */
//	xil_printf("SpiFlashRead COMMAND_DUAL_READ\r\n");
//	Status = SpiFlashRead(&spi, Address, PAGE_SIZE, COMMAND_DUAL_READ);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	/*
//	 * Compare the data read against the data written.
//	 */
//	for(Index = 0; Index < PAGE_SIZE; Index++) {
//		if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES +
//				DUAL_READ_DUMMY_BYTES] !=
//					(u8)(Index + TestByte)) {
//			return XST_FAILURE;
//		}
//	}
	//
//	/*
//	 * Clear the read Buffer.
//	 */
//	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES +
//			  QUAD_READ_DUMMY_BYTES; Index++) {
//		ReadBuffer[Index] = 0x0;
//	}

//	/*
//	 * Read the data from the Page using Quad Output Fast Read command.
//	 */
//	xil_printf("SpiFlashRead COMMAND_QUAD_READ\r\n");
//	Status = SpiFlashRead(&Spi, Address, PAGE_SIZE, COMMAND_QUAD_READ);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

//	/*
//     	 * Compare the data read against the data written.
//	 */
//	for(Index = 0; Index < PAGE_SIZE; Index++) {
//		if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES +
//			QUAD_READ_DUMMY_BYTES] != (u8)(Index + TestByte)) {
//				return XST_FAILURE;
//			}
//	}
//
//	/*
// 	 * Perform the Write Enable operation.
// 	 */
//	xil_printf("SpiFlashWriteEnable\r\n");
//	Status = SpiFlashWriteEnable(&Spi);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

//	/*
//	 * Write the data to the next Page using Quad Fast Write command. Erase
//	 * is not required since we are writing to next page with in the same
//	 * erased sector
//	 */
//	TestByte = 0x09;
//	Address += PAGE_SIZE;
//
//	xil_printf("SpiFlashWrite COMMAND_QUAD_WRITE\r\n");
//	Status = SpiFlashWrite(&Spi, Address, PAGE_SIZE, COMMAND_QUAD_WRITE);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	/*
//	 * Wait while the Flash is busy.
//	 */
//	xil_printf("SpiFlashWaitForFlashReady\r\n");
//	Status = SpiFlashWaitForFlashReady();
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	/*
//	 * Clear the read Buffer.
//	 */
//	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES;
//		Index++) {
//		ReadBuffer[Index] = 0x0;
//	}
//
//	/*
//	 * Read the data from the Page using Normal Read command.
//	 */
//	xil_printf("SpiFlashRead COMMAND_RANDOM_READ\r\n");
//	Status = SpiFlashRead(&Spi, Address, PAGE_SIZE, COMMAND_RANDOM_READ);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	/*
//	 * Compare the data read against the data written.
//	 */
//	for(Index = 0; Index < PAGE_SIZE; Index++) {
//		if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES] !=
//					(u8)(Index + TestByte)) {
//			return XST_FAILURE;
//		}
//	}
//
//	/*
//	 * Clear the read Buffer.
//	 */
//	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES +
//	    DUAL_IO_READ_DUMMY_BYTES; Index++) {
//		ReadBuffer[Index] = 0x0;
//	}
//
//
//	/*
//	 * Read the data from the Page using Dual IO Fast Read command.
//	 */
//	xil_printf("SpiFlashRead COMMAND_DUAL_IO_READ\r\n");
//	Status = SpiFlashRead(&Spi, Address, PAGE_SIZE, COMMAND_DUAL_IO_READ);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
//
//	/*
//	 * Compare the data read against the data written.
//	 */
//	for(Index = 0; Index < PAGE_SIZE; Index++) {
//		if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES +
//				DUAL_IO_READ_DUMMY_BYTES] !=
//					(u8)(Index + TestByte)) {
//			return XST_FAILURE;
//		}
//	}
//
//	/*
//	 * Clear the read Buffer.
//	 */
//	for(Index = 0; Index < PAGE_SIZE + READ_WRITE_EXTRA_BYTES +
//	    QUAD_IO_READ_DUMMY_BYTES; Index++) {
//		ReadBuffer[Index] = 0x0;
//	}
//
//	/*
//	 * Read the data from the Page using Quad IO Fast Read command.
//	 */
//	xil_printf("SpiFlashRead COMMAND_QUAD_IO_READ\r\n");
//	Status = SpiFlashRead(&Spi, Address, PAGE_SIZE, COMMAND_QUAD_IO_READ);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

//	/*
//	 * Compare the data read against the data written.
//	 */
//	for(Index = 0; Index < PAGE_SIZE; Index++) {
//		if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES +
//				QUAD_IO_READ_DUMMY_BYTES] !=
//					(u8)(Index + TestByte)) {
//			return XST_FAILURE;
//		}
//	}

//	xil_printf("Successfully ran Spi numonyx flash quad Example\r\n");
//	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables writes to the Numonyx Serial Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWriteEnable(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_WRITE_ENABLE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				WRITE_ENABLE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function writes the data to the specified locations in the Numonyx Serial
* Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address in the Buffer, where to write the data.
* @param	ByteCount is the number of bytes to be written.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWrite(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 WriteCmd, char* data)
{
	u32 Index;
	int Status;

	for(Index=0;Index<ByteCount;Index++)
	{
		WriteBuffer[Index+READ_WRITE_EXTRA_BYTES] = data[Index];
	}

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = WriteCmd;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;


//	/*
//	 * Fill in the TEST data that is to be written into the Numonyx Serial
//	 * Flash device.
//	 */
//	for(Index = 4; Index < ByteCount + READ_WRITE_EXTRA_BYTES; Index++) {
//		WriteBuffer[Index] = (u8)((Index - 4) + TestByte);
//	}

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction.
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the data from the Numonyx Serial Flash Memory
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the starting address in the Flash Memory from which the
*		data is to be read.
* @param	ByteCount is the number of bytes to be read.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount, u8 ReadCmd, char* data)
{
	int Status, i;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = ReadCmd;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;

	if (ReadCmd == COMMAND_DUAL_READ) {
		ByteCount += DUAL_READ_DUMMY_BYTES;
	} else if (ReadCmd == COMMAND_DUAL_IO_READ) {
		ByteCount += DUAL_READ_DUMMY_BYTES;
	} else if (ReadCmd == COMMAND_QUAD_IO_READ) {
		ByteCount += QUAD_IO_READ_DUMMY_BYTES;
	} else if (ReadCmd==COMMAND_QUAD_READ) {
		ByteCount += QUAD_READ_DUMMY_BYTES;
	}

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer( SpiPtr, WriteBuffer, ReadBuffer,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction.
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	for(i=0;i<ByteCount;i++)
	{
		data[i] = ReadBuffer[i+READ_WRITE_EXTRA_BYTES];
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the entire contents of the Numonyx Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will read as 0xFF.
*
******************************************************************************/
int SpiFlashBulkErase(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_BULK_ERASE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
						BULK_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the contents of the specified Sector in the Numonyx
* Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address within a sector of the Buffer, which is to
*		be erased.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will be read back as 0xFF.
*
******************************************************************************/
int SpiFlashSectorErase(XSpi *SpiPtr, u32 Addr)
{
	int Status;

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_SECTOR_ERASE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) (Addr);

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
					SECTOR_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the Status register of the Numonyx Flash.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The status register content is stored at the second byte
*		pointed by the ReadBuffer.
*
******************************************************************************/
int SpiFlashGetStatus(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_STATUSREG_READ;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						STATUS_READ_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}
/*****************************************************************************/
/**
*
* This function writes to the Status register of the Intel Flash.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	StatusRegister is the value to be written to the status register
* 		of the flash device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The status register content is stored at the second byte pointed
*		by the ReadPtr.
*
******************************************************************************/
int SpiIntelFlashWriteStatus(XSpi *SpiPtr, u8 StatusRegister)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_STATUSREG_WRITE;
	WriteBuffer[BYTE2] = StatusRegister;

	/*
	 * Initiate the Transfer.
	 */
//	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				STATUS_WRITE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	//while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


/*****************************************************************************/
/**
*
* This function waits till the Numonyx serial Flash is ready to accept next
* command.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		This function reads the status register of the Buffer and waits
*.		till the WIP bit of the status register becomes 0.
*
******************************************************************************/
int SpiFlashWaitForFlashReady(void)
{
	int Status;
	u8 StatusReg;
	u8 StatusReg_dbg;

	u8 ReadBuffer_dbg[PAGE_SIZE + READ_WRITE_EXTRA_BYTES + 4];

	while(1) {

		/*
		 * Get the Status Register. The status register content is
		 * stored at the second byte pointed by the ReadBuffer.
		 */


		Status = SpiFlashGetStatus(&spi);
		if(Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		/*
		 * Check if the flash is ready to accept the next command.
		 * If so break.
		 */


		StatusReg = ReadBuffer[1];

		//ReadBuffer_dbg = ReadBuffer;

		strcpy (ReadBuffer_dbg, ReadBuffer);

		StatusReg_dbg = StatusReg;

		//wait_on_power_up(10000);

		if((StatusReg & FLASH_SR_IS_READY_MASK) == 0) {
			break;
		}
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the handler which performs processing for the SPI driver.
* It is called from an interrupt context such that the amount of processing
* performed should be minimized. It is called when a transfer of SPI data
* completes or an error occurs.
*
* This handler provides an example of how to handle SPI interrupts and
* is application specific.
*
* @param	CallBackRef is the upper layer callback reference passed back
*		when the callback function is invoked.
* @param	StatusEvent is the event that just occurred.
* @param	ByteCount is the number of bytes transferred up until the event
*		occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void SpiHandler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount)
{
	/*
	 * Indicate the transfer on the SPI bus is no longer in progress
	 * regardless of the status event.
	 */
	TransferInProgress = FALSE;

	/*
	 * If the event was not transfer done, then track it as an error.
	 */
	if (StatusEvent != XST_SPI_TRANSFER_DONE) {
		ErrorCount++;
	}
	//print("@");
}

/*****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the Spi device. This function is application specific since the actual
* system may or may not have an interrupt controller. The Spi device could be
* directly connected to a processor without an interrupt controller.  The
* user should modify this function to fit the application.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SetupInterruptSystemSPI(XIntc * pInterruptController)
{


	int Status;
//
//	/*
//	 * Initialize the interrupt controller driver so that
//	 * it's ready to use, specify the device ID that is generated in
//	 * xparameters.h
//	 */



//	Status = XIntc_Initialize(pInterruptController, INTC_DEVICE_ID);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler
	 * performs the specific interrupt processing for the device
	 */
	print("XIntc_Connect\n\r");
	Status = XIntc_Connect(pInterruptController,
				SPI_INTR_ID,
				(XInterruptHandler)XSpi_InterruptHandler,//XSpi_InterruptHandler,
				(void *)&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

//	/*
//	 * Start the interrupt controller such that interrupts are enabled for
//	 * all devices that cause interrupts, specific real mode so that
//	 * the SPI can cause interrupts thru the interrupt controller.
//	 */
//	Status = XIntc_Start(pInterruptController, XIN_REAL_MODE);
//	if(Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}

	/*
	 * Enable the interrupt for the SPI.
	 */
	print("XIntc_Enable\n\r");
	XIntc_Enable(pInterruptController, SPI_INTR_ID);



//	/*
//	 * Initialize the exception table.
//	 */
	Xil_ExceptionInit();
//
	/*
//	 * Register the interrupt controller handler with the exception table.
//	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)INTC_HANDLER,
				&InterruptController);

	/*
//	 * Enable non-critical exceptions.
//	 */
	Xil_ExceptionEnable();
//
	return XST_SUCCESS;
}

XStatus Write2Flash(char* addr_in_mem, u32 addr_in_flash, u32 size)
{
	XStatus Status;

	if(addr_in_flash>0x00FFFFFF)
		ChangeFlashBank(1);
	else
		ChangeFlashBank(0);
	/*
	 * Perform the Write Enable operation.
	 */
	//xil_printf("SpiFlashWriteEnable\r\n");
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform the Sector Erase operation.
	 */
	//xil_printf("SpiFlashSectorErase\r\n");
	Status = SpiFlashSectorErase(&spi, (u32)addr_in_flash);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Perform the Write Enable operation.
	 */
	//xil_printf("SpiFlashWriteEnable\r\n");
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Write the data to the Page using Page Program command.
	 */
	//xil_printf("SpiFlashWrite COMMAND_PAGE_PROGRAM\r\n");
	Status = SpiFlashWrite(&spi, (u32)addr_in_flash, size, COMMAND_PAGE_PROGRAM, addr_in_mem);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

XStatus Write2FlashNoErase(char* addr_in_mem, u32 addr_in_flash, u32 size)
{
	XStatus Status;

	if(addr_in_flash>0x00FFFFFF)
		ChangeFlashBank(1);
	else
		ChangeFlashBank(0);
	/*
	 * Perform the Write Enable operation.
	 */
	//xil_printf("SpiFlashWriteEnable\r\n");
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Write the data to the Page using Page Program command.
	 */
	//xil_printf("SpiFlashWrite COMMAND_PAGE_PROGRAM\r\n");
	Status = SpiFlashWrite(&spi, (u32)addr_in_flash, size, COMMAND_PAGE_PROGRAM, addr_in_mem);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait while the Flash is busy.
	 */
	Status = SpiFlashWaitForFlashReady();
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


XStatus	ReadFromFlash(char* addr_in_mem, u32 addr_in_flash, u32 size)
{
	XStatus Status;
	//change bank if needed
	//print("F");

	if(addr_in_flash>0x00FFFFFF)
		ChangeFlashBank(1);
	else
		ChangeFlashBank(0);
	/*
	 * Read the data from the Page using Random Read command.
	 */
	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	//xil_printf("SpiFlashRead COMMAND_RANDOM_READ\r\n");
	return SpiFlashRead(&spi, (u32)addr_in_flash, size, COMMAND_RANDOM_READ, addr_in_mem);

}

XStatus EraseSector(u32 addr_in_mem)
{
	XStatus Status;

	//change bank if needed
	if(addr_in_mem>0x00FFFFFF)
		ChangeFlashBank(1);
	else
		ChangeFlashBank(0);

	Status = SpiFlashWriteEnable(&spi);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	return SpiFlashSectorErase(&spi, (u32)addr_in_mem);
}

void EraseBulk()
{
	XStatus Status;
	SpiFlashWriteEnable(&spi);
	Status = SpiFlashBulkErase(&spi);
	if(Status) print("SpiFlashBulkErase ret. err\n\r");
}


// This function is only for flash with size more than 128Mb
// bank_num=0|1
void ChangeFlashBank(int bank_num)

{
	int Status = 0;
	int static last_bank = 0;
	if(last_bank != bank_num)
	{
		xil_printf("bank=%d\n\r", bank_num);
		Status |= SpiFlashWaitForFlashReady();
		Status |= SpiIntelFlashWriteStatus(&spi, INTEL_DISABLE_PROTECTION_ALL);
		Status |= SpiFlashWaitForFlashReady();
		Status |= SpiFlashWriteEnable(&spi);
		Status |= SpiFlashWaitForFlashReady();
		Status |= SpiMicronFlashWriteToExtAddrReg(&spi, 0xFF*bank_num);
		Status |= SpiFlashWaitForFlashReady();
		last_bank = bank_num;
	}
}
