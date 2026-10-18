# ROCK_NEO.EXE.1.c audit

The owner-supplied file is a Ghidra-style C decompiler export of the base USA
PS1 executable. It is reference evidence, not compilable source and not part of
the distributable project. Its SHA-256 is
`adab7ad8e5832491af2fb76c577b5e786c48c9c1acfca3b916d23ac3953de488`.
The file has 54,376 lines, 1,365,168 bytes, and 607 address-named function
definitions.

The export materially exposes the complete caller at `0x80034EB8`, including
its call to the previously reconstructed `0x80035508` lease/GTE consumer and
its two calls to the previously reconstructed `0x80035724` Buster producer.
This established the two independent fire lanes, source-proxy mode admission,
simple/composite point projection, depth and screen gates, closest-horizontal
selection, and flag-`0x80` rejection used by the new native bridge.

The export does not contain dynamically loaded `0x8010....` overlay function
definitions, so it is not a complete decompilation of tutorial enemies or EBD.
It also lacks a discovered definition for `0x80015840`; the original-instruction
probe remains the authority for that transition callback. Decompiler types,
argument counts, and recovered control flow remain hints. The owner executable
probe in this directory checks the reached target-selection branches directly.

No C export, executable bytes, private target-window table, or retail payload is
included in the source checkpoint.
