v0.2.3-cache-metrics - Windows UE4SS testing release

- Increase capsule safety margin from 15 to 40 cm on radius and half-height.
- Replace recurring snapshot file I/O with an in-memory bridge.
- Cache and validate the controller, refresh every 100 uses, and read current pawn/component references each update.
- Invalidate native snapshots immediately when objects are missing or callbacks fail.
- Include cumulative performance metrics and capture instructions; instrumentation remains enabled.

The release ZIP uses the exact DLL and Lua script from the latest in-game capture: 6,289 accepted snapshots, 906 withheld attempts, no unavailable-player/unsupported-aim checks. Average callback duration in separate captures decreased from 1,166 to 81 microseconds; this is not a controlled FPS benchmark.

Requires the executable fingerprint documented in README and an existing UE4SS installation. Close the game and replace both Lua and DLL. Do not hot reload. No loader or game binaries are bundled.
