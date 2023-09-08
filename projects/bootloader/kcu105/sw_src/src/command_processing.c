#include <stdio.h>
#include "platform.h"
#include "xspi.h"		/* SPI device driver */
#include "xil_printf.h"
#include "xspi_numonyx_flash_quad_example.h"
#include "command_processing.h"
#include <stdlib.h>


#define MAX_ADDR_PAGE 512*SECTOR_SIZE*PAGE_SIZE


	int k = 0;


int parse_num_page(char *data)
{
    //char data[20] = "FLASH_READ;1000;";
    char data_tmp[20];
    int pos = 0;
    int end_pos = 0;
    int page_num = 0;

    pos = strncmp(data,";",1); //(long unsigned int)
    end_pos = strlen(data);

    strncpy(data_tmp, data + pos, end_pos - pos - 1);
    page_num = atoi(data_tmp);

    //printf("%d", page_num);

    return page_num;
}


int parse_num_page_for_write(char *data, int *num_size)
{

    //char *data = "FLASH_READ;10024;";
    char data_tmp[20];
    int page_num = 0;

    char *istr_start;
    char *istr_end;

    istr_start = strstr(data,";");

    istr_end = strstr(istr_start + 1,";");

    strncpy(data_tmp, istr_start + 1, istr_end-istr_start-1);

    page_num = atoi(data_tmp);

    *num_size = istr_end - istr_start;

    return page_num;
}
