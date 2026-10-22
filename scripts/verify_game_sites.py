"""Offline startup-resolution regression suite. Never executes game code."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
for mode in ['--image','--leaf']:
    subprocess.run([str(root/'build/address_probe.exe'),mode,str(Path(sys.argv[1]).resolve())],check=True)
