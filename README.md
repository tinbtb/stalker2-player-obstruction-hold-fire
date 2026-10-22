# Player Obstruction Hold Fire — v0.5.0-beta.2

The [two-family resolver](COMPATIBILITY_FAMILIES.md) validates both the Steam and development-only executable layouts offline. The author confirmed the new ASI works in-game on Steam v2.0.6. Development-fixture gameplay remains untested. Detailed behavior checks below were recorded for the earlier v0.5.0-beta.1 build.

[Releases / Завантаження / Скачать](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire/releases)

Tested in-game on Steam v2.0.6: friendlies hold fire, enemies fire normally, and friendly-to-hostile transitions work. Startup resolution replaces the whole-file hash gate; missing, changed, or ambiguous relevant native code disables the hooks. Address movement may be tolerated, but future game updates still require testing.

Only `PlayerObstructionHoldFire.asi` and optional `winhttp.dll` are at the ZIP root. Configuration, documentation, licenses, manifests, and checksums are under `PlayerObstructionHoldFire/`. Replace the previous Vortex installation and redeploy to remove old supporting files. Copy custom INI settings into the new subfolder before starting the game. A root-level INI is used only when the subfolder INI is absent.

## English

**AI-generated code: OpenAI Codex, directed by tinbtb.** Neutral/friendly NPCs
hold fire when you obstruct their predicted shot (player capsule + 40 cm).
NPCs targeting you can retaliate even if their relationship remains neutral.
Unknown targets allow firing. Neutral retaliation and hold/resume were tested;
Friendly/hostile behavior and transitions were verified in-game.

Copy the ZIP contents directly into `Stalker2/Binaries/Win64`; disable older
ASI/UE4SS copies of this mod. The with-loader ZIP includes optional Ultimate ASI
Loader 9.7.4 x64 as `winhttp.dll`: use it only if that file is absent.
**Do not overwrite an existing proxy DLL.** Any compatible x64 ASI loader may
be used; keep the PlayerObstructionHoldFire subfolder beside the ASI. Install one copy.
UE4SS is not required; for other mods the author uses its build from
[Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

Edit `PlayerObstructionHoldFire/PlayerObstructionHoldFire.ini` and restart: `EnableHoldFire=0` disables protection;
`EnableMetrics=1` writes `native_metrics.csv`; `CaptureEngineFrames=1` adds
`engine_frames.csv` (game-thread intervals, not GPU/display timings);
`CaptureRelationships=1` adds relationship/focus counters and `focused_targets.csv`.
Both capture options also enable metrics. Diagnostics default to **off**.
Preserve logs between runs. Startup log: ACTIVE = enabled, REFUSED = not installed.

Already-fired bullets, spread, ricochets and explosives remain dangerous.
The conservative check may hold fire while you are behind the intended target.
Unknown/invalid data allows firing; unexplained long pauses remain under investigation.
To uninstall, close the game and remove ASI/INI; remove the loader only if unused
by other mods. Loader license: `PlayerObstructionHoldFire/Ultimate-ASI-Loader-LICENSE.txt`.

## Українська

**Код згенеровано ШІ: OpenAI Codex під керівництвом tinbtb.** Нейтральні/дружні
NPC утримують вогонь, коли ви перекриваєте прогнозовану траєкторію (капсула + 40 см).
NPC, чия ціль — ви, можуть стріляти у відповідь навіть за нейтрального ставлення.
За невідомої цілі стрільбу дозволено. Помсту нейтральних NPC і зупинку/відновлення
вогню перевірено; поведінку дружніх/ворожих NPC та переходи між ними перевірено.

Скопіюйте вміст ZIP у `Stalker2/Binaries/Win64`; вимкніть старі ASI/UE4SS-копії
цього моду. Необов'язковий завантажувач `winhttp.dll` (Ultimate ASI Loader 9.7.4)
додавайте лише якщо такого файлу немає. **Не замінюйте наявні проксі-DLL.**
INI, інструкції та ліцензії містяться в підпапці `PlayerObstructionHoldFire/` поруч з ASI.
Залиште одну копію. UE4SS не потрібен; для інших модів автор використовує збірку
з [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

Змініть INI та перезапустіть гру: `EnableHoldFire=0` вимикає захист;
`EnableMetrics=1` записує метрики; `CaptureEngineFrames=1` — інтервали ігрового
потоку, а не GPU; `CaptureRelationships=1` — відносини, цілі й зразки у
`focused_targets.csv`. Обидва режими захоплення вмикають метрики. Діагностику
**типово вимкнено**. Зберігайте журнали. ACTIVE — мод працює; REFUSED — не встановлено.

Кулі в польоті, розкид, рикошети й вибухи залишаються небезпечними. Мод може
утримувати вогонь і коли ви стоїте за ціллю. За невідомих/некоректних даних
стрільбу дозволено; причини тривалих пауз ще досліджуються. Для видалення закрийте
гру й видаліть ASI/INI; завантажувач — лише якщо він не потрібен іншим модам.

## Русский

**Код создан ИИ: OpenAI Codex под руководством tinbtb.** Нейтральные/дружественные
NPC удерживают огонь, когда вы перекрываете прогнозируемую траекторию (капсула + 40 см).
NPC, чья цель — вы, могут стрелять в ответ даже при нейтральном отношении.
При неизвестной цели стрельба разрешена. Ответный огонь нейтральных NPC и
остановка/возобновление проверены; поведение дружественных/враждебных NPC и переходы проверены.

Скопируйте содержимое ZIP в `Stalker2/Binaries/Win64`; отключите старые ASI/UE4SS-копии
этого мода. Необязательный загрузчик `winhttp.dll` (Ultimate ASI Loader 9.7.4)
добавляйте только если такого файла нет. **Не заменяйте существующие прокси-DLL.**
INI, инструкции и лицензии находятся в подпапке `PlayerObstructionHoldFire/` рядом с ASI.
Оставьте одну копию. UE4SS не требуется; для других модов автор использует сборку
из [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

Измените INI и перезапустите игру: `EnableHoldFire=0` отключает защиту;
`EnableMetrics=1` записывает метрики; `CaptureEngineFrames=1` — интервалы игрового
потока, не GPU; `CaptureRelationships=1` — отношения, цели и образцы в
`focused_targets.csv`. Оба режима захвата включают метрики. Диагностика
**по умолчанию выключена**. Сохраняйте журналы. ACTIVE — мод работает; REFUSED — не установлен.

Летящие пули, разброс, рикошеты и взрывы остаются опасными. Мод может удерживать
огонь и когда вы стоите за целью. При неизвестных/некорректных данных стрельба
разрешена; причины длительных пауз ещё исследуются. Для удаления закройте игру
и удалите ASI/INI; загрузчик — только если он не нужен другим модам.

## Tested baseline / Перевірена версія / Проверенная версия

Steam v2.0.6 baseline, not a runtime hash allowlist / не обмеження за хешем / не ограничение по хешу.

`Stalker2-Win64-Shipping.exe` — 174798384 bytes / байтів / байт:
`61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`

[Source / Код](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire) ·
[Build](BUILD.md) · [Technical notes](TECHNICAL.md)
