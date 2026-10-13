"""Reproducible ASI packages; only the Python standard library is required."""
from pathlib import Path
import hashlib,json,zipfile,struct,subprocess,sys,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1]
META=json.loads((ROOT/'release.json').read_text())
def digest(data):return hashlib.sha256(data).hexdigest()
def sha(p):return digest(p.read_bytes())
def main():
 asi=ROOT/'build/PlayerObstructionHoldFire.asi';loader=ROOT/'.tools/asi-loader/winhttp.dll'
 data=asi.read_bytes();assert sha(asi)==META['expected_asi_sha256'], 'ASI differs from verified release binary'
 pe=struct.unpack_from('<I',data,0x3c)[0]
 assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x8664
 assert sha(loader)==META['loader_dll_sha256']
 # Smoke tests never run in the game. Each DLL host exits before its directory is removed.
 for config,expect in [('',False),('[Debug]\nEnableMetrics=1\n',True),('[Debug]\nCaptureEngineFrames=1\n',True)]:
  with tempfile.TemporaryDirectory(prefix='holdfire-smoke-') as temp:
   dll=Path(temp)/'test.asi';shutil.copy2(asi,dll)
   if config:(Path(temp)/'PlayerObstructionHoldFire.ini').write_text(config)
   subprocess.run([sys.executable,str(ROOT/'scripts/native_smoke.py'),str(dll),'1' if expect else '0'],check=True)
 entries={'README.md':(ROOT/'README.md').read_bytes(),'LICENSE.txt':(ROOT/'LICENSE').read_bytes(),'PlayerObstructionHoldFire.asi':data,'PlayerObstructionHoldFire.ini':(ROOT/'PlayerObstructionHoldFire.ini').read_bytes()}
 for name in ['BUILD.md','TECHNICAL.md']:entries[name]=(ROOT/name).read_bytes()
 for name in ['LICENSE.txt','PROVENANCE.md']:entries['Ultimate-ASI-Loader-'+name]=(ROOT/'third_party/ultimate-asi-loader'/name).read_bytes()
 verification={'version':META['version'],'compiler':'Zig '+META['compiler'],'asi_sha256':sha(asi),'loader_sha256':sha(loader),'supported_game_sha256':META['game_sha256'],'source_sha256':{n:sha(ROOT/n) for n in ['src/PlayerObstructionHoldFire.c','src/frame_capture.h','build.ps1','scripts/package.py','scripts/native_smoke.py','scripts/get-loader.ps1','scripts/normalize_pe.py']},'tests':{'geometry':16,'native_memory_and_metrics_switch':26,'frame_buffer_and_output':10,'unsupported_host_and_diagnostic_defaults':3},'in_game_status':'Derived from v0.3.4 tested native implementation. Release diagnostic-default switches are build/synthetic/smoke verified; no claim of separate in-game validation.'}
 out=ROOT/'dist';out.mkdir(exist_ok=True);archives=[]
 for variant,bundle in [('ASI-with-loader',True),('ASI-only',False)]:
  payload=dict(entries)
  if bundle:payload['winhttp.dll']=loader.read_bytes()
  manifest=dict(verification);manifest['bundles_optional_loader']=bundle;manifest['files']={n:digest(b) for n,b in sorted(payload.items())}
  payload['verification.json']=(json.dumps(manifest,indent=2)+'\n').encode()
  payload['SHA256SUMS.txt']=''.join(digest(b)+'  '+n+'\n' for n,b in sorted(payload.items())).encode()
  archive=out/('PlayerObstructionHoldFire_v'+META['version']+'_'+variant+'.zip')
  with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
   for n,b in sorted(payload.items()):
    info=zipfile.ZipInfo(n,date_time=(2026,10,13,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o644<<16;z.writestr(info,b)
  with zipfile.ZipFile(archive) as z:
   assert z.testzip() is None and set(z.namelist())==set(payload)
   assert all('/' not in n and '\\' not in n for n in z.namelist()), 'ZIP must contain root-level files only'
   for n,b in payload.items():assert z.read(n)==b
  archives.append(archive);print('Verified '+archive.name+' SHA256 '+sha(archive))
 (out/'SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in archives),encoding='ascii')
 (out/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
if __name__=='__main__':main()
