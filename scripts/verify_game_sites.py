"""Offline startup-resolution regression suite. Never executes game code."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
modes=['--outlined','--outlined-leaf'] if '--outlined' in sys.argv[2:] else ['--image','--leaf']
for mode in modes:
    subprocess.run([str(root/'build/address_probe.exe'),mode,str(Path(sys.argv[1]).resolve())],check=True)
