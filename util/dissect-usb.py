#!/usr/bin/env python3

# Depends on scapy-python3
# Usage with tshark:
#
# tshark -P -i usbmon6 -w dump.pcap
#
# ./dissect-usb.py dump.pcap --mtp --extract-transactions
#


from scapy.all import *
import textwrap
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

CombinedMapping = {**OperationCodeMapping, **ResponseCodeMapping}

CodeToNameMapping = dict((v,k) for (k,v) in CombinedMapping.items())

class SignedLongField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "q")

class LESignedLongField(Field):
    def __init__(self, name, default):
        Field.__init__(self, name, default, "<q")

class XLEShortField(LEShortField):
    def i2repr(self, pkt, x):
        return lhex(self.i2h(pkt, x))

class XLEIntField(LEIntField):
    def i2repr(self, pkt, x):
        return lhex(self.i2h(pkt, x))

class XLELongField(LELongField):
    def i2repr(self, pkt, x):
        return lhex(self.i2h(pkt, x))
"""
class CharEnumField(EnumField):
    def __init__(self, name, default, enum, fmt = "1s"):
        EnumField.__init__(self, name, default, enum, fmt)
        k = list(self.i2s.keys())
        if k and len(k[0]) != 1:
            self.i2s,self.s2i = self.s2i,self.i2s
    def any2i_one(self, pkt, x):
        if len(x) != 1:
            x = self.s2i[x]
        return x
"""

class ZeroByteFlagField(ByteField):
    def i2m(self, pkt, x):
        return x == 0
    def m2i(self, pkt, x):
        return 0 if x else 0x3E
    def i2repr(self, pkt, x):
        return self.i2m(pkt, x)

class URB(Packet):
    name = "USB Request Block"
    fields_desc = [
        XLELongField("id", 0),
        CharEnumField("type", b"S", {
            b"S":"sub",
            b"C":"cb",
            b"E":"err",
        }),
        ByteEnumField("xfer_type", 0, ["ISO", "Intr","Ctrl", "Bulk"]),
        XByteField("epnum", 0),
        ByteField("devnum", 0),
        LEShortField("busnum", 0),
        XByteField("flag_setup", 0),
        ZeroByteFlagField("flag_data", 0x3E),
        LESignedLongField("ts_sec", 0),
        LESignedIntField("ts_usec", 0),
        LESignedIntField("status", 0),
        LEIntField("length", 0),
        LEIntField("len_cap", 0),
        StrFixedLenField("setup", bytes(bytearray(8)), length=8),
        LESignedIntField("interval", 0),
        LESignedIntField("start_frame", 0),
        LEIntField("xfer_flags", 0),
        LEIntField("ndesc", 0)
    ]

    def guess_payload_class(self, payload):
        if len(payload) >= 12:
            block_length, block_type = struct.unpack("<IH", payload[:6])
            if block_length >= len(payload) and block_type in [1, 2, 3, 4]:
                return MTP

        return Packet.guess_payload_class(self, payload)

conf.l2types.register(220, URB)

class MTP(Packet):
    name = "USB Media Transfer Protocol"
    fields_desc = [
        LEIntField("length", 12),
        LEShortEnumField("type", 0, ["Undf", "Comm", "Data", "Resp", "Evnt"]),
        LEShortEnumField("code", 0, CodeToNameMapping),
        LEIntField("transactionID", 0),
    ]
    def guess_payload_class(self, payload):
        if self.type == 1:
            return Command
        #elif self.type == 2:
        #    return Data
        elif self.type == 3:
            return Response
        else:
            return Packet.guess_payload_class(self, payload)

#conf.debug_dissector = True

class Command(Packet):
    name = "USB MTP Command Block"
    fields_desc = [
        FieldListField("parameters", None, XLEIntField("parameter", 0), count_from=lambda pkt: (pkt.underlayer.length-12)/4)
    ]

class Response(Packet):
    name = "USB MTP Response Block"
    fields_desc = [
        FieldListField("parameters", None, XLEIntField("parameter", 0), count_from=lambda pkt: (pkt.underlayer.length-12)/4)
    ]

