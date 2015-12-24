/*
 * Copyright (c) 2015, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MTP_DEFS_H
#define MTP_DEFS_H

/* Taken from USB Still Image Capture Device Definition Revision 1.0 */

#define USB_CLASS_IMAGE                     0x06
#define USB_IMAGE_SUBCLASS_STILL_IMAGING    0x01

#define USB_PTP_REQ_CANCEL_REQ              0x64
#define USB_PTP_REQ_GET_EXTENDED_EVENT_DATA 0x65
#define USB_PTP_REQ_DEVICE_RESET            0x66
#define USB_PTP_REQ_GET_DEVICE_STATUS       0x67

/* Table 5.2-2 Format of Cancel Request Data */
struct usb_ptp_cancel_request {
    uint16_t code;
    uint32_t transactionID;
} __attribute__((packed));

/* Table 5.2-4 Data Format of Get Extended Event Data Request */
struct usb_ptp_extended_event_data_request_header {
    uint16_t code;
    uint32_t transactionID;
    uint16_t numParams;
    uint8_t remainder[0];
} __attribute__((packed));

/* Table 5.2-6  */
struct usb_ptp_get_device_status_request {
    uint16_t length;
    uint16_t code;
    uint8_t remainder[0];
} __attribute__((packed));

/* Table 7.1-1 Generic Container Structure */
struct usb_ptp_data_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint8_t payload[0];
} __attribute__((packed));

struct usb_ptp_command_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[0];
} __attribute__((packed));

struct usb_ptp_response_block {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[0];
} __attribute__((packed));

/* Table 7.3-1 Asynchronous Event Interrupt Data Format */
struct usb_ptp_async_event {
    uint32_t length;
    uint16_t type;
    uint16_t code;
    uint32_t transactionID;
    uint32_t parameters[3];
} __attribute__((packed));

/* Taken from USB Media Transfer Protocol Specification Revision 1.1 */

/* Table D.1: Operation Summary Table,
   Table E.1: Enhanced Operation Summary Table */
enum OperationCode {
    /* PTP */
    OPR_Undefined = 0x1000,
    OPR_GetDeviceInfo = 0x1001,
    OPR_OpenSession = 0x1002,
    OPR_CloseSession = 0x1003,
    OPR_GetStorageIDs = 0x1004,
    OPR_GetStorageInfo = 0x1005,
    OPR_GetNumObjects = 0x1006,
    OPR_GetObjectHandles = 0x1007,
    OPR_GetObjectInfo = 0x1008,
    OPR_GetObject = 0x1009,
    OPR_GetThumb = 0x100A,
    OPR_DeleteObject = 0x100B,
    OPR_SendObjectInfo = 0x100C,
    OPR_SendObject = 0x100D,
    OPR_InitiateCapture = 0x100E,
    OPR_FormatStore = 0x100F,
    OPR_ResetDevice = 0x1010,
    OPR_SelfTest = 0x1011,
    OPR_SetObjectProtection = 0x1012,
    OPR_PowerDown = 0x1013,
    OPR_GetDevicePropDesc = 0x1014,
    OPR_GetDevicePropValue = 0x1015,
    OPR_SetDevicePropValue = 0x1016,
    OPR_ResetDevicePropValue = 0x1017,
    OPR_TerminateOpenCapture = 0x1018,
    OPR_MoveObject = 0x1019,
    OPR_CopyObject = 0x101A,
    OPR_GetPartialObject = 0x0101B,
    OPR_InitiateOpenCapture = 0x101C,
    /* MTP */
    OPR_GetObjectPropsSupported = 0x9801,
    OPR_GetObjectPropDesc = 0x9802,
    OPR_GetObjectPropValue = 0x9803,
    OPR_SetObjectPropValue = 0x9804,
    OPR_GetObjectPropList = 0x9805,
    OPR_SetObjectPropList = 0x9806,
    OPR_GetInterdependentPropDesc = 0x9807,
    OPR_SendObjectPropList = 0x9808,
    OPR_GetObjectReferences = 0x9810,
    OPR_SetObjectReferences = 0x9811,
    OPR_Skip = 0x9820,
};

