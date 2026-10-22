# Reproducible Windows build

Requires Windows x64, Python 3 and Zig 0.15.2. No game binaries are needed for the normal build; the fingerprint profile is checked in.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\get-zig.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Supply -ZigPath and -Python to use existing tools. The compiler and official ASI
loader archives are SHA-256 pinned. The loader DLL is renamed without modifying
its bytes. License/provenance: third_party/ultimate-asi-loader/.

GitHub Actions runs the same builder: 18 geometry, 61 native-memory/gate/metrics-switch,
10 frame-buffer/output and four isolated unsupported-host/default tests.
Output ZIPs in dist/ have fixed metadata, checksums and source/binary manifests.
Builds omit debug paths; release.json pins the verified ASI hash and packaging
refuses a different binary. CI and published assets are compared byte-for-byte.

Neutral return fire and obstruction hold/resume were validated in the previous build. The v0.5.0-beta.1 ASI passed in-game friendly, hostile, and friendly-to-hostile transition checks on Steam v2.0.6.
Friendly/hostile behavior and transitions passed in-game checks.
Release-default diagnostics settings were verified by isolated smoke tests. Earlier UE4SS source is retained in historical tags.
No raw game captures or tester installation data are published.

The builder clears non-runtime PE timestamps and the .buildid PDB-identification
section, then removes its debug-directory entry. Code, imports, runtime unwind
information and other sections are unchanged. No PDB is distributed. This removes
linker IDs that vary with source/output paths. scripts/normalize_pe.py performs
this documented step before tests/packaging; the third-party loader is untouched.

For offline resolution tests, pass `-GameExe <audited-executable>` to build.ps1.
The compiled `build/address_probe.exe --image <exe>` loads the executable only as
PAGE_READWRITE data and checks identity changes, synthetic function/global
relocation, changed instructions, duplicate candidates, broken references,
changed class names, and malformed headers. `--inspect <exe>` only resolves;
exit 10 means rejected. Neither mode runs game code or changes the input file.
CI builds the probe but has no proprietary game input. It runs the existing
behavioral tests and isolated unsupported-host smoke tests instead.

To regenerate the profile, install capstone and pefile in your development
Python environment, then run `scripts/make_address_profile.py <audited-exe>`.
Generation requires the audited baseline hash and is not part of normal builds.

Only the ASI and optional winhttp.dll are at the ZIP root. The INI and supporting
files are inside PlayerObstructionHoldFire/. The plugin checks that subfolder
first, then a legacy adjacent INI only if the new file is absent.