class Testing(Packet):
    fields_desc = [
        FieldListField("parameters", None, XLEIntField("parameter", 0))
    ]

def filtered_packets(args):
    with args.capture_file as capture:
        for pkt in capture:
            if pkt.haslayer("URB") and len(pkt.payload) > 0:
                if not args.endpoints or (pkt.epnum in args.endpoints):
                    yield pkt

def extract_transactions(urb_packets):
    txn_id = None
    txn_bytes_remaining = 0
    txn = []
    for pkt in urb_packets:
        if pkt.haslayer("MTP") and txn_bytes_remaining == 0:
            if pkt.payload.transactionID != txn_id:
                if txn:
                    yield txn
                txn_id = pkt.payload.transactionID
                txn = [pkt.payload]
                txn_bytes_remaining = 0
            else:
                txn.append(pkt.payload)
            if txn[-1].length > len(txn[-1]):
                txn_bytes_remaining = txn[-1].length-len(txn[-1])
        elif txn and txn_bytes_remaining > 0:
            if len(pkt.payload) <= txn_bytes_remaining:
                pkt.decode_payload_as(conf.raw_layer)
                txn.append(pkt.payload)
                txn_bytes_remaining -= len(pkt.payload)
            else:
                print("    expecting {:d} more bytes but got {:d}".format(txn_bytes_remaining, len(pkt.payload)))
    if txn:
        yield txn

def structure_transaction(packets):
    # Extract the command and response packets
    command, *data_packets, response = packets

    txn = {}
    txn["id"] = command.transactionID
    txn["command"] = command.code
    if command.payload:
        txn["arguments"] = command.payload.parameters
    else:
        txn["arguments"] = None

    if data_packets:
        txn["data"] = b"".join(filter(None, map(get_leftovers, data_packets)))
        txn["data_length"] = data_packets[0].length - 12
        txn["data_dir"] = "in" if (data_packets[0].underlayer.epnum & 0x80) else "out"
    else:
        txn["data"] = None
        txn["data_length"] = None

    txn["status"] = response.code
    if response.payload:
        txn["response"] = response.payload.parameters
    else:
        txn["response"] = None

    return txn

def get_leftovers(pkt):
    while pkt.payload:
        pkt = pkt.payload
    if hasattr(pkt, "load"):
        return pkt.load
    else:
        return None

def wrap_hex_data(initial, data):
    wrapper = textwrap.TextWrapper(initial_indent=initial,
                                   subsequent_indent=" "*len(initial))
    return wrapper.fill(" ".join("{:02X}".format(b) for b in data))

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("capture_file", type=PcapReader)
    parser.add_argument("--extract-transactions", action="store_true")
    parser.add_argument("--endpoints", type=(lambda x:int(x,0)), nargs="*", default=[])
    parser.add_argument("--mtp", action="store_true")

    args = parser.parse_args()

    if args.mtp:
        args.endpoints.extend([0x86, 0x05])

    if args.extract_transactions:
        for txn_pkts in extract_transactions(filtered_packets(args)):
            txn_id = txn_pkts[0].transactionID
            print("Transaction {:02d}:".format(txn_id))
            #for pkt in txn_pkts:
            #    print("    " + repr(pkt))
            if len(txn_pkts) >= 2:
                txn = structure_transaction(txn_pkts)
                if txn["arguments"]:
                    print("   >{:24s} {:s}".format(
                        CodeToNameMapping[txn["command"]],
                        ", ".join("0x{:08X}".format(arg) for arg in txn["arguments"])
                    ))
                else:
                    print("   >"+CodeToNameMapping[txn["command"]])

                if txn["data"]:
                    arrow = "<" if txn["data_dir"] == "in" else ">"
                    print(wrap_hex_data("   " + arrow, txn["data"]))

                strings = [CodeToNameMapping[txn["status"]]]
                if txn["response"]:
                    strings.extend("0x{:08X}".format(arg) for arg in txn["response"])
                print("   <" + (", ".join(strings)))
            else:
                print("   Partial transaction")
    else:
        for pkt in filtered_packets(args):
            print(repr(pkt))
