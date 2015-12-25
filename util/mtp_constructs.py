from construct import *
import datetime

__all__ = ("String DateTime Array "
           "OperationCode EventCode ResponseCode "
           "ObjectFormatCode DevicePropertyCode "
           "DeviceInfo StorageInfo ObjectInfo").split(" ")

def String(name):
    length = ExprAdapter(ULInt8("length"),
                         lambda obj, ctx: obj // 2 if obj > 0 else 0,
                         lambda obj, ctx: obj * 2 if obj > 0 else 0)
    utf16_prefixed_string = PascalString(name, length, encoding="utf_16_le")

    return ExprAdapter(utf16_prefixed_string,
                        lambda obj, ctx: obj + u"\x00" if obj else obj,
                        lambda obj, ctx: obj[:-1] if obj.endswith(u"\x00") else obj)

def DateTime(name):
    fmt = "%Y%m%dT%H%M%S"
    return ExprAdapter(String(name),
                       lambda obj, ctx: obj.strftime(fmt) if obj is not None else "",
                       lambda obj, ctx: datetime.datetime.strptime(obj, fmt) if obj else None)

def Array(subcon):
    return PrefixedArray(subcon, ULInt32("length"))

OperationCodeMapping = {
    "Undefined": 0x1000,
    "GetDeviceInfo": 0x1001,
    "OpenSession": 0x1002,
    "CloseSession": 0x1003,
    "GetStorageIDs": 0x1004,
    "GetStorageInfo": 0x1005,
    "GetNumObjects": 0x1006,
    "GetObjectHandles": 0x1007,
    "GetObjectInfo": 0x1008,
    "GetObject": 0x1009,
    "GetThumb": 0x100A,
    "DeleteObject": 0x100B,
    "SendObjectInfo": 0x100C,
    "SendObject": 0x100D,
    "InitiateCapture": 0x100E,
    "FormatStore": 0x100F,
    "ResetDevice": 0x1010,
    "SelfTest": 0x1011,
    "SetObjectProtection": 0x1012,
    "PowerDown": 0x1013,
    "GetDevicePropDesc": 0x1014,
    "GetDevicePropValue": 0x1015,
    "SetDevicePropValue": 0x1016,
    "ResetDevicePropValue": 0x1017,
    "TerminateOpenCapture": 0x1018,
    "MoveObject": 0x1019,
    "CopyObject": 0x101A,
    "GetPartialObject": 0x0101B,
    "InitiateOpenCapture": 0x101C,
    "GetObjectPropsSupported": 0x9801,
    "GetObjectPropDesc": 0x9802,
    "GetObjectPropValue": 0x9803,
    "SetObjectPropValue": 0x9804,
    "GetObjectPropList": 0x9805,
    "SetObjectPropList": 0x9806,
    "GetInterdependentPropDesc": 0x9807,
    "SendObjectPropList": 0x9808,
    "GetObjectReferences": 0x9810,
    "SetObjectReferences": 0x9811,
    "Skip": 0x9820,
}

def OperationCode(name):
    return SymmetricMapping(
        ULInt16(name),
        OperationCodeMapping,
        default=Pass
    )

EventCodeMapping = {
    "Undefined": 0x4000,
    "CancelTransaction": 0x4001,
    "ObjectAdded": 0x4002,
    "ObjectRemoved": 0x4003,
    "StoreAdded": 0x4004,
    "StoreRemoved": 0x4005,
    "DevicePropChanged": 0x4006,
    "ObjectInfoChanged": 0x4007,
    "DeviceInfoChanged": 0x4008,
    "RequestObjectTransfer": 0x4009,
    "StoreFull": 0x400A,
    "DeviceReset": 0x400B,
    "StorageInfoChanged": 0x400C,
    "CaptureComplete": 0x400D,
    "UnreportedStatus": 0x400E,
    "ObjectPropChanged": 0xC801,
    "ObjectPropDescChanged": 0xC802,
    "ObjectReferencesChanged": 0xC803,
}

