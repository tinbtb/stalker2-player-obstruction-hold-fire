# Native implementation

## Firing gate

The exact supported EXE SHA-256/size and pre-shot CALL at RVA 0x60c67c are
verified before patching. The original readiness result takes priority. Eligible
checks obtain the current player controller/pawn on the owning firing thread,
validate UObject array membership/flags and gameplay identity, and verify the
supported NPC aim implementation. No cross-shot UObject or relationship cache
is retained. Engine calls are outside the SEH scope.

Relationship classification at RVA 0x235809b takes 64-bit identity keys obtained
via RVA 0x4d61a4, rather than gameplay pointers. Hostile/other classifications
exit before focused-enemy, capsule or aim work. A neutral faction relationship
can persist after the player provokes an NPC; relationship alone is insufficient.

For Neutral/Friend, call the game's GetFocusedEnemy native implementation at
RVA 0x6b6ea52, after guarded input-chain checks. The reflected registration at
0x8c49b70 identifies GetFocusedEnemy -> exec wrapper 0x6702046 -> native getter.
The getter manages its scoped data reference and resolves the enemy's weak actor
reference via 0x2560e08, checking UObject index, serial and destruction flags.
Compare that actor directly with the freshly validated player pawn.

Player targets, null targets and unavailable focus data allow the original shot.
Only a known different focused actor proceeds to obstruction geometry. Scripted
fire with no focused enemy therefore has reduced protection. No engine calls or
actor-pointer dereferences occur in the diagnostic worker.

The aim helper's GetSocketByName routine is not a combat-target getter. The
intermediate socket experiments were incorrect and are absent from this release.
Only the original firing hook and optional frame-capture hook are installed.

## Geometry and guards

Capsule class, scaling and crouch dimensions are read for each eligible
other-target shot. The test measures segment-to-vertical-capsule distance, with
a 40 cm margin. A capsule with equal radius and half-height is treated as a sphere.

Shot preparation can update mode/range after the hook and blend previous/current
transforms. The gate checks modes 0/1, preserving an existing other mode where
needed, and conservatively bounds the transform blend. The hook cannot know the
prepared shot's final target distance: the check can withhold shots when the
player is behind the intended target. Spread, ricochets, explosives and bullets
already emitted are not universally covered.

Scoped SEH covers the mod's byte loads only. Guard-page exceptions restore
PAGE_GUARD before rejecting the read. The non-inlined copy helper is exercised
with actual optimized-code faults; normal reads make no VirtualQuery calls.
Invalid data allows the original shot. Target lifetime during engine calls still
depends on execution on the owning firing thread. Adjacent paths are length-checked.

## Diagnostics

All timing/counters are opt-in. EnableHoldFire=0 is a control run with no firing
hook. CaptureEngineFrames=1 enables metrics and replaces only the tick-epilogue
CALL at RVA 0x50c976, preserving its security-cookie target 0x2835d30. Its SPSC ring
does not overwrite unread samples. These are engine tick intervals, not displayed
or GPU frame times. File I/O occurs on the worker.

CaptureRelationships=1 enables metrics and adds relationship/withheld counters
and focused_enemy_player/other/none/unavailable classifications. The focused_enemy
row times the getter plus its guarded input checks. focused_targets.csv stores
up to 128 immutable samples: check, shooter, target, player, relation, status.
Statuses: 0 unavailable, 1 no enemy, 2 player, 3 other. Sampling records up to 32
early transitions and then every 128 checks. Samples contain process addresses;
raw captures are local validation evidence and are not published or packaged.

## Validation

The earlier 0.4.8 ASI demonstrated provoked-neutral return fire: 104 Neutral
checks, 104 focused-player matches, zero withheld/unknown targets or read faults.
All 32 samples matched the player pawn across five NPCs.

The exact released test2 ASI demonstrated neutral other-target obstruction:
298 Neutral checks, 298 known other targets, 157 withheld and 141 allowed attempts.
The tester confirmed visible hold-fire and resumption after moving clear.
All 34 samples identified another actor across three shooters. No unknown targets
or read faults occurred. Getter mean/max: 0.347/0.7 microseconds. All-hook mean/max,
including readiness exits: 2.05/14.8 microseconds over 845 calls. The focus/gate
implementation is identical between these builds; test2 exports the previously
omitted focus timing row and adds a CSV-output regression test.

These are targeted neutral tests, not matched FPS measurements. Friendly/hostile
in-game regression for this version remains pending. Long pauses remain unresolved.

The builder runs 18 geometry, 61 native/gate/CSV, 10 frame-buffer/output and three
isolated unsupported-host/default tests. Optional -GameExe validates the actual
shot/getter signatures and reflected getter/weak-resolver call chain. CI cannot
include the proprietary game executable.

## Packaging

Zig/loader versions and hashes are pinned. The builder strips non-runtime PE
build identifiers and produces deterministic root-level ZIPs for Vortex, with
source/binary manifests and checksums. No logs, saves, installation identifiers,
local paths or game files are published. The release ASI retains its tested
0.4.8-focused-enemy-test2 identifier to preserve the exact validated binary.
