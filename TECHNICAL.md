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