def EventCode(name):
    return SymmetricMapping(
        ULInt16(name),
        EventCodeMapping,
        default=Pass
    )

DevicePropertyCodeMapping = {
    "StorageID": 0xDC01,
    "Object Format": 0xDC02,
    "Protection Status": 0xDC03,
    "Object Size": 0xDC04,
    "Association Type": 0xDC05,
    "Association Desc": 0xDC06,
    "Object File Name": 0xDC07,
    "Date Created": 0xDC08,
    "Date Modified": 0xDC09,
    "Keywords": 0xDC0A,
    "Parent Object": 0xDC0B,
    "Allowed Folder Contents": 0xDC0C,
    "Hidden": 0xDC0D,
    "System Object": 0xDC0E,
    "Persistent Unique Object Identifier": 0xDC41,
    "SyncID": 0xDC42,
    "Property Bag": 0xDC43,
    "Name": 0xDC44,
    "Created By": 0xDC45,
    "Artist": 0xDC46,
    "Date Authored": 0xDC47,
    "Description": 0xDC48,
    "URL Reference": 0xDC49,
    "Language-Locale": 0xDC4A,
    "Copyright Information": 0xDC4B,
    "Source": 0xDC4C,
    "Origin Location": 0xDC4D,
    "Date Added": 0xDC4E,
    "Non-Consumable": 0xDC4F,
    "Corrupt/Unplayable": 0xDC50,
    "ProducerSerialNumber": 0xDC51,
    "Representative Sample Format": 0xDC81,
    "Representative Sample Size": 0xDC82,
    "Representative Sample Height": 0xDC83,
    "Representative Sample Width": 0xDC84,
    "Representative Sample Duration": 0xDC85,
    "Representative Sample Data": 0xDC86,
    "Width": 0xDC87,
    "Height": 0xDC88,
    "Duration": 0xDC89,
    "Rating": 0xDC8A,
    "Track": 0xDC8B,
    "Genre": 0xDC8C,
}

def DevicePropertyCode(name):
    return SymmetricMapping(
        ULInt16(name),
        DevicePropertyCodeMapping,
        default=Pass
    )

ResponseCodeMapping = {
    "Undefined": 0x2000,
    "OK": 0x2001,
    "General Error": 0x2002,
    "Session Not Open": 0x2003,
    "Invalid TransactionID": 0x2004,
    "Operation Not Supported": 0x2005,
    "Parameter Not Supported": 0x2006,
    "Incomplete Transfer": 0x2007,
    "Invalid StorageID": 0x2008,
    "Invalid ObjectHandle": 0x2009,
    "DeviceProp Not Supported": 0x200A,
    "Invalid ObjectFormatCode": 0x200B,
    "Store Full": 0x200C,
    "Object WriteProtected": 0x200D,
    "Store ReadOnly": 0x200E,
    "Access Denied": 0x200F,
    "No Thumbnail Present": 0x2010,
    "SelfTest Failed": 0x2011,
    "Partial Deletion": 0x2012,
    "Store Not Available": 0x2013,
    "Specification By Format Unsupported": 0x2014,
    "No Valid ObjectInfo": 0x2015,
    "Invalid Code Format": 0x2016,
    "Unknown Vendor Code": 0x2017,
    "Capture Already Terminated": 0x2018,
    "Device Busy": 0x2019,
    "Invalid ParentObject": 0x201A,
    "Invalid DevicePropFormat": 0x201B,
    "Invalid DevicePropValue": 0x201C,
    "Invalid Parameter": 0x201D,
    "Session Already Open": 0x201E,
    "Transaction Cancelled": 0x201F,
    "Specification Of Destination Unsupported": 0x2020,
    "Invalid ObjectPropCode": 0xA801,
    "Invalid ObjectProp Format": 0xA802,
    "Invalid ObjectProp Value": 0xA803,
    "Invalid ObjectReference": 0xA804,
    "Group Not Supported": 0xA805,
    "Invalid Dataset": 0xA806,
    "Specification By Group Unsupported": 0xA807,
    "Specification By Depth Unsupported": 0xA808,
    "Object Too Large": 0xA809,
    "ObjectProp Not Supported": 0xA80A,
}

