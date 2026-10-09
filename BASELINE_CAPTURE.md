# Baseline capture

Close the game, then install the diagnostic ZIP over the 40 cm mod. Replace both Lua and DLL. Do not hot reload. No cache, graphics or other mod settings should change.

For each scene (quiet area, populated settlement, sustained NPC combat):
1. Load the same save and warm up for 60 seconds.
2. Capture frame times for 120 seconds with your existing frame capture tool. Note the capture start/end time, hardware, graphics/frame generation settings, loader build and enabled mods.
3. Exit the game and copy Scripts/baseline_metrics.csv and Scripts/PlayerObstructionHoldFire.log to a folder named for the scene/run. The CSV is reset at next mod activation.
4. Repeat with the uninstrumented v0.2.1-testing build, and with this mod disabled, restarting the game each time. Keep everything else fixed. Use A/B/B/A order when comparing two configurations.

The CSV appends cumulative summaries approximately every 100 completed callbacks (normally five seconds), including unsuccessful snapshot callbacks. If the game thread stalls, summaries will also be delayed. A completely stalled callback cannot report until it resumes.

Metrics:
- lua_callback: game-thread callback body including its measured sub-stages; excludes queue wait and summary file output.
- controller_lookup: FindFirstOf only, including timer bridge overhead.
- capsule_reads: controller validation's subsequent pawn/component/property accesses and scaled capsule reads, for completed stages only. Early invalid returns or exceptions can make stage counts lower than callback counts.
- snapshot_write: snapshot file open/write/close and address formatting, for completed stages only.
- queue_wait: elapsed time from scheduling to game-thread callback entry; latency, not CPU work.
- native_gate_including_original: full gate wall duration including the original game readiness function and early-return paths. This is not solely mod overhead.
- native_snapshot_read: DLL snapshot parsing and object validation.

count and total_us are cumulative. Between two rows for the same metric, mean duration is delta(total_us)/delta(count), call rate is delta(count)/elapsed seconds, and measured time per second is delta(total_us)/elapsed seconds. max_us is the cumulative session maximum, including warm-up/loading; it is not an interval maximum. Timings are wall durations, not thread CPU times. Do not add callback duration to its nested stages, or treat queue wait as work. Native gate and callback totals can overlap in unusual reentrant execution; do not infer overall frame time by summing them.

Diagnostic timing and aggregate file output add overhead. Compare diagnostic FPS against the uninstrumented 40 cm build before attributing a slowdown. Capture external frame times for FPS/1% lows; this CSV does not measure rendering FPS. No per-shot disk logging is used. Keep raw capture files locally for analysis; no uploads or telemetry occur automatically.

## In-memory comparison

v0.2.2-memory-metrics replaces snapshot_write with snapshot_memory_transfer and native_snapshot_read with native_snapshot_commit. All other metric meanings remain. Compare this build with v0.2.1-metrics using the same scene and settings. The bridge uses 24 digit callbacks plus reset/commit per snapshot, without plain Lua API imports or a third-party cache dependency. Quantization error is at most 0.005 cm per dimension. Preserve both baseline CSVs. An old player_snapshot.txt may remain from previous versions but is not read or written.

## Controller cache comparison

Compare v0.2.3-cache-metrics against v0.2.2-memory-metrics in the same scene. controller_lookup now includes cache validation and any reacquisition, so compare its average and lua_callback totals. Extra rows controller_cache_hits, controller_cache_refreshes and snapshot_invalidations are cumulative event counts; their total_us/max_us are zero, not timing measurements.

Controller references are checked each callback and refreshed after 100 uses (approximately five seconds). Pawn and capsule references are read anew each update. Missing/invalid objects and callback errors discard the controller and invalidate the native snapshot immediately. Existing native serial/index/age validation remains. This does not depend on UObjectCacheMod.

Also test save reload, returning to menu/loading another save, and death/reload. Confirm snapshot successes resume, withholding still works and unavailable-player counts do not keep growing after gameplay resumes. Preserve the CSV before restarting. A valid but no-longer-active controller can persist until periodic refresh; single-player FindFirstOf selection remains an assumption inherited from the baseline.
