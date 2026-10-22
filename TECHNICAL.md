# Native implementation

## Firing gate

The candidate resolves addresses once on the startup worker. Nineteen complete
function/layout fingerprints retain member offsets, instructions, enum values,
and internal branches while masking address-dependent RIP/relative operands.
Seventeen native references, eleven references agreeing on four globals, the
UObject chunk/count relationship, and the CapsuleComponent registration name
must validate. Missing or ambiguous candidates disable hooks. Leaf functions
without unwind entries are found in executable sections and disambiguated by
validated callers. The checked-in profile supports builds without a game EXE.

This is a conservative compatibility heuristic, not proof that every transitive
callee or object layout is unchanged. Future game updates still need gameplay
testing. Original RVAs below document the audited Steam v2.0.6 baseline, not
runtime constants. Optional engine-frame capture uses resolved tick/cookie
addresses too. Baseline offline resolution took about 0.8 seconds on the local
machine; that is not an in-game startup benchmark. No scanning or compatibility
validation is added to the firing hook; it reads cached resolved global addresses.
The firing/focus policy and geometry remain unchanged.

The pre-shot CALL originally at RVA 0x60c67c is verified before patching. The original readiness result takes priority. Eligible
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
in-game regression for this version passed. Long pauses remain unresolved.

The builder runs 18 geometry, 61 native/gate/CSV, 10 frame-buffer/output and four
isolated unsupported-host/default tests. Optional -GameExe validates the actual
startup resolution and synthetic mutation/relocation rejection checks. CI cannot
include the proprietary game executable.

## Packaging

Zig/loader versions and hashes are pinned. The builder strips non-runtime PE
build identifiers and produces deterministic ZIPs for Vortex, with only the ASI
and optional loader at the root. Supporting files are under PlayerObstructionHoldFire/, with
source/binary manifests and checksums. No logs, saves, installation identifiers,
local paths or game files are published. Version 0.5.0-beta.1 passed in-game checks on Steam v2.0.6: friendly hold-fire, normal hostile firing, and friendly-to-hostile transitions. Earlier detailed timing evidence above applies to the previous build.

## Two-family beta.2

See `COMPATIBILITY_FAMILIES.md` for the outlined aim helper, coherent family selection, and offline-only fixture evidence. The author confirmed the new ASI works in-game on Steam v2.0.6. Development-fixture gameplay remains untested; earlier detailed v0.5.0-beta.1 measurements describe that previous binary.
