/*
 * UART, disk I/O and monitor firmware for EMU8088
 *
 * Based on main.c by Tetsuya Suzuki and emuz80_z80ram.c by Satoshi Okue
 * Base source code by @hanyazou https://twitter.com/hanyazou
 * Modified by Akihito Honda
 */
/*!
 * PIC18F47Q43/PIC18F47Q83/PIC18F47Q84 ROM image uploader and UART emulation firmware
 * This single source file contains all code
 *
 * Target: EMUZ80 with Z80+RAM
 * Compiler: MPLAB XC8 v2.40
 *
 * Modified by Satoshi Okue https://twitter.com/S_Okue
 * Version 0.1 2022/11/15
 */

/*
    PIC18F57Q43 ROM RAM and UART emulation firmware
    This single source file contains all code
    Original source code for PIC18F47Q43 ROM RAM and UART emulation firmware
    Designed by @hanyazou https://twitter.com/hanyazou

    Target: EMU8088 - The computer with only 8088/V20 and PIC18F57Q43
    Written by Akihito Honda
*/

#include "../src/emu88.h"
#include <stdio.h>
#include <assert.h>

#include "../fatfs/ff.h"
#include "../drivers/utils.h"

drive_t drives[] = {
    { 26 },
    { 26 },
    { 26 },
    { 26 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 128 },
    { 128 },
    { 128 },
    { 128 },
    { 0 },
    { 0 },
    { 0 },
    { 16484 },
};
const int num_drives = (sizeof(drives)/sizeof(*drives));
int do_bus_master;
int nmi_flg;

// console input buffers
#define U3B_SIZE 256
#define RETURN_TBL 2		/* bytes of return parameter */

static unsigned char rx_buf[U3B_SIZE];	//UART Rx ring buffer
static unsigned int rx_wp, rx_rp, rx_cnt;
// request table
static PTRSAV req_tbl;
// disk buffer
static uint8_t disk_buf[SECTOR_SIZE];

void io_init(void) {
	rx_wp = 0;
	rx_rp = 0;
	rx_cnt = 0;
	do_bus_master = 0;
	nmi_flg = 0;
	req_tbl.UREQ_COM = 0;
	req_tbl.CREQ_COM = 0;
}

//
// define interrupt
//
// Never called, logically
void __interrupt(irq(default),base(8)) Default_ISR(){}

////////////// UART3 Receive interrupt ////////////////////////////
//UART3 Rx interrupt
/////////////////////////////////////////////////////////////////
void __interrupt(irq(U3RX),base(8)) URT3Rx_ISR(){

	unsigned char rx_data;

	rx_data = U3RXB;			// get rx data

	if (rx_data == CTL_Q) {
		nmi_flg = 1;
	}
	else if (rx_cnt < U3B_SIZE) {
		rx_buf[rx_wp] = rx_data;
		rx_wp = (rx_wp + 1) & (U3B_SIZE - 1);
		rx_cnt++;
	}
}

// UART3 Transmit
void putch(char c) {
    while(!U3TXIF);             // Wait or Tx interrupt flag set
    U3TXB = c;                  // Write data
}

// UART3 Recive
int getch(void) {
	char c;

	while(!rx_cnt);             // Wait for Rx interrupt flag set
	GIE = 0;                // Disable interrupt
	c = rx_buf[rx_rp];
	rx_rp = (rx_rp + 1) & ( U3B_SIZE - 1);
	rx_cnt--;
	GIE = 1;                // enable interrupt
    return c;               // Read data
}

uint32_t get_physical_addr(uint16_t ah, uint16_t al)
{
	return (uint32_t)ah*0x10 + (uint32_t)al;
}

int seek_disk(void) {
	unsigned int n;
	FRESULT fres;
	FIL *filep = drives[req_tbl.disk_drive].filep;
	uint32_t sector;

//debug
//printf("seek_disk(): disk_drive(%04x), disk_sector(%04x)\r\n", disk_drive, disk_sector);

	if (drives[req_tbl.disk_drive].filep == NULL) return(-1);
	sector = req_tbl.disk_track * drives[req_tbl.disk_drive].sectors + req_tbl.disk_sector - 1;
	
	if ((fres = f_lseek(filep, sector * SECTOR_SIZE)) != FR_OK) {
		printf("f_lseek(): ERROR %d\n\r", fres);
		return(-1);
	}
	return(0);
}

int write_sector(void) {
	unsigned int n;
	FRESULT fres;
	FIL *filep = drives[req_tbl.disk_drive].filep;
	uint32_t addr;
	
	if (seek_disk()) return(-1);

	addr = get_physical_addr( req_tbl.data_dmah, req_tbl.data_dmal );

	// transfer write data from SRAM to the buffer
	read_sram(addr, disk_buf, SECTOR_SIZE);

	if (DEBUG_DISK_WRITE && DEBUG_DISK_VERBOSE && !(debug.disk_mask & (1 << req_tbl.disk_drive))) {
		util_hexdump_sum("buf: ", disk_buf, SECTOR_SIZE);
	}

	// write buffer to the DISK
	if ((fres = f_write(filep, disk_buf, SECTOR_SIZE, &n)) != FR_OK || n != SECTOR_SIZE) {
		printf("f_write(): ERROR res=%d, n=%d\n\r", fres, n);
		return(-1);
	}
	else if ((fres = f_sync(filep)) != FR_OK) {
		printf("f_sync(): ERROR %d\n\r", fres);
		return(-1);
	}
	return(0);
}

