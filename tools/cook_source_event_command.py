#!/usr/bin/env python3
"""Cook owner-local command lookup/cold metadata, never code or CD media."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib
from cook_source_event_phase2 import EXE_SHA, cook as cook_phase2


def cook(image):
    phase2, _ = cook_phase2(image)  # Exact executable identity/profile guard.
    base, size = struct.unpack_from('<II', image, 0x18)
    def read(address, count):
        if address < base or address + count > base + size:
            raise ValueError('Command source field leaves measured LOAD')
        return image[2048+address-base:2048+address-base+count]
    identity = struct.unpack_from('<I', phase2, 68)[0]
    sector = struct.unpack('<I', read(0x80082CD0+identity*12, 4))[0]
    byte_addresses = [0x800988EC,0x8009896C,0x800988C0,0x80098B42,0x80098AB8]
    half_addresses = [0x800AD142,0x800AD14E]
    word_addresses = [0x800989B0,0x80091FFC,0x80092000,0x800989D8]
    bytes_ = [read(address, 1)[0] for address in byte_addresses]
    halves = [struct.unpack('<H', read(address, 2))[0] for address in half_addresses]
    words = [struct.unpack('<I', read(address, 4))[0] for address in word_addresses]
    mix, response = read(0x800AD154, 4), read(0x80098A98, 8)
    if any(bytes_+halves+words) or any(mix+response):
        raise ValueError('Unsupported cold command LOAD profile')
    payload = struct.pack('<IIBBBBB3xHHIIII4s8s', identity,sector,*bytes_,*halves,*words,mix,response)
    assert len(payload)==48
    header = struct.pack('<4sHH6I32s',b'L3EC',1,64,1,0,len(payload),64+len(payload),
                         zlib.crc32(payload),0,bytes.fromhex(EXE_SHA))
    blob=header+payload
    proof={'schema':'mml-event-command-cook/1','status':'SOURCE_BACKED_PARTIAL',
           'source_sha256':EXE_SHA,'command_id':identity,'file_sector_80082cd0':sector,
           'lookup_address':hex(0x80082CD0+identity*12),
           'cold_byte_addresses':[hex(x) for x in byte_addresses],
           'cold_half_addresses':[hex(x) for x in half_addresses],
           'cold_word_addresses':[hex(x) for x in word_addresses],
           'cold_regions':['0x800AD154:4','0x80098A98:8'],
           'cold_fields_measured_in_load_image':True,'live_transport_receipts_provided':False,
           'cooked_bytes':len(blob),'cooked_sha256':hashlib.sha256(blob).hexdigest(),
           'owner_payload_release_permitted':False,'retail_parity':False}
    return blob,proof


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists(): raise SystemExit('Refusing to overwrite command package')
    blob,proof=cook(args.exe.read_bytes())
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_bytes(blob)
    print(json.dumps(proof,indent=2))


if __name__=='__main__': main()
