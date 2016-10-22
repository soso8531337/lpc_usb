/*
 * @brief U-Storage Project
 *
 * @note
 * Copyright(C) i4season, 2016
 * Copyright(C) Szitman, 2016
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

#include "MassStorageHost.h"
#include "fsusb_cfg.h"
#include "FreeRTOS.h"
#include "task.h"
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

enum AOA_Descriptor_ClassSubclassProtocol_t
{
	AOA_TransparentClass		  = 0xff, /**< Descriptor Class value indicating that the device or interface
											   *   belongs to the AOA.
											   */
	AOA_TransparentSubclass   = 0xff,
	AOA_TransportProtocol = 0x00, 
};
//#define INTERFACE_CLASS 255
//#define INTERFACE_SUBCLASS 254
//#define INTERFACE_PROTOCOL 2
enum IOS_Descriptor_ClassSubclassProtocol_t
{
	IOS_TransparentClass		  = 0xff, /**< Descriptor Class value indicating that the device or interface
											   *   belongs to the IOS.
											   */
	IOS_TransparentSubclass   = 0xfe,
	IOS_TransportProtocol = 0x02, 
};


/** LPCUSBlib Mass Storage Class driver interface configuration and state information. This structure is
 *  passed to all Mass Storage Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
 /** Use USB0 for Phone
 *    Use USB1 for Mass Storage
*/
static USB_ClassInfo_UStorage_t UStorage_Interface[]	= {
	{
		.Config = {
			.DataINPipeNumber       = 1,
			.DataINPipeDoubleBank   = false,

			.DataOUTPipeNumber      = 2,
			.DataOUTPipeDoubleBank  = false,
			.PortNumber = 0,
		},
	},
	{
			.Config = {
			.DataINPipeNumber       = 1,
			.DataINPipeDoubleBank   = false,

			.DataOUTPipeNumber      = 2,
			.DataOUTPipeDoubleBank  = false,
			.PortNumber = 1,
		},
	},
	
};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Function to spin forever when there is an error */
static void die(URESULT rc)
{
	DEBUGOUT("*******DIE %d*******\r\n", rc);
	while (1) {}/* Spin for ever */
}

static void UStorage_init(int port_num)
{
	if(port_num >= 2){
		die(UR_INT_ERR);
		return;
	}
#if (defined(CHIP_LPC43XX) || defined(CHIP_LPC18XX))
	if (port_num== 0){
		Chip_USB0_Init();
	} else {
		Chip_USB1_Init();
	}
#endif
	USB_Init(UStorage_Interface[port_num].Config.PortNumber, USB_MODE_Host);
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
static void SetupHardware(void)
{
#if (defined(CHIP_LPC43XX_NOSYSTEM))
	SystemCoreClockUpdate();
	Board_Init();
#endif
	UStorage_init(0);
	UStorage_init(1);
#if (defined(CHIP_LPC43XX_NOSYSTEM))
	/* Hardware Initialization */
	Board_Debug_Init();
#endif
}
/*USB for Phone*/
static void vUStoragePhoneTask(void *pvParameters)
{
	uint8_t  ErrorCode = PIPE_RWSTREAM_NoError;
	uint16_t BytesRem =24;
	struct scsi_head header;

	while(1){
		USB_ClassInfo_MS_Host_t *MSInterfaceInfo = NULL;
		if (USB_HostState[USTOR_PHONE_USBADDR] != HOST_STATE_Configured) {
			USB_USBTask(USTOR_PHONE_USBADDR, USB_MODE_Host);
			continue;
		}
		/*Have Connected Phone Read or Write*/
		/*Test*/
		MSInterfaceInfo = &UStorage_Interface[USTOR_PHONE_USBADDR];
		if ((ErrorCode = MS_Host_WaitForDataReceived(MSInterfaceInfo)) != PIPE_RWSTREAM_NoError)
		{
			Pipe_Freeze();
			DEBUGOUT("Wait Data Timeout\r\n");
			continue;
		}

		Pipe_SelectPipe(USTOR_PHONE_USBADDR,MSInterfaceInfo->Config.DataINPipeNumber);
		Pipe_Unfreeze();
		memset(&header, 0, sizeof(struct scsi_head));
		if ((ErrorCode = Pipe_Read_Stream_LE(USTOR_PHONE_USBADDR,(void*)&header, BytesRem, NULL)) != PIPE_RWSTREAM_NoError){
			DEBUGOUT("Pipe_Read_Stream_LE Failed:%d\r\n", ErrorCode);			
			Pipe_ClearIN(USTOR_PHONE_USBADDR);
			continue;
		}
		DEBUGOUT("Pipe_Read_Stream_LE Successful\r\nwtag=%d\r\nctrid=%d\r\naddr=%d\r\nlen=%d\r\nwlun=%d\r\n", 
					header.wtag, header.ctrid, header.addr, header.len, header.wlun);
		Pipe_ClearIN(USTOR_PHONE_USBADDR);
	}
}
/*USB for Storage*/
static void vUStorageDiskTask(void *pvParameters) {

}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/** Main program entry point. This routine configures the hardware required by the application, then
 *  calls the filesystem function to read files from USB Disk
 *Ustorage Project by Szitman 20161022
 */
void vs_main(void *pvParameters)
{
	int ret;
	char ubuffer[USTORAGE_BUFFER_SIZE];

	SetupHardware();

	DEBUGOUT("U-Storage Running.\r\n");
	vUStoragePhoneTask(NULL);
}

static uint8_t DCOMP_US_AOA_NextMSInterface(void* const CurrentDescriptor)
{
	USB_Descriptor_Header_t* Header = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Header_t);

	if (Header->Type == DTYPE_Interface)
	{
		USB_Descriptor_Interface_t* Interface = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Interface_t);

		if ((Interface->Class    == AOA_TransparentClass)        &&
		    (Interface->SubClass == AOA_TransparentSubclass) &&
		    (Interface->Protocol == AOA_TransportProtocol))
		{
			return DESCRIPTOR_SEARCH_Found;
		}
	}

	return DESCRIPTOR_SEARCH_NotFound;
}

