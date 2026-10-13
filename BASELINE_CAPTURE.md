# Performance capture

See README.md for current INI switches. Use EnableMetrics=1 and CaptureEngineFrames=1
for an active run, then repeat with EnableHoldFire=0 for the control. Archive all
outputs between launches. Keep saves, route, NPC types/counts and settings matched.

Counter and timing totals are cumulative. Subtract adjacent totals; do not add nested
timers to the full native gate. max_us is a session maximum. Frame intervals measure
game-thread tick cadence rather than GPU/display FPS. Treat loading/spawning separately.
Report median, p95, p99, p99.9, maximum and counts above 50/100/1000 ms in equal-length
combat windows. Quantiles from short captures are noisy; repeat if conclusions depend
on a few extreme samples. No raw tester captures are included in this repository.
