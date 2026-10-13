# v0.4.8 — Fix provoked-neutral return fire (testing)

- Use the game's GetFocusedEnemy getter: neutral/friendly NPCs targeting the player may fire even while their relationship stays neutral.
- Only a known different enemy can trigger hold-fire; unknown targets allow firing.
- Add optional focus counters/samples and getter timings. Diagnostics are off by default.

Neutral retaliation and obstruction hold/resume were verified in-game. Friendly/hostile regression checks for this version remain pending. Long pauses and conservative behind-target suppression remain unresolved.

Two flat ZIPs are provided: ASI-only and ASI with optional official winhttp.dll loader. Both include short EN/UA/RU instructions and checksums. The ASI is byte-identical to the tested 0.4.8-focused-enemy-test2 binary; its test identifier is retained.
