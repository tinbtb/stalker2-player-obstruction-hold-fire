# Player Obstruction Hold Fire — v0.4.0-testing

[Releases / Завантаження / Скачать](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire/releases)

```ini
[Debug]
EnableHoldFire=1
EnableMetrics=0
CaptureEngineFrames=0
```

## English

Neutral/friendly NPCs hold fire when you obstruct their predicted shot (capsule +
40 cm margin). **AI-generated code: OpenAI Codex, directed by tinbtb.** Native test
builds were tested in-game; this release is experimental. UE4SS is not required.
For other mods, the author uses its build from [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

**Install:** close the game and disable old ASI/UE4SS copies of this mod. Extract
the root-level files into `Stalker2/Binaries/Win64`. The with-loader ZIP includes optional
Ultimate ASI Loader 9.7.4 x64 as `winhttp.dll`: put it in Win64 only if that file
is absent. **Do not overwrite existing proxy DLLs.** Any compatible x64 ASI loader
may be used; place ASI and INI together in its plugin folder. An ASI-only ZIP is
available. Install exactly one copy.

**Settings:** edit the adjacent INI and restart. `EnableHoldFire=0` disables
protection for a control run. `EnableMetrics=1` writes counters/timings to
`native_metrics.csv`. `CaptureEngineFrames=1` writes game-thread tick intervals to
`engine_frames.csv` and enables metrics; these are not GPU/display frame timings.
Diagnostics are **off by default**. Preserve logs before another diagnostic run.

The startup log should say ACTIVE; REFUSED means incompatible EXE/conflict.
In-flight bullets, wider dispersion and other damage can still hurt you. Invalid
data permits the original shot. To uninstall, close the game and remove ASI/INI;
remove the optional loader only if no other mod needs it. Loader license: `Ultimate-ASI-Loader-LICENSE.txt`.

## Українська

Нейтральні та дружні NPC припиняють вогонь, коли ви перекриваєте прогнозовану
траєкторію пострілу (капсула + 40 см). **Код згенеровано ШІ: OpenAI Codex під
керівництвом tinbtb.** Нативні тестові збірки перевірено у грі; реліз
експериментальний. UE4SS не потрібен. Для інших модів автор використовує його збірку
з [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

**Встановлення:** закрийте гру й вимкніть старі ASI/UE4SS-копії цього моду.
Розпакуйте файли з кореня архіву у `Stalker2/Binaries/Win64`. Архів із завантажувачем містить
необов'язковий Ultimate ASI Loader 9.7.4 x64 — `winhttp.dll`: додайте його у Win64,
лише якщо такого файлу немає. **Не замінюйте наявні проксі-DLL.** Можна використати
будь-який сумісний x64 ASI-завантажувач; покладіть ASI та INI разом у його теку
модулів. Є архів без завантажувача. Залиште лише одну копію.

**Налаштування:** змініть INI поруч з ASI та перезапустіть гру. `EnableHoldFire=0`
вимикає захист для контрольного запуску. `EnableMetrics=1` записує лічильники й час
виконання у `native_metrics.csv`. `CaptureEngineFrames=1` записує інтервали тактів
ігрового потоку в `engine_frames.csv` і вмикає метрики; це не час GPU чи відображення
кадрів. Діагностику **типово вимкнено**. Зберігайте журнали між тестами.

У стартовому журналі має бути ACTIVE; REFUSED означає несумісний EXE або конфлікт.
Кулі в польоті, більший розкид та інша шкода залишаються небезпечними. Некоректні
дані дозволяють звичайний постріл. Для видалення закрийте гру й видаліть ASI/INI;
завантажувач видаляйте лише якщо він не потрібен іншим модам. Ліцензія — `Ultimate-ASI-Loader-LICENSE.txt`.

## Русский

Нейтральные и дружественные NPC прекращают огонь, когда вы перекрываете
прогнозируемую траекторию выстрела (капсула + 40 см). **Код создан ИИ: OpenAI Codex
под руководством tinbtb.** Нативные тестовые сборки проверены в игре; релиз
экспериментальный. UE4SS не требуется. Для других модов автор использует его сборку
из [Ultra+ Manager](https://theultraplace.com/tools/ultra-plus-manager/).

**Установка:** закройте игру и отключите старые ASI/UE4SS-копии этого мода.
Распакуйте файлы из корня архива в `Stalker2/Binaries/Win64`. Архив с загрузчиком включает
необязательный Ultimate ASI Loader 9.7.4 x64 — `winhttp.dll`: добавьте его в Win64,
только если такого файла нет. **Не заменяйте существующие прокси-DLL.** Можно
использовать любой совместимый x64 ASI-загрузчик; разместите ASI и INI вместе в его
папке плагинов. Есть архив без загрузчика. Оставьте одну копию.

**Настройки:** измените INI рядом с ASI и перезапустите игру. `EnableHoldFire=0`
отключает защиту для контрольного запуска. `EnableMetrics=1` записывает счётчики и
время выполнения в `native_metrics.csv`. `CaptureEngineFrames=1` записывает интервалы
тактов игрового потока в `engine_frames.csv` и включает метрики; это не время GPU
или отображения кадров. Диагностика **по умолчанию выключена**. Сохраняйте журналы
между тестами.

В стартовом журнале должно быть ACTIVE; REFUSED означает несовместимый EXE или
конфликт. Летящие пули, больший разброс и другой урон остаются опасными. Некорректные
данные разрешают обычный выстрел. Для удаления закройте игру и удалите ASI/INI;
загрузчик удаляйте только если он не нужен другим модам. Лицензия — `Ultimate-ASI-Loader-LICENSE.txt`.

## EXE / SHA-256

`Stalker2-Win64-Shipping.exe` — 174798384 bytes / байтів / байт:
`61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b`

[Source / Код](https://github.com/tinbtb/stalker2-player-obstruction-hold-fire) ·
[Build](BUILD.md) · [Technical notes](TECHNICAL.md)
