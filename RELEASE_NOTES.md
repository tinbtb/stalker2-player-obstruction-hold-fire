# v0.4.0-testing - Native ASI with optional diagnostics

Native ASI replaces this mod's UE4SS/Lua dependency. Neutral/Friendly NPCs withhold
shots when the predicted path intersects the player capsule plus a 40 cm margin.
Guarded reads remove the VirtualQuery-heavy validation path measured in older builds.

Metrics and engine-frame capture are disabled by default and can be enabled in the
adjacent INI. A control setting skips the entire firing hook. Includes a README,
checksums and optional Ultimate ASI Loader 9.7.4 x64 as winhttp.dll with MIT notice.
An ASI-only package is also available. Keep an existing loader; do not overwrite
proxy DLLs. Disable old ASI/UE4SS copies before installing exactly one native mod.

Experimental exact-build release. Derived native code has in-game enabled/control
measurements; release diagnostic-default changes are synthetic/smoke tested. The
uncapped comparison used different NPC counts, so no matched FPS claim is made.
Game-thread capture does not measure displayed frames or GPU time.
