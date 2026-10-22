# v0.5.0-beta.2 — Additional native compatibility family

- Recognize a second audited compiler layout, including the outlined aim fallback helper. Require a complete, consistent family before installing hooks.
- Keep startup-only validation, existing firing behavior, and clean ZIP layout. No runtime scanning or UE4SS dependency.
- Author confirmed the new ASI works in-game on Steam v2.0.6. The development-only executable passes offline validation and mutation/relocation tests; its gameplay remains untested.
- Other executable builds may work when their relevant native code matches a validated family. Arbitrary future compiler/layout changes still require review.

# v0.5.0-beta.1 — Startup compatibility and cleaner packages

- Replace the whole-executable hash gate with one startup scan of relevant native functions, layout witnesses, call relationships, and global references.
- Reject missing, changed, or ambiguous matches before installing hooks. Resolve the optional frame-capture hook as well.
- Keep firing/focus policy and geometry unchanged; no runtime scanning or polling is added.
- Put all supporting files, including the optional debug INI, under PlayerObstructionHoldFire/. Only the ASI and optional winhttp.dll remain at the ZIP root.
- Read the new INI location first, with legacy adjacent-file fallback. Migrate custom settings when replacing a Vortex installation.

Tested in-game on Steam v2.0.6: friendlies hold fire, enemies fire normally, and friendly-to-hostile transitions work. Future game updates still need testing. The historical release notes below describe the previous binary.

# v0.4.8 — Fix provoked-neutral return fire (testing)

- Use the game's GetFocusedEnemy getter: neutral/friendly NPCs targeting the player may fire even while their relationship stays neutral.
- Only a known different enemy can trigger hold-fire; unknown targets allow firing.
- Add optional focus counters/samples and getter timings. Diagnostics are off by default.

Neutral retaliation and obstruction hold/resume were verified in-game. Friendly/hostile regression checks for this version remain pending. Long pauses and conservative behind-target suppression remain unresolved.

The previous release provided two flat ZIPs: ASI-only and ASI with optional official winhttp.dll loader. Both include short EN/UA/RU instructions and checksums. The ASI is byte-identical to the tested 0.4.8-focused-enemy-test2 binary; its test identifier is retained.
