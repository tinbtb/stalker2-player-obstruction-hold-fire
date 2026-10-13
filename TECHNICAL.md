# Native implementation

The game fingerprint and pre-shot CALL at RVA 0x60c67c remain exact-build guarded.
Native player/controller getters run only on the owning firing thread. Object array
membership/flags, player identity and capsule class/data are freshly read each shot
using a scoped SEH byte-copy helper. Engine calls are outside the SEH scope. There
is no persistent UObject or mapped-region cache. Concurrent lifetime changes during
engine calls still depend on the firing-thread assumption.

Guard-page exceptions restore PAGE_GUARD before rejecting the read. The copy helper
must remain non-inlined so Zig/Clang generates a working SEH invoke boundary; tests
exercise actual access faults in optimized code. Normal reads make no VirtualQuery
calls. Unknown/invalid data permits the original shot.

The obstruction test uses segment-to-vertical-capsule distance and a 40 cm margin,
then checks Neutral/Friendly relation before asking the original defer path to run.
Capsule scaling/crouching is read every eligible shot.

INI flags gate all diagnostic counters/QPC timing. CaptureEngineFrames implies
EnableMetrics. Only the optional tick epilogue call at RVA 0x50c976 is replaced for
frame capture, preserving its original security-cookie target at RVA 0x2835d30.
No tick entry hook is required. The SPSC ring never overwrites unread samples; file
I/O is confined to the worker. A control run installs no firing hook at all.

The source compiles without debug paths for reproducible builds.
Packages have fixed ZIP metadata and pinned compiler/loader inputs. Verification
manifests include source and binary hashes. No tester logs, installation paths,
saves, machine IDs or game files are packaged.


## 0.4.2 experimental early exits

The original readiness check still runs first. The owning firing thread resolves
and validates the current player identity, validates the shooter and supported
aim function, then checks the relationship. Only relation 2 or 3 proceeds to
shot-range validation, capsule sampling, aim calculation, and obstruction geometry.
The safety margin and intersection rules are unchanged. Invalid data permits firing.
No relationship or UObject cache persists between shots.

Metrics now separate `native_player_identity` from `player_capsule`. The former
replaces the old combined `native_player_lookup`; compare their combined total
when assessing lookup work. `snapshot_successes` now counts completed capsule
samples, so fewer successes than attempts is expected for hostile-only encounters.
Relation calls increase for neutral/friendly shots whose trajectories miss the player;
this tradeoff needs testing in addition to hostile-only encounters.

Validation: repeat hostile-only and mixed encounters with EnableMetrics=1 and
CaptureEngineFrames=1, plus EnableHoldFire=0 control runs. Compare gate total and
maximum, aim/capsule counts, and frame p99/p99.9/max. Metrics remain off by default.
Hostile-only eligible checks should produce relation calls but no aim/capsule calls.
Synthetic tests cover readiness, hostile exits with an invalid capsule, both allowed
relationships, relationship changes between shots, clear shots, invalid ranges,
unsupported shooters and missing players. Real game performance is not yet measured.

Other work-reduction options considered: per-shot data reuse is implemented;
spatial broadphase needs a verified conservative bound on muzzle position before
it can safely replace aim work. Cross-shot relationship caching risks stale hostility;
periodic sampling risks missing the player entering a firing line. Engine object calls
remain on the firing thread. Moving these calls to a worker would require an engine
thread-safety guarantee. File output already runs on the background worker and
release diagnostics are disabled by default.


## 0.4.3 relationship identity correction

The old relation call incorrectly supplied gameplay object addresses. The game's
own damage code at RVA 0xad1c37 calls RVA 0x4d61a4 for each participant before
calling the relationship-score routine at RVA 0x2358a77. The classification wrapper
at RVA 0x235809b calls that same score routine. Its keys are 64-bit identities;
the score routine orders them, hashes them, and searches its pair cache.
The resolver reads the NPC identity at gameplay+0x120; player identity uses a
special initialized singleton. We now call the same resolver for both participants.
This corrects the argument contract without guessing the player's special key.
Passing object addresses could select nonexistent pairs; indirect performance
consequences are possible, but these disassemblies do not prove a pause cause.

CaptureRelationships=1 implies metrics and writes cumulative relationship_value_N
and withheld_relationship_N rows to native_metrics.csv every five seconds on the
worker. No per-shot file writes, reverse probes, or persistent object references.
Relation timing now includes both identity resolver calls. Raw values are captured
for every resolved supported shooter, and withheld values count actual suppression.
The policy remains exactly values 2 and 3; values must be validated against known
hostile, neutral, and friendly subjects. No performance conclusion from old logs
should assume that their relationship classifications were valid.

Next test: repeat the known-hostile encounter without changing faction or using
neutral spawns. Note the approximate combat start/end. Wait six seconds after
combat before exiting to flush cumulative metrics. Then run a separate known
neutral/friendly obstruction test to verify protection still works with correct keys.
