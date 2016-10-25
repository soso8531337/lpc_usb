/*
 * @brief Definitions and declarations of Mass Storage Host example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * Copyright(C) Dean Camera, 2011, 2012
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

#ifndef __USTORAGE_H_
#define __USTORAGE_H_

#include "board.h"
#include "USB.h"
#include <ctype.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USTOR_PHONE_USBADDR		0
#define USTOR_DISK_USBADDR		1

/*AOA Itunes protocol*/
#define VID_APPLE 0x5ac
#define PID_RANGE_LOW 0x1290
#define PID_RANGE_MAX 0x12af

/**********Android AOA***********/
/* Product IDs / Vendor IDs */
#define AOA_ACCESSORY_VID		0x18D1	/* Google */
#define AOA_ACCESSORY_PID		0x2D00	/* accessory */
#define AOA_ACCESSORY_ADB_PID		0x2D01	/* accessory + adb */
#define AOA_AUDIO_PID			0x2D02	/* audio */
#define AOA_AUDIO_ADB_PID		0x2D03	/* audio + adb */
#define AOA_ACCESSORY_AUDIO_PID		0x2D04	/* accessory + audio */
#define AOA_ACCESSORY_AUDIO_ADB_PID	0x2D05	/* accessory + audio + adb */
#define INTERFACE_CLASS_AOA 255 // Referrance http://www.usb.org/developers/defined_class/#BaseClassFFh
#define INTERFACE_SUBCLASS_AOA 255
/* Android Open Accessory protocol defines */
#define AOA_GET_PROTOCOL		51
#define AOA_SEND_IDENT			52
#define AOA_START_ACCESSORY		53
#define AOA_REGISTER_HID		54
#define AOA_UNREGISTER_HID		55
#define AOA_SET_HID_REPORT_DESC		56
#define AOA_SEND_HID_EVENT		57
#define AOA_AUDIO_SUPPORT		58
/* String IDs */
#define AOA_STRING_MAN_ID		0
#define AOA_STRING_MOD_ID		1
#define AOA_STRING_DSC_ID		2
#define AOA_STRING_VER_ID		3
#define AOA_STRING_URL_ID		4
#define AOA_STRING_SER_ID		5


/* UStorage return code (URESULT) */
typedef enum {
	UR_OK = 0,				/* (0) Succeeded */
	UR_MEM_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	UR_INT_ERR,				/* (2) Assertion failed */
	UR_NOT_READY,			/* (3) The physical drive cannot work */
	UR_NO_FILE,				/* (4) Could not find the file */
	UR_NO_PATH,				/* (5) Could not find the path */
	UR_INVALID_NAME,		/* (6) The path name format is invalid */
	UR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	UR_EXIST,				/* (8) Access denied due to prohibited access */
	UR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	UR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	UR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	UR_NOT_ENABLED,			/* (12) The volume has no work area */
	UR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	UR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
	UR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	UR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	UR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	UR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_SHARE */
	UR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} URESULT;		
typedef USB_ClassInfo_MS_Host_t USB_ClassInfo_UStorage_t;
/** Size of share memory that a device uses to store data transfer to/ receive from Phone
 *  or a host uses to store data transfer to/ receive from device.
 */
#define USTORAGE_BUFFER_SIZE  (4*1024)

struct accessory_t {
	uint32_t aoa_version;
	uint16_t vid;
	uint16_t pid;
	char *device;
	char *manufacturer;
	char *model;
	char *description;
	char *version;
	char *url;
	char *serial;
};
static struct accessory_t acc_default = {
	.manufacturer = "i4season",
	.model = "U-Storage",
	.description = "U-Storage",
	.version = "1.0",
	.url = "https://www.simicloud.com/download/index.html",
	.serial = "0000000012345678",
};

struct scsi_head{
	int32_t head;	/*Receive OR Send*/
	int32_t wtag; /*Task ID*/
	int32_t ctrid; /*Command ID*/
	int32_t addr; /*Offset addr*512   represent sectors */
	int32_t len;
	int16_t wlun;
	int16_t relag; /*Response Code*/
};

#define SCSI_HEAD_SIZE			sizeof(struct scsi_head)
#define STOR_PAYLOAD		1
#define SCSI_PHONE_MAGIC		 0xccddeeff
#define SCSI_DEVICE_MAGIC		 0xaabbccdd
#define SCSI_WFLAG  1 << 7
#define NXP_USBBUF			(8*1024)
#define NXP_DFT_SECTOR_SIZE		512

enum {
  SCSI_TEST = 0,
  SCSI_READ  = 1,//28
  SCSI_WRITE = 2 | SCSI_WFLAG,//2a
  SCSI_INQUIRY = 3,//12
  SCSI_READ_CAPACITY =4,//25
  SCSI_GET_LUN = 5,
  SCSI_INPUT = 6,
  SCSI_OUTPUT = 7,
};

enum{
	EREAD = 1,
	EWRITE=2,
	ENODISK = 3,
	EDISKLEN = 4,
	EDISKINFO=5
};

#ifdef __cplusplus
}
#endif

#endif /* __MASS_STORAGE_HOST_H_ */
