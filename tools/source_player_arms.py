#!/usr/bin/env python3
"""Pick the player mesh set exactly as the original renderer 0x8002786C does.

0x8002786C draws five mesh banks (body, head, right arm, left arm, feet) from
one of three 5-word address sets at 0x800898EC:
    set = 0 if player+0x171 == 0 else (2 if player+0x170 == 0 else 1)
Each word is the RAM address the bank's PBD was loaded to, so the set maps to
resources by their container load address. No retail bytes are embedded here;
only addresses and names read from the owner's files are returned.
"""
import struct
from cook_actor_combat import OwnerExe, OWNER_SHA

MESH_SETS = 0x800898EC
RENDERER = 0x8002786C
SLOTS = ('body', 'head', 'right', 'left', 'feet')


def mesh_set_index(selection_170, selection_171):
    # 0x8002786C: lb +0x171; beq -> 0; lb +0x170; beq -> 2; else 1.
    if selection_171 == 0:
        return 0
    return 2 if selection_170 == 0 else 1


def select_player_meshes(image, resources_by_load, selection_170, selection_171):
    """resources_by_load: {load_address: internal PBD name} of collected resources.

    Returns {'set', 'addresses', 'resources', 'provenance'}; raises if a bank's
    address has no collected resource (never substitutes another arm).
    """
    owner = OwnerExe(image)
    index = mesh_set_index(selection_170, selection_171)
    addresses = struct.unpack('<5I', owner.read(MESH_SETS + index * 20, 20))
    resources = {}
    for slot, address in zip(SLOTS, addresses):
        internal = resources_by_load.get(address)
        if internal is None:
            raise ValueError(f'0x8002786C set {index} {slot} bank 0x{address:08X} has no collected resource')
        resources[slot] = internal.rsplit('\\', 1)[-1]
    return {'set': index, 'addresses': [hex(a) for a in addresses], 'resources': resources,
            'provenance': {'status': 'SOURCE_BACKED_PARTIAL', 'renderer': hex(RENDERER),
                           'mesh_sets': hex(MESH_SETS), 'executable_sha256': OWNER_SHA,
                           'selection_170': selection_170, 'selection_171': selection_171,
                           'selection_source': 'MEASURED: psxrecomp oracle (US disc) at first tutorial '
                                               'control, +0x170=0 +0x171=1 (also PCSX-Redux, research_01514)'}}
