from pathlib import Path
import hashlib, json, shutil, subprocess, sys

ROOT = Path(__file__).resolve().parents[2]
PASS = Path(__file__).resolve().parent
OLD = ROOT / 'work/pass-20261010-worldalive-01426/pack-runtime-r3'
PACK = PASS / 'pack-runtime-r1'
HOST = PASS / 'texture-host'
DAT = ROOT / 'owner-data/extracted/CDDATA/DAT'
COMMANDS = []

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def run(args):
    args = [str(x) for x in args]
    result = subprocess.run(args, capture_output=True, text=True)
    COMMANDS.append({'argv': args, 'returncode': result.returncode,
                     'stdout': result.stdout, 'stderr': result.stderr})
    if result.returncode:
        raise RuntimeError('recook failed: ' + str(args))

if PACK.exists():
    raise SystemExit('New pack path required; do not overwrite prior data')
shutil.copytree(OLD, PACK)
manifest = json.loads((PACK / 'manifest.json').read_text())
try:
    for record in manifest['resources']:
        if record['type'] == 1:
            relative = Path(record['cooked']).relative_to('legends3ds/tutorial')
            run([HOST / 'l3cook_texture', DAT / record['container'],
                 record['internal'], PACK / relative])
    prior = json.loads((ROOT / 'work/Legends3DS_0.0.14.26_WorldComesAlive/docs/validation_01426/head-cook-result.json').read_text())
    caches = [args for args in prior['commands'] if Path(args[0]).name in
              {'l3cook_material_cache', 'l3cook_player_material_cache', 'l3cook_actor_material_cache'}]
    assert len(caches) == 15
    for args in caches:
        mapped = [HOST / Path(args[0]).name]
        for arg in args[1:]:
            mapped.append(str(arg).replace(str(OLD), str(PACK)))
        run(mapped)
    changed = []
    for output in manifest['outputs']:
        rel = Path(output['path']).relative_to('legends3ds/tutorial')
        fp = PACK / rel
        new_hash = digest(fp)
        if new_hash != output['sha256']:
            changed.append({'path': rel.as_posix(), 'before': output['sha256'], 'after': new_hash})
        output['sha256'] = new_hash
        output['bytes'] = fp.stat().st_size
    manifest['checkpoint'] = '0.0.14.27'
    manifest['type1_layout'] = {'decoder': 'sector32words_by32rows_to_rowmajor',
                               'status': 'SOURCE_BACKED_PARTIAL',
                               'recooked_indexed': 39, 'recooked_caches': 15,
                               'unchanged_geometry_uv_pose': True}
    (PACK / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    (PASS / 'recook-result.json').write_text(json.dumps({'status': 'PASS', 'changed': changed,
                                                       'commands': COMMANDS}, indent=2) + '\n')
    print(json.dumps({'status': 'PASS', 'commands': len(COMMANDS), 'changed_outputs': len(changed)}))
except Exception as exc:
    (PASS / 'recook-result.json').write_text(json.dumps({'status': 'FAILED', 'error': str(exc),
                                                       'commands': COMMANDS}, indent=2) + '\n')
    raise
