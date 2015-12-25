#!/usr/bin/env python3

import datetime
import textwrap

from construct import Container
import yaml

from mtp_constructs import *

def make_c_array_definition(name, data, width=80, indent=4, pre_indent=0, const=False):
    words = ["0x{:02X}".format(x) for x in bytearray(data)]
    if const:
        lines = ["const uint8_t {:s}[] = {{".format(name)]
    else:
        lines = ["uint8_t {:s}[] = {{".format(name)]
    wrapped_lines = textwrap.wrap(", ".join(words), width-pre_indent-4)
    lines.extend(" "*indent + line for line in wrapped_lines)
    lines.append("};")

    return "\n".join(" "*pre_indent + line for line in lines)

def make_c_array_extern(name, data, const=False):
    if const:
        return "extern const uint8_t {:s}[{:d}];".format(name,len(data))
    else:
        return "extern uint8_t {:s}[{:d}];".format(name,len(data))

defaults = {
    "DeviceInfo": Container(
        StandardVersion=100,
        MTPVendorExtensionID=0xFFFFFFFF,
        MTPVersion=100,
        MTPExtensions="",
        FunctionalMode="Standard mode",
        OperationsSupported=[],
        EventsSupported=[],
        CaptureFormats=[],
        PlaybackFormats=[],
        DevicePropertiesSupported=[],
        Manufacturer="",
        Model="",
        DeviceVersion="",
    ),

    "StorageInfo": Container(
        MaxCapacity=0,
        FreeSpaceInBytes=0,
        FreeSpaceInObjects=0,
        StorageDescription="",
    ),
    
    "ObjectInfo": Container(
        ProtectionStatus="No protection",
        ThumbFormat="Undefined",
        ThumbCompressedSize=0,
        ThumbPixWidth=0,
        ThumbPixHeight=0,
        ImagePixWidth=0,
        ImagePixHeight=0,
        ImageBitDepth=0,
        AssociationType="Undefined",
        AssociationDescription=0,
        SequenceNumber=0,
        DateCreated=None,
        DateModified=None,
        Keywords=[],
    ),
}

constructs = {
    "DeviceInfo":  DeviceInfo,
    "StorageInfo": StorageInfo,
    "ObjectInfo":  ObjectInfo,
}

if __name__ == "__main__":
    import argparse
    import sys
    
    parser = argparse.ArgumentParser()
    parser.add_argument("defn",
                        type=argparse.FileType("r"),
                        help="YAML datastructure definition file")
    parser.add_argument("dest",
                        type=argparse.FileType("w+"),
                        help="Filename to write to")
    parser.add_argument("--mode",
                        choices=["header", "definition"],
                        default="definition",
                        help="Output mode - header or definition")
    parser.add_argument("--warn-unused",
                        action="store_true",
                        help="Warn about unused fields")

    args = parser.parse_args()
    definitions = yaml.load(args.defn)
    args.defn.close()

    # Parse YAML definitions of datasets
    datasets = []    
    for dataset_defn in definitions["datasets"]:
        name = dataset_defn.pop("name")
        dataset_type = dataset_defn.pop("type")
        if "comment" in dataset_defn:
            comment = dataset_defn.pop("comment")
        else:
            comment = None
        if "const" in dataset_defn:
            const = dataset_defn.pop("const")
            if not isinstance(const, bool):
                if const not in ["true", "True", "TRUE", "false", "False", "FALSE"]:
                    raise ValueError("Invalid boolean value for 'const': {}".format(const))
                else:
                    const = const in ["true", "True", "TRUE"]
        else:
            const = True

        dataset_construct = constructs[dataset_type]
        container = defaults[dataset_type].copy()
        container.update(dataset_defn)
        
        dataset_bytes = dataset_construct.build(container)
        datasets.append((name, dataset_bytes, const, comment))

        if args.warn_unused:
            real_fields = set(dataset_construct.parse(dataset_bytes).keys())
            unused_fields = set(dataset_defn.keys()) - real_fields
            if unused_fields:
                print("Unused fields in {}".format(name), file=sys.stderr)
                for unused_name in unused_fields:
                    print(unused_name, file=sys.stderr)


    # Generate output file
    if args.mode == "header":
        prologue = definitions.get("header_prologue", "")
        epilogue = definitions.get("header_epilogue", "")
        body = ""
        for (name, bytes_, const, comment) in datasets:
            if comment:
                body += "/* {} */\n".format(comment)
            body += make_c_array_extern(name, bytes_, const=const)
            body += "\n"
    else:
        prologue = definitions.get("definition_prologue", "")
        epilogue = definitions.get("definition_epilogue", "")
        body = ""
        for (name, bytes_, const, comment) in datasets:
            if comment:
                body += "/* {} */\n".format(comment)
            body += make_c_array_definition(name, bytes_, const=const)
            body += "\n"

    for section in [prologue, body, epilogue]:
        if section and not section.endswith("\n"):
            section += "\n"
        args.dest.write(section)

    args.dest.close()
