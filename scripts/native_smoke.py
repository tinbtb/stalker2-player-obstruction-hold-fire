"""Test unsupported-host refusal and diagnostic file defaults in isolated processes."""
from pathlib import Path
import ctypes,sys,time
p=Path(sys.argv[1]).resolve()
module=ctypes.CDLL(str(p));getattr(module,'InitializeASI')
log=p.with_name('PlayerObstructionHoldFire.log')
for _ in range(100):
 if log.exists() and log.stat().st_size:break
 time.sleep(.05)
assert log.read_text().startswith('REFUSED: unsupported executable SHA256/size.'), 'Host refusal missing'
assert p.with_name('native_metrics.csv').exists()==(sys.argv[2]=='1'), 'Metrics file default mismatch'
