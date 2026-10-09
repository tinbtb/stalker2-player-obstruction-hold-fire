# Player Obstruction Hold Fire for S.T.A.L.K.E.R. 2

An experimental Windows x64 UE4SS mod that asks Neutral and Friendly NPCs to hold their fire when the player obstructs their current aim line. Once the player moves aside, the NPC can resume shooting. The game decides which characters are Neutral or Friendly.

**AI-generated code:** The native C code, Lua bridge, and build/packaging scripts were generated using AI (OpenAI Codex), under the author's direction. The author has personally tested the mod in-game.

The author has tested the current diagnostic build in-game. Its capture recorded 6,289 accepted snapshots and 906 withheld shot attempts, with no unavailable-player or unsupported-aim gate checks. Wider compatibility testing is still needed. Withholding a shot prevents creation of that bullet, which should also prevent its bleeding and armor-wear effects; this does not protect against other sources of damage.

## Download and install

Download **PlayerObstructionHoldFire_v0.2.3-cache-metrics_UE4SS.zip** from [Releases](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire/releases). GitHub's automatic "Source code" archives contain source, not a ready-to-play DLL.

1. Close the game completely.
2. Install UE4SS if you do not already use it. **I (tinbtb) personally use the UE4SS build supplied by [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/), and that is the setup I tested this mod with.** Follow Ultra+ Manager's installation instructions for S.T.A.L.K.E.R. 2. If you already have its UE4SS build installed, keep that installation. The tested build identifies itself as UE4SS 3.0.1 Beta #0, Git revision `44afb36d`, on UE 5.5. [Other UE4SS builds](https://github.com/UE4SS-RE/RE-UE4SS/releases) have not been validated here. This mod needs `package.loadlib`, `LoopAsync`, and `ExecuteInGameThread`.
3. Extract the mod ZIP into your game's `Stalker2/Binaries/Win64` folder. Merge the `ue4ss` folder with the existing one.
4. Confirm `ue4ss/Mods/PlayerObstructionHoldFire/enabled.txt` and `Scripts/PlayerObstructionHoldFire.dll` exist.
5. Start the game and load a save.

No compiler, Python, administrator access, or ASI loader is needed to install this mod. The package does not bundle UE4SS or replace any proxy DLL. If you have our earlier `NeutralFriendlyFire` mod or `zzzzzz_FriendlyFireTest_P.pak`, disable those for a clean test; their damage filtering can conceal missed obstruction checks.

## Supported game executable

This release supports only the analyzed executable, from the vanilla 2.0.6 data environment:

- File: `Stalker2-Win64-Shipping.exe`
- Size: `174798384` bytes
- SHA256: `61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`

The SHA256 and instruction bytes are checked before installation. Other builds are refused. A matching version label alone is insufficient. This mod patches process memory; it does not modify the executable on disk.

## Check that it loaded

Open `ue4ss/Mods/PlayerObstructionHoldFire/Scripts/PlayerObstructionHoldFire.log` after loading a save. `ACTIVE` means the hook was installed; an increasing `withheld` counter confirms attempts were deferred.

| Counter | Meaning |
| --- | --- |
| `checks` | Shot attempts allowed by the original readiness check and considered by this mod |
| `withheld` | Attempts redirected to the game's existing defer/retry branch |
| `unavailable_player` | Player snapshot absent/stale, or shot hook ran on a different thread |
| `unsupported_aim` | Shooter has an aim routine this release does not recognize |
| `snapshot_age_ms` | Time since the last player snapshot |

Counters normally refresh every 100 completed callbacks (approximately five seconds). Snapshot errors appear in `UE4SS.log`. Zero checks may mean the weapon does not use this path. If the log says `REFUSED`, include its full text in a bug report.

## Test and report

Have a Neutral/Friendly NPC fight an enemy. Stand between them: shooting should pause and `withheld` should rise. Move aside: shooting should resume without a catch-up volley. Repeat mid-burst, crouched, close to the muzzle, and after loading a save. Confirm hostile NPCs still shoot you and the protected NPC still shoots when you are beside or behind it. Observe HP, bleeding, armor wear, and ammo use where practical.