/* Table F.1: Response Summary Table */
enum ResponseCode {
    /* PTP */
    RSP_Undefined = 0x2000,
    RSP_OK = 0x2001,
    RSP_General_Error = 0x2002,
    RSP_Session_Not_Open = 0x2003,
    RSP_Invalid_TransactionID = 0x2004,
    RSP_Operation_Not_Supported = 0x2005,
    RSP_Parameter_Not_Supported = 0x2006,
    RSP_Incomplete_Transfer = 0x2007,
    RSP_Invalid_StorageID = 0x2008,
    RSP_Invalid_ObjectHandle = 0x2009,
    RSP_DeviceProp_Not_Supported = 0x200A,
    RSP_Invalid_ObjectFormatCode = 0x200B,
    RSP_Store_Full = 0x200C,
    RSP_Object_WriteProtected = 0x200D,
    RSP_Store_ReadOnly = 0x200E,
    RSP_Access_Denied = 0x200F,
    RSP_No_Thumbnail_Present = 0x2010,
    RSP_SelfTest_Failed = 0x2011,
    RSP_Partial_Deletion = 0x2012,
    RSP_Store_Not_Available = 0x2013,
    RSP_Specification_By_Format_Unsupported = 0x2014,
    RSP_No_Valid_ObjectInfo = 0x2015,
    RSP_Invalid_Code_Format = 0x2016,
    RSP_Unknown_Vendor_Code = 0x2017,
    RSP_Capture_Already_Terminated = 0x2018,
    RSP_Device_Busy = 0x2019,
    RSP_Invalid_ParentObject = 0x201A,
    RSP_Invalid_DevicePropFormat = 0x201B,
    RSP_Invalid_DevicePropValue = 0x201C,
    RSP_Invalid_Parameter = 0x201D,
    RSP_Session_Already_Open = 0x201E,
    RSP_Transaction_Cancelled = 0x201F,
    RSP_Specification_Of_Destination_Unsupported = 0x2020,
    /* MTP */
    RSP_Invalid_ObjectPropCode = 0xA801,
    RSP_Invalid_ObjectProp_Format = 0xA802,
    RSP_Invalid_ObjectProp_Value = 0xA803,
    RSP_Invalid_ObjectReference = 0xA804,
    RSP_Group_Not_Supported = 0xA805,
    RSP_Invalid_Dataset = 0xA806,
    RSP_Specification_By_Group_Unsupported = 0xA807,
    RSP_Specification_By_Depth_Unsupported = 0xA808,
    RSP_Object_Too_Large = 0xA809,
    RSP_ObjectProp_Not_Supported = 0xA80A,
};

/* Table G.1: Event Summary */
enum EventCode {
    /* PTP */
    EVT_Undefined = 0x4000,
    EVT_CancelTransaction = 0x4001,
    EVT_ObjectAdded = 0x4002,
    EVT_ObjectRemoved = 0x4003,
    EVT_StoreAdded = 0x4004,
    EVT_StoreRemoved = 0x4005,
    EVT_DevicePropChanged = 0x4006,
    EVT_ObjectInfoChanged = 0x4007,
    EVT_DeviceInfoChanged = 0x4008,
    EVT_RequestObjectTransfer = 0x4009,
    EVT_StoreFull = 0x400A,
    EVT_DeviceReset = 0x400B,
    EVT_StorageInfoChanged = 0x400C,
    EVT_CaptureComplete = 0x400D,
    EVT_UnreportedStatus = 0x400E,
    /* MTP */
    EVT_ObjectPropChanged = 0xC801,
    EVT_ObjectPropDescChanged = 0xC802,
    EVT_ObjectReferencesChanged = 0xC803,
};

#endif
