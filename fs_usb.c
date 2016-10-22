/*
 * @brief USB Mass Storage Chan FATFS simple abstraction layer
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "fsusb_cfg.h"
#include "board.h"
#include "chip.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Disk Status */
static volatile DSTATUS Stat[2] = {STA_NOINIT, STA_NOINIT};

/* 100Hz decrement timer stopped at zero (disk_timerproc()) */
static volatile WORD Timer2;

static DISK_HANDLE_T *hDisk[2];

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize Disk Drive */
DSTATUS disk_initialize(BYTE drv)
{
	if (drv > 1) {
		return STA_NOINIT;				/* Supports only single drive */
	}
	/*	if (Stat & STA_NODISK) return Stat;	*//* No card in the socket */

	if (Stat[drv] != STA_NOINIT) {
		return Stat[drv];					/* card is already enumerated */

	}

	#if !_FS_READONLY
//	FSUSB_InitRealTimeClock();
	#endif

	/* Initialize the Card Data Strucutre */
	hDisk[drv] = FSUSB_DiskInit(drv);

	/* Reset */
	Stat[drv] = STA_NOINIT;

	FSUSB_DiskInsertWait(hDisk[drv]); /* Wait for card to be inserted */

	/* Enumerate the card once detected. Note this function may block for a little while. */

	if (!FSUSB_DiskAcquire(hDisk[drv])) {
		DEBUGOUT("Disk Enumeration failed...\r\n");
		return Stat[drv];
	}

	Stat[drv] &= ~STA_NOINIT;
	return Stat[drv];

}

/* Disk Drive miscellaneous Functions */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
	DRESULT res;

	if (drv > 1) {
		return RES_PARERR;
	}
	if (Stat[drv] & STA_NOINIT) {
		return RES_NOTRDY;
	}

	res = RES_ERROR;

	switch (ctrl) {
	case CTRL_SYNC:	/* Make sure that no pending write process */
		if (FSUSB_DiskReadyWait(hDisk[drv], 50)) {
			res = RES_OK;
		}
		break;

	case GET_SECTOR_COUNT:	/* Get number of sectors on the disk (DWORD) */
		*(DWORD *) buff = FSUSB_DiskGetSectorCnt(hDisk[drv]);
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:	/* Get R/W sector size (WORD) */
		*(WORD *) buff = FSUSB_DiskGetSectorSz(hDisk[drv]);
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:/* Get erase block size in unit of sector (DWORD) */
		*(DWORD *) buff = FSUSB_DiskGetBlockSz(hDisk[drv]);
		res = RES_OK;
		break;

	default:
		res = RES_PARERR;
		break;
	}

	return res;
}

/* Read Sector(s) */
DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
	if ((drv > 1) || !count) {
		return RES_PARERR;
	}
	if (Stat[drv] & STA_NOINIT) {
		return RES_NOTRDY;
	}

	if (FSUSB_DiskReadSectors(hDisk[drv], buff, sector, count)) {
		return RES_OK;
	}

	return RES_ERROR;
}

/* Get Disk Status */
DSTATUS disk_status(BYTE drv)
{
	if (drv > 1) {
		return STA_NOINIT;	/* Supports only single drive */
	}
	return Stat[drv];
}

/* Write Sector(s) */
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{

	if ((drv>1) || !count) {
		return RES_PARERR;
	}
	if (Stat[drv] & STA_NOINIT) {
		return RES_NOTRDY;
	}

	if (FSUSB_DiskWriteSectors(hDisk[drv], (void *) buff, sector, count)) {
		return RES_OK;
	}

	return RES_ERROR;
}
