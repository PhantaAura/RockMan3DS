#!/usr/bin/env python3
"""Package only audited source/document extensions; fail closed on unexpected files."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import zipfile

ALLOWED={'.c','.cpp','.h','.py','.sh','.lua','.md','.json','.txt','.pica','.sha256','.csv','.tsv','.log','.su'}
SKIP_DIRS={'__pycache__','.git','build_3ds'}
FORBIDDEN_NAMES={'ROCK_NEO.EXE','SLUS_006.03','SYSTEM.CNF'}
FORBIDDEN_SUFFIXES={'.chd','.bin','.dat','.cue','.iso','.img','.rom','.exe','.xa','.str',
                    '.mcr','.gme','.vgs','.sav','.mmlpkg','.3ds','.3dsx','.cia','.elf','.smdh'}


def collect_source_files(root:Path)->list[Path]:
    """Admit source/document text only; reject renamed binary payloads too."""
    files=[]
    for f in sorted(root.rglob('*')):
        rel=f.relative_to(root)
        if f.is_symlink():raise ValueError('symlink in source '+str(rel))
        if any(part in SKIP_DIRS for part in rel.parts):continue
        if f.is_dir():continue
        if not f.is_file():raise ValueError('non-regular source file '+str(rel))
        if (f.name.upper() in FORBIDDEN_NAMES or f.suffix.lower() in FORBIDDEN_SUFFIXES or
                f.suffix.lower().startswith('.l3')):
            raise ValueError('forbidden retail/cooked/build payload '+str(rel))
        if f.name!='Makefile' and f.suffix not in ALLOWED:raise ValueError('unexpected source file '+str(rel))
        data=f.read_bytes()
        if b'\0' in data:raise ValueError('binary data in source text '+str(rel))
        try:data.decode('utf-8')
        except UnicodeDecodeError as error:raise ValueError('non-UTF-8 source text '+str(rel)) from error
        files.append(f)
    return files


def release_source(root:Path,output:Path)->dict:
    root=Path(root).resolve();requested=Path(output)
    if requested.is_symlink():raise ValueError('release output must not be a symlink')
    output=requested.resolve();checksum=output.with_suffix(output.suffix+'.sha256')
    if root==output or root in output.parents:raise ValueError('release must be outside source tree')
    if output.suffix!='.zip':raise ValueError('source release output must have .zip suffix')
    if output.exists() or checksum.exists() or checksum.is_symlink():raise ValueError('release already exists; choose a new checkpoint name')
    version=json.loads((root/'state/CURRENT_CHECKPOINT.json').read_text())['version']
    if not isinstance(version,str) or not re.fullmatch(r'[0-9]+(?:\.[0-9]+){3}[a-z]?(?:-r[0-9]+)?',version):raise ValueError('invalid checkpoint version')
    files=collect_source_files(root)
    manifest=root/'SOURCE_MANIFEST.sha256'
    manifest.write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix()+'\n' for f in files if f!=manifest))
    if manifest not in files:files.append(manifest)
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for f in sorted(files):
            info=zipfile.ZipInfo('Legends3DS_'+version+'_Source/'+f.relative_to(root).as_posix(),(2026,10,1,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=(0o100755 if f.suffix in {'.py','.sh'} else 0o100644)<<16
            z.writestr(info,f.read_bytes())
    digest=hashlib.sha256(output.read_bytes()).hexdigest()
    with checksum.open('x') as stream:stream.write(digest+'  '+output.name+'\n')
    with zipfile.ZipFile(output) as z:
        if z.testzip():raise ValueError('archive CRC failure')
    return {'files':len(files),'sha256':digest,'path':str(output)}


def main():
    p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
    result=release_source(Path(__file__).resolve().parents[1],a.output)
    print(f'{result["files"]} source files; SHA-256 {result["sha256"]}')

if __name__=='__main__':main()
