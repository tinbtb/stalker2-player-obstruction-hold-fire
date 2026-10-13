# Player Obstruction Hold Fire — v0.4.8-focused-enemy-test2

[Releases / Завантаження / Скачать](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire/releases)

## English

**AI-generated code: OpenAI Codex, directed by tinbtb.** Neutral/friendly NPCs
hold fire when you obstruct their predicted shot (player capsule + 40 cm).
NPCs targeting you can retaliate even if their relationship remains neutral.
Unknown targets allow firing. Neutral retaliation and hold/resume were tested;
Friendly/hostile regression tests for this version remain pending.

Copy the ZIP contents directly into `Stalker2/Binaries/Win64`; disable older
ASI/UE4SS copies of this mod. The with-loader ZIP includes optional Ultimate ASI
Loader 9.7.4 x64 as `winhttp.dll`: use it only if that file is absent.
**Do not overwrite an existing proxy DLL.** Any compatible x64 ASI loader may
be used; keep ASI and INI together in its plugin folder. Install one copy.
UE4SS is not required; for other mods the author uses its build from
[Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

Edit the adjacent INI and restart: `EnableHoldFire=0` disables protection;
`EnableMetrics=1` writes `native_metrics.csv`; `CaptureEngineFrames=1` adds
`engine_frames.csv` (game-thread intervals, not GPU/display timings);
`CaptureRelationships=1` adds relationship/focus counters and `focused_targets.csv`.
Both capture options also enable metrics. Diagnostics default to **off**.
Preserve logs between runs. Startup log: ACTIVE = enabled, REFUSED = not installed.

Already-fired bullets, spread, ricochets and explosives remain dangerous.
The conservative check may hold fire while you are behind the intended target.
Unknown/invalid data allows firing; unexplained long pauses remain under investigation.
To uninstall, close the game and remove ASI/INI; remove the loader only if unused
by other mods. Loader license: `Ultimate-ASI-Loader-LICENSE.txt`.

## Українська

**Код згенеровано ШІ: OpenAI Codex під керівництвом tinbtb.** Нейтральні/дружні
NPC утримують вогонь, коли ви перекриваєте прогнозовану траєкторію (капсула + 40 см).
NPC, чия ціль — ви, можуть стріляти у відповідь навіть за нейтрального ставлення.
За невідомої цілі стрільбу дозволено. Помсту нейтральних NPC і зупинку/відновлення
вогню перевірено; перевірка дружніх/ворожих NPC для цієї версії ще очікується.

Скопіюйте вміст ZIP у `Stalker2/Binaries/Win64`; вимкніть старі ASI/UE4SS-копії
цього моду. Необов'язковий завантажувач `winhttp.dll` (Ultimate ASI Loader 9.7.4)
додавайте лише якщо такого файлу немає. **Не замінюйте наявні проксі-DLL.**
Підійде будь-який сумісний x64 ASI-завантажувач: ASI та INI мають бути разом.
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
остановка/возобновление проверены; проверки дружественных/враждебных NPC ещё ожидаются.

Скопируйте содержимое ZIP в `Stalker2/Binaries/Win64`; отключите старые ASI/UE4SS-копии
этого мода. Необязательный загрузчик `winhttp.dll` (Ultimate ASI Loader 9.7.4)
добавляйте только если такого файла нет. **Не заменяйте существующие прокси-DLL.**
Подойдёт любой совместимый x64 ASI-загрузчик: ASI и INI должны лежать вместе.
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

## EXE / SHA-256

`Stalker2-Win64-Shipping.exe` — 174798384 bytes / байтів / байт:
`61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`

[Source / Код](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire) ·
[Build](BUILD.md) · [Technical notes](TECHNICAL.md)