static uint8_t DCOMP_US_SWITCHAOA_NextMSInterface(void* const CurrentDescriptor)
{
	USB_Descriptor_Header_t* Header = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Header_t);

	if (Header->Type == DTYPE_Interface)
	{
		USB_Descriptor_Interface_t* Interface = DESCRIPTOR_PCAST(CurrentDescriptor, USB_Descriptor_Interface_t);

		if (Interface->Class  == AOA_TransparentClass)
		{
			return DESCRIPTOR_SEARCH_Found;
		}
	}

	return DESCRIPTOR_SEARCH_NotFound;
}

uint8_t MS_Host_Ustorage_SwitchAOA(const uint8_t corenum,
                               uint16_t ConfigDescriptorSize,
							   void* ConfigDescriptorData)
{
	USB_ClassInfo_MS_Host_t *MSInterfaceInfo = &UStorage_Interface[corenum];
	uint8_t version[2] = {0};
	
	memset(&MSInterfaceInfo->State, 0x00, sizeof(MSInterfaceInfo->State));

	if (DESCRIPTOR_TYPE(ConfigDescriptorData) != DTYPE_Configuration)
		return MS_ENUMERROR_InvalidConfigDescriptor;
	/*Found Interface Class */
	if (USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
				DCOMP_US_SWITCHAOA_NextMSInterface) != DESCRIPTOR_SEARCH_COMP_Found)
	{		
		DEBUGOUT("Attached Device Not a Valid AOA Device[NO 0xff Interface].\r\n");
		return MS_ENUMERROR_NoCompatibleInterfaceFound;
	}
	/*Set AOA*/
	if(USB_APP_SendControlRequest(corenum, REQDIR_DEVICETOHOST|REQTYPE_VENDOR
				, AOA_GET_PROTOCOL, 0, 0, 2, version)){
		DEBUGOUT("Get AOA Protocol Version Failed.\r\n");
		return 4;
	}
	acc_default.aoa_version = ((version[1] << 8) | version[0]);
	DEBUGOUT("Found Device supports AOA %d.0!\r\n", acc_default.aoa_version);
	/* In case of a no_app accessory, the version must be >= 2 */
	if((acc_default.aoa_version < 2) && !acc_default.manufacturer) {
		DEBUGOUT("Connecting without an Android App only for AOA 2.0.\r\n");
		return 4;
	}
	if(acc_default.manufacturer) {
		DEBUGOUT("sending manufacturer: %s\r\n", acc_default.manufacturer);
		if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
					, AOA_SEND_IDENT, 0, AOA_STRING_MAN_ID, 
					strlen(acc_default.manufacturer) + 1, (uint8_t *)acc_default.manufacturer)){
			DEBUGOUT("Get AOA Protocol Version Failed.\r\n");
			return 4;
		}
	}
	if(acc_default.model) {
		DEBUGOUT("sending model: %s\r\n", acc_default.model);
		if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
					, AOA_SEND_IDENT, 0, AOA_STRING_MOD_ID, 
					strlen(acc_default.model) + 1, (uint8_t *)acc_default.model)){
			DEBUGOUT("Could not Set AOA model.\r\n");
			return 4;
		}
	}
	
	DEBUGOUT("sending description: %s\r\n", acc_default.description);
	if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
				, AOA_SEND_IDENT, 0, AOA_STRING_DSC_ID, 
				strlen(acc_default.description) + 1, (uint8_t *)acc_default.description)){
		DEBUGOUT("Could not Set AOA description.\r\n");
		return 4;
	}
	
	DEBUGOUT("sending version string: %s\r\n", acc_default.version);
	if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
				, AOA_SEND_IDENT, 0, AOA_STRING_VER_ID, 
				strlen(acc_default.version) + 1, (uint8_t *)acc_default.version)){
		DEBUGOUT("Could not Set AOA version.\r\n");
		return 4;
	}
	DEBUGOUT("sending url string: %s\r\n", acc_default.url);
	if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
				, AOA_SEND_IDENT, 0, AOA_STRING_URL_ID, 
				strlen(acc_default.url) + 1, (uint8_t *)acc_default.url)){
		DEBUGOUT("Could not Set AOA url.\r\n");
		return 4;
	}
	DEBUGOUT("sending serial number: %s\r\n", acc_default.serial);
	if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
				, AOA_SEND_IDENT, 0, AOA_STRING_SER_ID, 
				strlen(acc_default.serial) + 1, (uint8_t *)acc_default.serial)){
		DEBUGOUT("Could not Set AOA serial.\r\n");
		return 4;
	}

	if(USB_APP_SendControlRequest(corenum, REQDIR_HOSTTODEVICE|REQTYPE_VENDOR
				, AOA_START_ACCESSORY, 0, 0, 
				0, NULL)){
		DEBUGOUT("Could not Start AOA.\r\n");
		return 4;
	}
	
	DEBUGOUT("Start AOA Successful  Android Will Reconnect\r\n");
	return 0;
}


