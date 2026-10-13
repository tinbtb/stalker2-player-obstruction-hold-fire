# Reproducible Windows build

Requires Windows x64, Python 3 and Zig 0.15.2. No game binaries are needed.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\get-zig.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Supply -ZigPath and -Python to use existing tools. The compiler and official ASI
loader archives are SHA-256 pinned. The loader DLL is renamed without modifying
its bytes. License/provenance: third_party/ultimate-asi-loader/.

GitHub Actions runs the same builder: 16 geometry, 26 native-memory/metrics-switch,
10 frame-buffer/output and three isolated unsupported-host/default tests.
Output ZIPs in dist/ have fixed metadata, checksums and source/binary manifests.
Builds omit debug paths; release.json pins the verified ASI hash and packaging
refuses a different binary. CI and published assets are compared byte-for-byte.

The release's diagnostics-default changes are synthetic/smoke tested, without a
separate in-game run of this exact release binary. Derived native code was tested
in-game with diagnostics. Earlier UE4SS source is retained in historical tags.
No raw game captures or tester installation data are published.

The builder clears non-runtime PE timestamps and the .buildid PDB-identification
section, then removes its debug-directory entry. Code, imports, runtime unwind
information and other sections are unchanged. No PDB is distributed. This removes
linker IDs that vary with source/output paths. scripts/normalize_pe.py performs
this documented step before tests/packaging; the third-party loader is untouched.
