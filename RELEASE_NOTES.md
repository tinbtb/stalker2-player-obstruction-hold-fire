# v0.4.3-relationship-test - Correct NPC relationship identities

Fixes the native relationship-call arguments used in v0.4.1: gameplay object
addresses were passed where the engine expects 64-bit identity keys. The mod now
uses the game's identity resolver before classifying the NPC/player relationship.
Incorrect arguments could misclassify shooters; no claim is made that this fix
resolves reported stutters or long pauses.

Hostiles exit before capsule sampling, aim calculation and obstruction geometry.
Neutral/friendly shots retain the player capsule plus 40 cm safety margin.

The exact release ASI was tested with hostiles (324 relationship checks, zero
withheld shots and zero aim calculations) and friends (1,771 checks, 569 shots
withheld, observed stop/resume). Neutral validation is pending. Average full hook
cost including readiness early returns was 0.49 us and 1.49 us respectively;
these different workloads do not establish a matched FPS improvement.

Metrics, engine tick capture and optional raw relationship counters are off by
default. Enable them through the adjacent INI and restart. Diagnostic captures
measure engine tick intervals, not GPU or displayed-frame timings.

Both ZIPs contain root-level files for Vortex. The with-loader ZIP bundles optional
Ultimate ASI Loader 9.7.4 x64 as winhttp.dll, with license and provenance; an ASI-only
ZIP is available. Keep an existing compatible loader and disable old copies of
this mod. Includes a short EN/UA/RU README and explicit AI-generated disclosure.

Experimental exact-build release; see README for supported EXE hash and settings.
