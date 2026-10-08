"""Verify and package the Windows mod using only Python's standard library."""
from pathlib import Path
import argparse, ctypes, hashlib, json, os, struct, zipfile

ROOT = Path(__file__).resolve().parents[1]
META = json.loads((ROOT / 'release.json').read_text())

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def verify_game(path):
    data = path.read_bytes()
    if len(data) != 174798384 or hashlib.sha256(data).hexdigest() != META['game_sha256']:
        raise RuntimeError('Unsupported game executable SHA256/size')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    sections = struct.unpack_from('<H', data, pe + 6)[0]
    table = pe + 24 + struct.unpack_from('<H', data, pe + 20)[0]
    for n in range(sections):
        at = table + 40*n
        virtual_size, rva, raw_size, offset = struct.unpack_from('<IIII', data, at + 8)
        if rva <= 0x60c67c < rva + max(virtual_size, raw_size):
            hook = offset + 0x60c67c - rva
            if data[hook:hook+8] != bytes.fromhex('e89b3de3ff84c075'):
                raise RuntimeError('Game hook bytes differ')
            return
    raise RuntimeError('Game hook RVA not found')

def verify_dll(path):
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', data, pe+4)[0] != 0x8664:
        raise RuntimeError('Not a Windows x64 PE DLL')
    if os.name != 'nt':
        raise RuntimeError('Native smoke verification requires Windows')
    module = ctypes.CDLL(str(path.resolve()))
    for name in ('luaopen_PlayerObstructionHoldFire', 'luaopen_PlayerObstructionSnapshot', 'luaopen_PlayerObstructionStatus'):
        getattr(module, name)
    module.luaopen_PlayerObstructionHoldFire.argtypes = [ctypes.c_void_p]
    module.luaopen_PlayerObstructionHoldFire.restype = ctypes.c_int
    module.luaopen_PlayerObstructionHoldFire(None)
    log = path.with_name('PlayerObstructionHoldFire.log')
    if not log.read_text().startswith('REFUSED: unsupported executable SHA256/size.'):
        raise RuntimeError('Unsupported-host guard did not refuse')
    log.unlink()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dll', type=Path, default=ROOT/'build/PlayerObstructionHoldFire.dll')
    parser.add_argument('--game-exe', type=Path)
    args = parser.parse_args()
    dll = args.dll.resolve()
    verify_dll(dll)
    if args.game_exe:
        verify_game(args.game_exe)
    original = sha(dll) == META['tested_dll_sha256']
    prefix = 'ue4ss/Mods/PlayerObstructionHoldFire/'
    entries = {
        'README.md': ROOT/'README.md',
        'LICENSE': ROOT/'LICENSE',
        prefix+'enabled.txt': ROOT/'mod/enabled.txt',
        prefix+'Scripts/main.lua': ROOT/'mod/Scripts/main.lua',
        prefix+'Scripts/PlayerObstructionHoldFire.dll': dll,
    }
    manifest = {
        'version': META['version'],
        'package_binary': 'original-user-tested-binary' if original else 'source-rebuild',
        'in_game_evidence': 'User reported successful behavior with original DLL; broad compatibility testing pending.',
        'compiler_version': META['compiler'],
        'supported_game_sha256': META['game_sha256'],
        'unsupported_host_refusal': 'passed',
        'x64_dll_exports': 'passed',
        'exact_game_hook_verification': 'passed' if args.game_exe else 'not run in this packaging invocation',
        'source_sha256': {'src/PlayerObstructionHoldFire.c': sha(ROOT/'src/PlayerObstructionHoldFire.c'), 'mod/Scripts/main.lua': sha(ROOT/'mod/Scripts/main.lua')},
        'files': {name: sha(path) for name,path in entries.items()},
    }
    payload = {name:path.read_bytes() for name,path in entries.items()}
    payload['verification.json'] = (json.dumps(manifest, indent=2)+'\n').encode()
    payload['SHA256SUMS.txt'] = ''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(payload.items())).encode()
    out = ROOT/'dist'; out.mkdir(exist_ok=True)
    name = 'PlayerObstructionHoldFire_v'+META['version']+'_UE4SS.zip'
    archive = out/name
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name,data in sorted(payload.items()):
            info=zipfile.ZipInfo(name, date_time=(2026,10,8,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=0o644 << 16
            z.writestr(info,data)
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None: raise RuntimeError('ZIP CRC failed')
        if set(z.namelist()) != set(payload): raise RuntimeError('ZIP entry mismatch')
        for name,data in payload.items():
            if z.read(name)!=data: raise RuntimeError('ZIP contents differ: '+name)
    (out/'SHA256SUMS.txt').write_text(sha(archive)+'  '+archive.name+'\n', encoding='ascii')
    (out/'verification.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Verified '+str(archive))
    print('Binary: '+manifest['package_binary'])
    print('ZIP SHA256: '+sha(archive))

if __name__ == '__main__':
    main()
