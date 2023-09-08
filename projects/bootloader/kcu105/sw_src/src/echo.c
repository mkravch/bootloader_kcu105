/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
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

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

#include "command_processing.h"
#include "xspi_numonyx_flash_quad_example.h"

int parse_num_page(char *data);
int parse_num_page_for_write(char *data, int *num_size);
XStatus	ReadFromFlash(char* addr_in_mem, u32 addr_in_flash, u32 size);

int transfer_data() {
	return 0;
}

void print_app_header()
{
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
	xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}


#define NUM_PAGES 2

u8 read_flash_flag;
int page_num = 0;

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{

	int status = 0;
	char page_data [NUM_PAGES][PAGE_SIZE];
	char read_data [PAGE_SIZE];
	u16 length = 0;
	u32 addr[NUM_PAGES];
	u8 err_erase;

	u16 num_size = 0;


	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);


	if (strncmp(p->payload,"FLASH_READ;",11) == 0)
	{

		page_num = parse_num_page(p->payload);

		xil_printf("page_num = %d\r\n",page_num);

		status = ReadFromFlash(&page_data[0], page_num*PAGE_SIZE, sizeof(page_data[0]));
		if(status != 0)
		{
			print("FLASH ERROR\n\r");
		}

		length = (u16_t)(sizeof(page_data[0]));

		xil_printf("tcp_sndbuf(tpcb) = %d\r\n",tcp_sndbuf(tpcb));

		if (tcp_sndbuf(tpcb) > length) {
			tcp_write(tpcb, page_data[0], length, 1);
		} else
			xil_printf("no space in tcp_sndbuf\n\r");
	}
	else if (strncmp(p->payload,"FLASH_WRITE;",12) == 0)
	{

		page_num = parse_num_page_for_write(p->payload,&num_size);

//		xil_printf("page_num = %d\r\n",page_num);

		for(int i=0;i<PAGE_SIZE;i++)
		{
			for (int k=0;k<NUM_PAGES;k++)
			{
				page_data[k][i]=*(u8*)(p->payload + 12 + k*PAGE_SIZE + num_size + i);
			}

		}


		for (int k=0;k<NUM_PAGES;k++)
		{
			addr[k] = (page_num*NUM_PAGES + k)*PAGE_SIZE;
		}

		if(addr[0]%SIZE_SECTOR == 0)
		{
			err_erase = EraseSector(addr[0]);
			xil_printf("addr = %x\r\n", addr[0]);
			if (err_erase != 0)
				xil_printf("err_erase = %d\n", err_erase);

		}

		for (int k=0;k<NUM_PAGES;k++)
		{
			status = Write2FlashNoErase((char*)&page_data[k], addr[k], PAGE_SIZE);

			if(status != 0)
			{
				print("FLASH ERROR\n\r");
			}

			status = ReadFromFlash(&read_data, addr[k], sizeof(read_data));
			if(status != 0)
			{
				print("FLASH ERROR\n\r");
			}

			for(int i=0;i<PAGE_SIZE;i++)
			{
				if (page_data[k][i] != read_data[i])
					xil_printf("error in %d %d\r\n",k,i);
			}
		}

//		xil_printf("num_size = %d;\r\n",num_size);

		if (tcp_sndbuf(tpcb) > length) {
			tcp_write(tpcb, "OK;", 3, 1);
		} else
			xil_printf("no space in tcp_sndbuf\n\r");

	}
	else if (strncmp(p->payload,"SECTOR_ERASE;",13) == 0)
	{

		erase_and_check(0x00000000);
		erase_and_check(0x00010000);
		erase_and_check(0x00020000);
		erase_and_check(0x00030000);
		erase_and_check(0x00040000);

		tcp_write(tpcb, "OK;", 3, 1);

	}


	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

void erase_and_check (u32 addr)
{

	u8 status = 0;

	char read_data [PAGE_SIZE];

	EraseSector(addr);

	xil_printf("addr = %x\r\n", addr);

	status = ReadFromFlash(&read_data, addr, sizeof(read_data));

	for (int i=0;i<10;i++)
	{
		xil_printf("data %x\r\n",read_data[i]);
	}
}


err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);
	//tcp_sent(newpcb, send_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	/* increment for subsequent accepted connections */
	connection++;

	return ERR_OK;
}


int start_application()
{
	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 7;

	/* create new TCP PCB structure */
	pcb = tcp_new();
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ADDR_ANY, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", port);

	return 0;
}