/*Add by Szitman 20161022*/
uint8_t MS_Host_Ustorage_ConfigurePipes(USB_ClassInfo_MS_Host_t* const MSInterfaceInfo,
                               uint16_t ConfigDescriptorSize,
							   void* ConfigDescriptorData)
{
	USB_Descriptor_Endpoint_t*  DataINEndpoint       = NULL;
	USB_Descriptor_Endpoint_t*  DataOUTEndpoint      = NULL;
	USB_Descriptor_Interface_t* MassStorageInterface = NULL;
	uint8_t portnum = MSInterfaceInfo->Config.PortNumber;


	memset(&MSInterfaceInfo->State, 0x00, sizeof(MSInterfaceInfo->State));

	if (DESCRIPTOR_TYPE(ConfigDescriptorData) != DTYPE_Configuration)
	  return MS_ENUMERROR_InvalidConfigDescriptor;

	while (!(DataINEndpoint) || !(DataOUTEndpoint))
	{
		if (!(MassStorageInterface) ||
		    USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
		                              DCOMP_MS_Host_NextMSInterfaceEndpoint) != DESCRIPTOR_SEARCH_COMP_Found)
		{
			if (USB_GetNextDescriptorComp(&ConfigDescriptorSize, &ConfigDescriptorData,
			                              DCOMP_US_AOA_NextMSInterface) != DESCRIPTOR_SEARCH_COMP_Found)
			{
				return MS_ENUMERROR_NoCompatibleInterfaceFound;
			}

			MassStorageInterface = DESCRIPTOR_PCAST(ConfigDescriptorData, USB_Descriptor_Interface_t);

			DataINEndpoint  = NULL;
			DataOUTEndpoint = NULL;

			continue;
		}

		USB_Descriptor_Endpoint_t* EndpointData = DESCRIPTOR_PCAST(ConfigDescriptorData, USB_Descriptor_Endpoint_t);

		if ((EndpointData->EndpointAddress & ENDPOINT_DIR_MASK) == ENDPOINT_DIR_IN)
		  DataINEndpoint  = EndpointData;
		else
		  DataOUTEndpoint = EndpointData;
	}

	for (uint8_t PipeNum = 1; PipeNum < PIPE_TOTAL_PIPES; PipeNum++)
	{
		uint16_t Size;
		uint8_t  Type;
		uint8_t  Token;
		uint8_t  EndpointAddress;
		bool     DoubleBanked;

		if (PipeNum == MSInterfaceInfo->Config.DataINPipeNumber)
		{
			Size            = le16_to_cpu(DataINEndpoint->EndpointSize);
			EndpointAddress = DataINEndpoint->EndpointAddress;
			Token           = PIPE_TOKEN_IN;
			Type            = EP_TYPE_BULK;
			DoubleBanked    = MSInterfaceInfo->Config.DataINPipeDoubleBank;

			MSInterfaceInfo->State.DataINPipeSize = DataINEndpoint->EndpointSize;
		}
		else if (PipeNum == MSInterfaceInfo->Config.DataOUTPipeNumber)
		{
			Size            = le16_to_cpu(DataOUTEndpoint->EndpointSize);
			EndpointAddress = DataOUTEndpoint->EndpointAddress;
			Token           = PIPE_TOKEN_OUT;
			Type            = EP_TYPE_BULK;
			DoubleBanked    = MSInterfaceInfo->Config.DataOUTPipeDoubleBank;
			
			MSInterfaceInfo->State.DataOUTPipeSize = DataOUTEndpoint->EndpointSize;
		}
		else
		{
			continue;
		}

		if (!(Pipe_ConfigurePipe(portnum,PipeNum, Type, Token, EndpointAddress, Size,
		                         DoubleBanked ? PIPE_BANK_DOUBLE : PIPE_BANK_SINGLE)))
		{
			return MS_ENUMERROR_PipeConfigurationFailed;
		}
	}

	MSInterfaceInfo->State.InterfaceNumber = MassStorageInterface->InterfaceNumber;
	MSInterfaceInfo->State.IsActive = true;

	return MS_ENUMERROR_NoError;
}