def ResponseCode(name):
    return SymmetricMapping(
        ULInt16(name),
        ResponseCodeMapping,
        default=Pass
    )

ObjectFormatCodeMapping = {
    "Undefined": 0x3000,
    "Association": 0x3001,
    "Script": 0x3002,
    "Executable": 0x3003,
    "Text": 0x3004,
    "HTML": 0x3005,
    "DPOF": 0x3006,
    "AIFF": 0x3007,
    "WAV": 0x3008,
    "MP3": 0x3009,
    "AVI": 0x300A,
    "MPEG": 0x300B,
    "ASF": 0x300C,
    "Undefined Image": 0x3800,
    "EXIF/JPEG": 0x3801,
    "TIFF/EP": 0x3802,
    "FlashPix": 0x3803,
    "BMP": 0x3804,
    "CIFF": 0x3805,
    "GIF": 0x3807,
    "JFIF": 0x3808,
    "CD": 0x3809,
    "PICT": 0x380A,
    "PNG": 0x380B,
    "TIFF": 0x380D,
    "TIFF/IT": 0x380E,
    "JP2": 0x380F,
    "JPX": 0x3810,
    "Undefined Firmware": 0xB802,
    "Windows Image Format": 0xB881,
    "WBMP": 0xB803,
    "JPEG XR": 0xB804,
    "Undefined Audio": 0xB900,
    "WMA": 0xB901,
    "OGG": 0xB902,
    "AAC": 0xB903,
    "Audible": 0xB904,
    "FLAC": 0xB906,
    "QCELP": 0xB907,
    "AMR": 0xB908,
    "Undefined Video": 0xB980,
    "WMV": 0xB981,
    "MP4Container": 0xB982,
    "MP2": 0xB983,
    "3GP Container": 0xB984,
    "3G2": 0xB985,
    "AVCHD": 0xB986,
    "ATSC-TS": 0xB987,
    "DVB-TS": 0xB988,
    "Undefined Collection": 0xBA00,
    "Abstract Multimedia Album": 0xBA01,
    "Abstract Image Album": 0xBA02,
    "Abstract Audio Album": 0xBA03,
    "Abstract Video Album": 0xBA04,
    "Abstract Audio & Video Playlist": 0xBA05,
    "Abstract Contact Group": 0xBA06,
    "Abstract Message Folder": 0xBA07,
    "Abstract Chaptered Production": 0xBA08,
    "Abstract Audio Playlist": 0xBA09,
    "Abstract Video Playlist": 0xBA0A,
    "Abstract Mediacast": 0xBA0B,
    "WPL Playlist": 0xBA10,
    "M3U Playlist": 0xBA11,
    "MPL Playlist": 0xBA12,
    "ASX Playlist": 0xBA13,
    "PLS Playlist": 0xBA14,
    "Undefined Document": 0xBA80,
    "Abstract Document": 0xBA81,
    "XML Document": 0xBA82,
    "Microsoft Word Document": 0xBA83,
    "MHT Compiled HTML Document": 0xBA84,
    "Microsoft Excel Spreadsheet": 0xBA85,
    "Microsoft Powerpoint Presentation": 0xBA86,
    "Undefined Message": 0xBB00,
    "Abstract Message": 0xBB01,
    "Undefined Bookmark": 0xBB10,
    "Abstract Bookmark": 0xBB11,
    "Undefined Appointment": 0xBB20,
    "Abstract Appointment": 0xBB21,
    "vCalendar 1.0": 0xBB22,
    "Undefined Task": 0xBB40,
    "Abstract Task": 0xBB41,
    "iCalendar": 0xBB42,
    "Undefined Note": 0xBB60,
    "Abstract Note": 0xBB61,
    "Undefined Contact": 0xBB80,
    "Abstract Contact": 0xBB81,
    "vCard 2": 0xBB82,
    "vCard 3": 0xBB83,
}