int read_sector(void) {
	unsigned int n;
	FRESULT fres;
	FIL *filep = drives[req_tbl.disk_drive].filep;
	uint32_t addr;
	
	if (seek_disk()) return(-1);

	// read from the DISK
	if ((fres = f_read(filep, disk_buf, SECTOR_SIZE, &n)) != FR_OK || n != SECTOR_SIZE) {
		printf("f_read(): ERROR res=%d, n=%d\n\r", fres, n);
		return(-1);
	}
	else if (DEBUG_DISK_READ && DEBUG_DISK_VERBOSE && !(debug.disk_mask & (1 << req_tbl.disk_drive))) {
		util_hexdump_sum("buf: ", disk_buf, SECTOR_SIZE);
	}
	else {
		// transfer read data to SRAM
		addr = get_physical_addr( req_tbl.data_dmah, req_tbl.data_dmal );
		write_sram(addr, disk_buf, SECTOR_SIZE);

		#ifdef MEM_DEBUG
		printf("f_read(): SRAM address(%08lx),data_dmah(%04x),data_dmal(%04x)\n\r",
			     addr, req_tbl.data_dmah, req_tbl.data_dmal);
		read_sram(addr, disk_buf, SECTOR_SIZE);
		util_hexdump_sum("RAM: ", disk_buf, SECTOR_SIZE);
		#endif  // MEM_DEBUG
	}
	return(0);
}

int setup_drive(void) {
	req_tbl.CBI_CHR = 0;		/* clear error status */
	if ( req_tbl.disk_drive >= num_drives ) return( -1 );
	if ( drives[req_tbl.disk_drive].sectors == 0 ) return( -1 );	// not support disk
	if ( req_tbl.disk_sector > drives[req_tbl.disk_drive].sectors ) return( -1 ); // sector no. start with 1
	return( 0 );
}

void dsk_err(void) {
	req_tbl.UNI_CHR = 1;
}

void unimon_console(void) {

	uint8_t *buf;
	uint16_t cnt;

	switch (req_tbl.UREQ_COM) {
		// CONIN
		case CONIN_REQ:
			req_tbl.UNI_CHR = (uint8_t)getch();
			break;
		// CONOUT
		case CONOUT_REQ:
			putch((char)req_tbl.UNI_CHR);		// Write data
			break;
		// CONST
		case CONST_REQ:
			req_tbl.UNI_CHR = (uint8_t)(rx_cnt !=0);
			break;
		case STROUT_REQ:
			buf = tmp_buf[0];
			cnt = (uint16_t)req_tbl.UNI_CHR;
			// get string
			read_sram(get_physical_addr(req_tbl.STR_SEG, req_tbl.STR_off), buf, cnt);
			while( cnt ) {
				putch( *buf++);
				cnt--;
			}
			break;
		case MNI_LEDOFF:
			nmi_flg = 0;
			nmi_sig_off();
			break;
		default:
			printf("UNKNOWN unimon CMD(%02x)\r\n", req_tbl.UREQ_COM);
	}
	req_tbl.UREQ_COM = 0;	// clear unimon request
}

//
// bus master handling
// this fanction is invoked at main() after HOLDA = 1
//
// bioreq_ubuffadr = top address of unimon
//
//  ---- request command to PIC
// UREQ_COM = 1 ; CONIN  : return char in UNI_CHR
//          = 2 ; CONOUT : UNI_CHR = output char
//          = 3 ; CONST  : return status in UNI_CHR
//                       : ( 0: no key, 1 : key exist )
//          = 4 ; STROUT : string address = (PTRSAV, PTRSAV_SEG)
//          = 5 ; DISK READ
//          = 6 ; DISK WRITE
//          = 7 ; NMI LED OFF
//          = 0 ; request is done( return this flag from PIC )
//                return status is in UNI_CHR;

void bus_master_operation(void) {
	uint32_t addr;
	uint8_t *buf;
	uint16_t cnt;

	// read request from 8088/V20
	read_sram(bioreq_ubuffadr, (uint8_t *)&req_tbl, (unsigned int)sizeof(PTRSAV));

	if (req_tbl.UREQ_COM) {
		unimon_console();
		// write end request to SRAM for 8088/V20
		write_sram(bioreq_ubuffadr, (uint8_t *)&req_tbl, RETURN_TBL);	// 2bytes
	}
	else {
		switch (req_tbl.CREQ_COM) {
			// CONIN
			case CONIN_REQ:
				req_tbl.CBI_CHR = (uint8_t)getch();
				break;
			// CONOUT
			case CONOUT_REQ:
				putch((char)req_tbl.CBI_CHR);		// Write data
				break;
			// CONST
			case CONST_REQ:
				req_tbl.CBI_CHR = (rx_cnt !=0) ? 255 : 0;
				break;
			case STROUT_REQ:
				buf = tmp_buf[0];
				cnt = (uint16_t)req_tbl.CBI_CHR;
				// get string
				read_sram(get_physical_addr(req_tbl.data_dmah, req_tbl.data_dmal), buf, cnt);
				while( cnt ) {
					putch( *buf++);
					cnt--;
				}
				break;
			case REQ_DREAD:
				if ( setup_drive() ) {
					dsk_err();
					break;
				}
				if ( read_sector() ) {
					dsk_err();
					break;
				}
				break;
			case REQ_DWRITE:
				if ( setup_drive() ) {
					dsk_err();
					break;
				}
				if ( write_sector() ) {
					dsk_err();
					break;
				}
				break;
			case MNI_LEDOFF:
				break;
//			default:
//				printf("UNKNOWN REQUEST : CMD(%02x)\r\n", req_tbl.CREQ_COM);
		}
		req_tbl.CREQ_COM = 0;	// clear cbios request
		// write end request to SRAM for 8088/V20
		write_sram(bioreq_cbuffadr, (uint8_t *)&req_tbl.CREQ_COM, RETURN_TBL);	// 2bytes
	}

}