/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Host_DeviceAttached(const uint8_t corenum)
{
	DEBUGOUT(("Device Attached on port %d\r\n"), corenum);
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
void EVENT_USB_Host_DeviceUnattached(const uint8_t corenum)
{
	DEBUGOUT(("\r\nDevice Unattached on port %d\r\n"), corenum);
}

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
void EVENT_USB_Host_Disk_DeviceEnumerationComplete(const uint8_t corenum)
{
	uint16_t ConfigDescriptorSize;
	uint8_t  ConfigDescriptorData[512];

	if (USB_Host_GetDeviceConfigDescriptor(corenum, 1, &ConfigDescriptorSize, ConfigDescriptorData,
										   sizeof(ConfigDescriptorData)) != HOST_GETCONFIG_Successful) {
		DEBUGOUT("Error Retrieving Configuration Descriptor.\r\n");
		return;
	}

	UStorage_Interface[corenum].Config.PortNumber = corenum;
	if (MS_Host_ConfigurePipes(&UStorage_Interface[corenum],
							   ConfigDescriptorSize, ConfigDescriptorData) != MS_ENUMERROR_NoError) {
		DEBUGOUT("Attached Device Not a Valid Mass Storage Device.\r\n");
		return;
	}

	if (USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 1) != HOST_SENDCONTROL_Successful) {
		DEBUGOUT("Error Setting Device Configuration.\r\n");
		return;
	}

	uint8_t MaxLUNIndex;
	if (MS_Host_GetMaxLUN(&UStorage_Interface[corenum], &MaxLUNIndex)) {
		DEBUGOUT("Error retrieving max LUN index.\r\n");
		USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 0);
		return;
	}

	DEBUGOUT(("Total LUNs: %d - Using first LUN in device.\r\n"), (MaxLUNIndex + 1));

	if (MS_Host_ResetMSInterface(&UStorage_Interface[corenum])) {
		DEBUGOUT("Error resetting Mass Storage interface.\r\n");
		USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 0);
		return;
	}

	SCSI_Request_Sense_Response_t SenseData;
	if (MS_Host_RequestSense(&UStorage_Interface[corenum], 0, &SenseData) != 0) {
		DEBUGOUT("Error retrieving device sense.\r\n");
		USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 0);
		return;
	}

// 	if (MS_Host_PreventAllowMediumRemoval(&FlashDisk_MS_Interface, 0, true)) {
// 		DEBUGOUT("Error setting Prevent Device Removal bit.\r\n");
// 		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber, 0);
// 		return;
// 	}

	SCSI_Inquiry_Response_t InquiryData;
	if (MS_Host_GetInquiryData(&UStorage_Interface[corenum], 0, &InquiryData)) {
		DEBUGOUT("Error retrieving device Inquiry data.\r\n");
		USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 0);
		return;
	}

	/* DEBUGOUT("Vendor \"%.8s\", Product \"%.16s\"\r\n", InquiryData.VendorID, InquiryData.ProductID); */

	DEBUGOUT("Mass Storage Device Enumerated.\r\n");
}