Report the game executable SHA256, UE4SS version, other combat mods, the native status log, and what happened through [Issues](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire/issues). Review logs for personal paths before posting them.

## Known limits

- The check uses the current central aim line and **weapon range**, not the intended target distance or world occlusion. It can withhold fire if the player stands behind the target or behind cover.
- Random spread, already-fired bullets, ricochets, grenades, and moving into a bullet are not covered.
- Only the identified pre-shot path and NPC aim implementation are covered. Unknown paths are allowed to proceed.
- Player identity and scaled capsule dimensions refresh on the game thread every 50 ms through an in-memory callback bridge. Snapshots older than 250 ms are ignored. The native hook reads the current player position and adds a 40 cm safety margin to both capsule radius and half-height.
- Missing, stale, or unsupported inputs leave the game's original firing behavior in place.
- The DLL is unsigned. Publishing source and checksums enables inspection and integrity checks; neither is a guarantee of safety.

## Uninstall

Close the game and remove `ue4ss/Mods/PlayerObstructionHoldFire`. Restart is required; do not hot-unload the DLL. Restore your earlier mod if desired.

## Build from source on Windows

Requires 64-bit Windows, PowerShell, and Python 3. The compiler setup script downloads the official Zig 0.15.2 Windows x64 archive and verifies its pinned SHA256. No game installation or Unreal SDK is required for the build.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\get-zig.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

You may supply your own Zig 0.15.2 compiler with `-ZigPath C:\path\to\zig.exe` to `build.ps1`. Build outputs and the testing ZIP are placed in `build/` and `dist/`. The build runs bridge transaction checks and 16 capsule-intersection cases and checks that the DLL refuses an unsupported host before packaging. Optional exact-executable validation:

```powershell
python .\scripts\package.py --game-exe "C:\path\to\Stalker2-Win64-Shipping.exe"
```

The v0.2.3-cache-metrics release ZIP contains the exact DLL and Lua script used for the author's latest diagnostic capture. CI separately rebuilds the source. PE metadata and build paths can make rebuilt DLL hashes differ; bit-identical builds are not claimed. `verification.json` records file hashes and binary provenance. `SHA256SUMS.txt` lets you check the downloaded ZIP with `Get-FileHash -Algorithm SHA256`.

GitHub Actions also builds the source on a Windows runner and uploads its separately identified CI artifacts. A successful build does not replace in-game testing.

## Implementation

[Native source](src/PlayerObstructionHoldFire.c) contains the game-build fingerprint, hook installer, original readiness call, aim/capsule check, and native relation query. [Lua bridge](mod/Scripts/main.lua) updates player identity/capsule dimensions on the game thread. [Technical notes](TECHNICAL.md) document the exact offsets and validation boundary.

MIT licensed. No game binaries, extracted game assets, compiler distribution, or third-party loader binaries are included.

Diagnostic status is overwritten approximately every five seconds in `Scripts/PlayerObstructionHoldFire.log`. It includes version, safety margin, system uptime, snapshot attempts/successes, gate checks, shots withheld, unavailable-player checks, unsupported aim and snapshot age. Counters are cumulative; take differences between samples to calculate rates. No per-shot logging is added. The existing 50 ms snapshot loop is unchanged for baseline comparison.

## Diagnostic release and performance evidence

This release retains timing instrumentation and writes cumulative summaries to `Scripts/baseline_metrics.csv`. The CSV resets on activation; preserve it before restarting. See [capture instructions](BASELINE_CAPTURE.md). No logs or measurements are uploaded automatically.

Compared with earlier captures, average Lua callback duration fell from 1,166 microseconds (disk bridge) to 278 microseconds (memory bridge) to 81 microseconds (controller cache). These were different gameplay runs with instrumentation, not controlled FPS benchmarks; an FPS improvement has not been established. The controller cache is validated every callback and refreshed every 100 uses. Pawn/component references are read anew, and missing objects or callback errors immediately invalidate the native snapshot. No UObjectCacheMod dependency is introduced.

The 40 cm margin is larger than the original release's 15 cm margin and can withhold more shots near the player. No movement prediction is included.
