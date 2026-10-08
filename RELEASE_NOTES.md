## v0.2.0-testing — Windows x64 UE4SS test release

Neutral/Friendly NPCs hold fire when the player intersects their current aim line, and can resume when the player moves aside. The initial tester reports successful in-game behavior. Wider testing is welcome.

**Download `PlayerObstructionHoldFire_v0.2.0-testing_UE4SS.zip`**, then extract it into `Stalker2/Binaries/Win64`, merging with your existing `ue4ss` directory. Compatible UE4SS must already be installed; no compiler or Python is needed to play. Do not use the automatic source archives as the installation package.

The ZIP includes the original successfully tested DLL, Lua bridge, enabled marker, installation instructions, MIT license, and a verification manifest. `SHA256SUMS.txt` contains the ZIP checksum. Source and the pinned Windows builder are in the repository; CI also builds the same source separately.

Supports only executable SHA256 `61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b` (174798384 bytes). Other builds refuse to activate. No executable files are changed on disk.

Known limits: weapon range is used rather than target distance; the mod can pause shots when you stand behind a target or behind cover. Random spread, ricochets, already-fired bullets and explosives are not covered. See README for the status counters and testing checklist. Disable the earlier damage-blocking mod/PAK for a clean test. Restart required to uninstall.

The DLL is unsigned. These are experimental mod files, with one successful user testing report rather than broad compatibility certification.