void EVENT_USB_Host_Phone_DeviceEnumerationComplete(const uint8_t corenum)
{
	USB_Descriptor_Device_t DeviceDescriptorData;
	uint16_t ConfigDescriptorSize;
	uint8_t  ConfigDescriptorData[512];
	
	/*We need to get Device Description*/
	if(USB_Host_GetDeviceDescriptor(corenum, &DeviceDescriptorData)){
		DEBUGOUT("Error Getting Device Descriptor.\r\n");
		return;
	}
	
	UStorage_Interface[corenum].Config.PortNumber = corenum;
	if (USB_Host_GetDeviceConfigDescriptor(corenum, 1, &ConfigDescriptorSize, ConfigDescriptorData,
										   sizeof(ConfigDescriptorData)) != HOST_GETCONFIG_Successful) {
		DEBUGOUT("Error Retrieving Configuration Descriptor.\r\n");
		return;
	}	
	if(DeviceDescriptorData.VendorID == VID_APPLE &&
		(DeviceDescriptorData.ProductID >= PID_RANGE_LOW 
			&& DeviceDescriptorData.ProductID <= PID_RANGE_MAX)){
		/*iPhone Device*/
		DEBUGOUT("Found iPhone Device[v/p=%d:%d].\r\n", 
				DeviceDescriptorData.VendorID, DeviceDescriptorData.ProductID);
		return;
	}else if(DeviceDescriptorData.VendorID == AOA_ACCESSORY_VID &&
		(DeviceDescriptorData.ProductID >= AOA_ACCESSORY_PID 
			&& DeviceDescriptorData.ProductID <= AOA_ACCESSORY_AUDIO_ADB_PID)){
		/*Android Device*/
		DEBUGOUT("Found Android Device[v/p=%d:%d].\r\n", 
				DeviceDescriptorData.VendorID, DeviceDescriptorData.ProductID);
		if(MS_Host_Ustorage_ConfigurePipes(&UStorage_Interface[corenum],
							   ConfigDescriptorSize, ConfigDescriptorData) != MS_ENUMERROR_NoError){
			DEBUGOUT("Attached Device Not a Valid Android AOA Device.\r\n");
			return;
		}
		if (USB_Host_SetDeviceConfiguration(UStorage_Interface[corenum].Config.PortNumber, 1) != HOST_SENDCONTROL_Successful) {
			DEBUGOUT("Error Setting Device Configuration.\r\n");
			return;
		}
		/*Android AOA Device Found*/
		DEBUGOUT("Mass Storage Device Enumerated.\r\n");
		return;
	}
	/*Try Switch AOA*/
	if(MS_Host_Ustorage_SwitchAOA(corenum,
							   ConfigDescriptorSize, ConfigDescriptorData)){
		DEBUGOUT("Switch Android AOA Failed.\r\n");
	}else{
		DEBUGOUT("Switch Android AOA Successful.\r\n");
	}
}

void EVENT_USB_Host_DeviceEnumerationComplete(const uint8_t corenum)
{
	if(corenum == USTOR_DISK_USBADDR){
		EVENT_USB_Host_Disk_DeviceEnumerationComplete(corenum);
	}else if(corenum == USTOR_PHONE_USBADDR){
		EVENT_USB_Host_Phone_DeviceEnumerationComplete(corenum);
	}else{
		DEBUGOUT("Unknown USB Port %d.\r\n", corenum);
	}
}
/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t corenum, const uint8_t ErrorCode)
{
	USB_Disable(corenum, USB_MODE_Host);

	DEBUGOUT(("Host Mode Error\r\n"
			  " -- Error port %d\r\n"
			  " -- Error Code %d\r\n" ), corenum, ErrorCode);

	die(UR_OK);
}

/** Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
 *  enumerating an attached USB device.
 */
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t corenum,
											const uint8_t ErrorCode,
											const uint8_t SubErrorCode)
{
	DEBUGOUT(("Dev Enum Error\r\n"
			  " -- Error port %d\r\n"
			  " -- Error Code %d\r\n"
			  " -- Sub Error Code %d\r\n"
			  " -- In State %d\r\n" ),
			 corenum, ErrorCode, SubErrorCode, USB_HostState[corenum]);

}