def ObjectFormatCode(name):
    return SymmetricMapping(
        ULInt16(name),
        ObjectFormatCodeMapping,
        default=Pass
    )

DeviceInfo = Struct(
    "DeviceInfo",
    ULInt16("StandardVersion"),
    ULInt32("MTPVendorExtensionID"),
    ULInt16("MTPVersion"),
    String("MTPExtensions"),
    SymmetricMapping(
        ULInt16("FunctionalMode"),
        {
            "Standard mode": 0x0000,
            "Sleep state": 0x0001,
            "Non-responsive playback": 0xC001,
            "Responsive playback": 0xC002,
        },
        default=Pass
    ),
    Array(OperationCode("OperationsSupported")),
    Array(EventCode("EventsSupported")),
    Array(DevicePropertyCode("DevicePropertiesSupported")),
    Array(ObjectFormatCode("CaptureFormats")),
    Array(ObjectFormatCode("PlaybackFormats")),
    String("Manufacturer"),
    String("Model"),
    String("DeviceVersion"),
    String("SerialNumber")
)

StorageInfo = Struct(
    "StorageInfo",
    SymmetricMapping(
        ULInt16("StorageType"),
        {
            "Undefined": 0x0000,
            "Fixed ROM": 0x0001,
            "Removable ROM": 0x0002,
            "Fixed RAM": 0x0003,
            "Removable RAM": 0x0004,
        },
        default=Pass
    ),
    SymmetricMapping(
        ULInt16("FilesystemType"),
        {
            "Undefined": 0x0000,
            "Generic flat": 0x0001,
            "Generic hierarchical": 0x0002,
            "DCF": 0x0003,
        },
        default=Pass
    ),
    SymmetricMapping(
        ULInt16("AccessCapability"),
        {
            "Read-write": 0x0000,
            "Read-only without object deletion": 0x0001,
            "Read-only with object deletion": 0x0002,
        },
        default=Pass
    ),
    ULInt64("MaxCapacity"),
    ULInt64("FreeSpaceInBytes"),
    ULInt32("FreeSpaceInObjects"),
    String("StorageDescription"),
    String("VolumeIdentifier")
)

ObjectInfo = Struct(
    "ObjectInfo",
    ULInt32("StorageID"),
    ObjectFormatCode("ObjectFormat"),
    SymmetricMapping(
        ULInt16("ProtectionStatus"),
        {
            "No protection": 0x0000,
            "Read-only": 0x0001,
            "Read-only data": 0x8002,
            "Non-transferable data": 0x8003,
        },
        default=Pass
    ),
    ULInt32("ObjectCompressedSize"),
    ObjectFormatCode("ThumbFormat"),
    ULInt32("ThumbCompressedSize"),
    ULInt32("ThumbPixWidth"),
    ULInt32("ThumbPixHeight"),
    ULInt32("ImagePixWidth"),
    ULInt32("ImagePixHeight"),
    ULInt32("ImageBitDepth"),
    ULInt32("ParentObject"),
    SymmetricMapping(
        ULInt16("AssociationType"),
        {
            "Undefined": 0x0000,
            "Generic Folder": 0x0001,
            "Album": 0x0002,
            "Time Sequence": 0x0003,
            "Horizontal Panoramic": 0x0004,
            "Vertical Panoramic": 0x0005,
            "2D Panoramic": 0x0006,
            "Ancillary Data": 0x0007,
        },
        default=Pass
    ),
    ULInt32("AssociationDescription"),
    ULInt32("SequenceNumber"),
    String("Filename"),
    DateTime("DateCreated"),
    DateTime("DateModified"),
    ExprAdapter(
        String("Keywords"),
        lambda obj, ctx: ' '.join(kw.replace(' ', '_') for kw in obj),
        lambda obj, ctx: obj.split(' ')
    )
)
