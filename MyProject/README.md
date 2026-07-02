# MyProject

## Что это за проект

Это UE5-проект с лобби, игровой сценой, добычей руды, торговцем, прокачкой персонажа, меню паузы, настройками и JSON-сохранением.

Основной текущий сценарий:

1. Проект стартует с карты `NewMap`.
2. На `NewMap` показывается стартовое лобби.
3. Из лобби игрок переходит на игровую карту `Showcase`.
4. На игровой карте игрок добывает руду, получает ресурсы, торгует и прокачивает персонажа.

## Журнал разработки

### 2026-04-14

- стартовое лобби переработано в более полноценный стартовый экран с обзором игрового цикла и быстрым доступом к настройкам
- настройки разделены на вкладки `Audio / Controls / Interface`
- добавлены отдельные настройки звука `Master / Music / SFX`
- добавлены настройки управления:
  чувствительность мыши
  invert Y
  полное переназначение основных клавиш из UI
- меню-настройки и бинды сохраняются в `Settings.json`
- оптимизирована верстка стартового лобби:
  длинные тексты ужаты
  карточки не наезжают друг на друга
  summary по управлению использует более компактные подписи клавиш
- меню паузы дополнительно поджато под текущий scale, чтобы кнопки и заголовок не съезжали
- HP bar руды теперь скрывается, когда открыто pause/settings/trade/progression меню
- начата чистка старых sample-анимаций и sandbox-ассетов перед повторным импортом нового пака
- временно тестировался перенос части `GameAnimationSample`, но лишний sandbox-хвост затем вычищен из проекта
- в проект возвращён только минимальный источник sample-анимаций:
  `Characters/UEFN_Mannequin`, `ABP_GenericRetarget`, `IK_Ch44_Retarget`, `RTG_UEFN_to_Ch44`
- дополнительно возвращены лёгкие data-зависимости для будущей встройки:
  `Blueprints/Data` и `Blueprints/MovementModes`
- дополнительно перенесены skin-наборы `Echo` и `TwinBlast` с их рабочими Blueprint-обёртками
  `BP_Echo` и `BP_Twinblast`, чтобы потом использовать их в магазине или системе выбора скинов
- sample-персонажи, `GM_Sandbox`, `GameAnimationWidget` и остальной sandbox-каркас в рабочий путь не включены
- основной боевой путь оставлен на `Bp_MyThifCatcher`, без обязательной зависимости от `GM_Sandbox`
- в `AThifCatcher` добавлен прямой проигрыш `AM_Pickaxe_Attack_Montage` для удара киркой
- анти-спам удара, блокировка движения, hit-timing и урон по руде продолжают управляться существующей C++ логикой игрока

## Важные карты

- Стартовая карта: `C:\RPGFARM_MAXGODDYK\MyProject\Content\NewMap.umap`
- Игровая карта: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Namaqualand\Levels\Showcase.umap`

Это также прописано в:

- `C:\RPGFARM_MAXGODDYK\MyProject\Config\DefaultEngine.ini`

Текущие ключевые настройки:

- `GameDefaultMap=/Game/NewMap.NewMap`
- `EditorStartupMap=/Game/NewMap.NewMap`
- `GlobalDefaultGameMode=/Script/MyProject.ThiefCatcerGameMode`

## Основные классы

### Игрок

- C++ класс игрока: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\ThifCatcher.h`
- Реализация: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\ThifCatcher.cpp`
- Главный Blueprint игрока: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\Bp_MyThifCatcher.uasset`

Что сейчас умеет игрок:

- движение
- спринт
- расход и восстановление стамины
- удар по ЛКМ
- урон по руде
- получение ресурсов за разрушение руды
- хранение золота, руды, опыта, уровня и очков улучшений
- открытие прокачки

### Анимации игрока

- Anim instance: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\PlayerAnimInstance.h`
- Реализация: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\PlayerAnimInstance.cpp`
- Основной Anim BP: `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\Animations\BPA_MainMavements.uasset`

Текущая рабочая схема:

- `IdleMove`
- `Jump`
- `Attack`

Для атаки используется state `Attack` внутри `Locomotion`.

Дополнительно основной удар теперь может напрямую проигрывать `AM_Pickaxe_Attack_Montage` из C++ класса `AThifCatcher`, поэтому базовая атака не зависит от sandbox-режима.

### Камень / руда

- C++ база руды: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\Actors\OreStoneBase.h`
- Реализация: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\Actors\OreStoneBase.cpp`
- Blueprint руды: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\BP_OreStone.uasset`

Что умеет руда:

- HP bar над камнем
- получение урона
- разрушение
- скрытие после разрушения
- респавн через заданное количество минут
- выдача ресурса игроку

### Торговец

- Класс торговца: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\Actors\TraderNPC.h`
- Реализация: `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\Actors\TraderNPC.cpp`

Торговец:

- показывает подсказку при подходе
- открывает торговый UI
- позволяет продавать руду
- позволяет покупать зелье стамины

### GameMode

- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\ThiefCatcerGameMode.h`
- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\ThiefCatcerGameMode.cpp`

Что делает:

- стартует лобби
- связывает HUD и контроллер
- используется как основной GameMode проекта

### PlayerController

- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\ThiefPlayerController.h`
- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\ThiefPlayerController.cpp`

Что сейчас лежит в контроллере:

- логика лобби
- логика загрузочного экрана
- логика паузы
- логика настроек
- логика торговли
- логика прокачки
- язык интерфейса
- музыка меню

### HUD

- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Public\UI\PlayerGameHUD.h`
- `C:\RPGFARM_MAXGODDYK\MyProject\Source\MyProject\Private\UI\PlayerGameHUD.cpp`

HUD сейчас рисует:

- стартовое лобби
- loading screen
- pause menu
- settings menu
- stamina bar
- панель ресурсов `Ore / Gold`
- панель уровня и опыта
- UI торговца
- UI прокачки
- подсказки взаимодействия

## Что уже реализовано

### 1. Стартовое лобби

Есть стартовое лобби на `NewMap`.

В лобби уже есть:

- старт игры
- переработанный стартовый экран с описанием игрового цикла
- настройки по вкладкам
- выход из игры
- музыка меню
- загрузочный экран перед переходом на игровую карту
- быстрый обзор текущих настроек звука и управления прямо в лобби

### 2. Добыча руды

Есть:

- HP bar у руды
- показ HP bar только рядом с игроком
- урон по руде ударом
- респавн руды
- настраиваемая задержка респавна в минутах

Основные настройки у `BP_OreStone`:

- `MaxHealth`
- `RespawnDelayMinutes`
- `OreResourceReward`

### 3. Игрок

Есть:

- стамина
- stamina bar
- удар
- блок движения во время атаки
- добыча ресурсов
- золото
- уровень
- опыт
- очки улучшений

### 4. Торговля

Есть торговый UI.

Текущие действия:

- продать 1 руду
- продать 5 руды
- купить зелье стамины

### 5. Прокачка персонажа

Есть UI прокачки.

Текущие улучшения:

- увеличить максимум стамины
- увеличить урон по руде
- увеличить скорость

### 6. Сохранение через JSON

Есть сохранение:

- настроек
- характеристик игрока

Файлы сохранения:

- `C:\RPGFARM_MAXGODDYK\MyProject\Saved\JsonSaves\Settings.json`
- `C:\RPGFARM_MAXGODDYK\MyProject\Saved\JsonSaves\Character.json`

Настройки сейчас сохраняют:

- master volume
- music volume
- sfx volume
- масштаб меню
- язык
- чувствительность мыши
- invert Y
- переназначенные клавиши управления

Персонаж сейчас сохраняет:

- текущую и максимальную стамину
- количество руды
- золото
- уровень
- опыт
- очки улучшений
- урон по руде
- скорости движения

## Управление

Текущие базовые бинды в `C:\RPGFARM_MAXGODDYK\MyProject\Config\DefaultInput.ini`:

- `WASD` — движение
- `MouseX / MouseY` — поворот камеры
- `Space` — прыжок
- `LeftShift` — спринт
- `LeftMouseButton` — удар

Дополнительные игровые действия обрабатываются через input mappings:

- `E` — взаимодействие / торговец
- `B` — быстрое открытие торговли
- `P` — прокачка
- `Enter` — старт / подтверждение
- `Esc` — меню / выход из окон
- `1 / 2 / 3` — быстрые действия в UI

Важно:

- эти бинды теперь можно переназначать прямо из стартового лобби
- сохраняется не только сама клавиша, но и настройки мыши `sensitivity / invert Y`

## Важное замечание по Esc в редакторе

Если во время `PIE` нажатие `Esc` завершает игру, а не открывает меню, это не баг проекта, а горячая клавиша самого Unreal Editor.

Нужно проверить `Editor Preferences -> Keyboard Shortcuts` и убрать `Escape` у:

- `Play World (PIE/SIE) -> Stop`

Иначе `Esc` будет останавливать `PIE`.

## Важные Blueprint-ассеты

### Основные

- Игрок: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\Bp_MyThifCatcher.uasset`
- Руда: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\BP_OreStone.uasset`
- Sandbox GameMode: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\GM_Sandbox.uasset`
- Sandbox Visual Override игрок: `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\RetargetedCharacters\BP_OurCharacter.uasset`

### Анимации игрока

- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\Animations\BPA_MainMavements.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\Animations\BS_RemyAnimations.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\Animations\AM_Pickaxe_Attack.uasset`

### UMG fallback-виджеты

- `C:\RPGFARM_MAXGODDYK\MyProject\Content\WBP_ResourceCounter.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\WBP_StaminaBar.uasset`

## Что за папки в Content

### Свои игровые данные

- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Audio`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Widgets`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Namaqualand`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\DEMO_MiningPack`

### Референсы и AAA sample

- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Locomotor`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Characters`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\RetargetedCharacters`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Levels\LocomotorLevel.umap`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Levels\NPCLevel.umap`

Эти папки нужны как референс и AAA-база. Основной игровой поток сейчас не должен зависеть от `GM_Sandbox`, если работа ведётся над обычным gameplay.

## Текущий статус по задачам куратора

Исходные задачи:

- показать фикс того, что слетело
- до апреля добавить торговлю (UI)
- добавить прокачку персонажа
- сделать начальное меню для перехода в игровую сцену

### Уже есть

- начальное меню
- загрузочный экран
- торговля через UI
- прокачка персонажа через UI
- добыча руды
- базовые сохранения JSON

### Что ещё логично улучшать

- более красивый UI торговца и прокачки
- чистая AAA-анимационная ветка без experimental-хвостов
- сохранение прогресса карты / разрушенной руды по сессиям
- отдельные звуковые классы / submix routing для ещё более точного разделения ambience / UI / gameplay

## Известные сложные места

### 1. AAA sample ветка

В проект импортирован `GameAnimationSample` и часть его логики.

Файлы:

- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\SandboxCharacter_CMC.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\SandboxCharacter_CMC_ABP.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\GM_Sandbox.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\RetargetedCharacters\ABP_GenericRetarget.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\Blueprints\RetargetedCharacters\BP_OurCharacter.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\RTG_UEFN_to_Ch44.uasset`
- `C:\RPGFARM_MAXGODDYK\MyProject\Content\MainPlayer\IK_Ch44_Retarget.uasset`

Это экспериментальная ветка для AAA-анимаций. Её лучше трогать отдельно от основной gameplay-ветки игрока, чтобы не сломать текущий рабочий сценарий.

Что уже настроено в этой ветке:

- наш `Ch44` подключён как `Visual Override`
- `ABP_GenericRetarget` уже знает про `RTG_UEFN_to_Ch44`
- в `BP_OurCharacter` удар киркой висит на `LMB`
- повторный старт удара блокируется, пока текущий монтаж не завершится

### 2. Некоторые UI-правки шли в C++

После изменений в коде почти всегда нужно:

1. перезапустить Unreal Editor
2. открыть нужный Blueprint
3. нажать `Compile`
4. нажать `Save`

## Как запускать и проверять

### Проверка лобби

1. Открыть проект.
2. Убедиться, что стартует `NewMap`.
3. Проверить стартовое меню.
4. Проверить переход в `Showcase`.

### Проверка руды

1. Поставить `BP_OreStone` на карту.
2. Настроить `MaxHealth`, `RespawnDelayMinutes`, `OreResourceReward`.
3. Запустить игру.
4. Проверить удар, разрушение, респавн и начисление ресурса.

### Проверка игрока

1. Открыть `Bp_MyThifCatcher`.
2. Проверить `Anim Class`.
3. Проверить `AttackRange`, `OreDamage`, тайминги атаки.
4. Проверить stamina bar, уровень, Ore/Gold.

### Проверка sandbox-ветки

1. Открыть карту `C:\RPGFARM_MAXGODDYK\MyProject\Content\Levels\LocomotorLevel.umap`
2. Убедиться, что в `World Settings` стоит `GM_Sandbox`
3. Запустить `PIE`
4. В `GameAnimationWidget` открыть `Visual Override`
5. Выбрать `BP_OurCharacter`
6. Проверить locomotion и удар киркой по `LMB`

### Проверка торговли

1. Поставить `TraderNPC` на игровую карту.
2. Подойти к торговцу.
3. Проверить подсказку взаимодействия.
4. Открыть окно торговли.

## Что важно сказать новому чату

Если работа продолжится в новом чате, начать нужно с такого контекста:

- проект UE5
- стартовая карта `NewMap`
- игровая карта `Showcase`
- основной игрок: `Bp_MyThifCatcher`
- основная анимация игрока: `BPA_MainMavements`
- руда: `BP_OreStone` на базе `OreStoneBase`
- торговец: `TraderNPC`
- HUD рисуется через `PlayerGameHUD`
- настройки и характеристики сохраняются в JSON
- в стартовом лобби уже есть вкладки `Audio / Controls / Interface`
- в лобби уже работает переназначение клавиш и отдельные уровни громкости `Master / Music / SFX`
- `GameAnimationSample` уже импортирован, но это отдельная AAA-ветка, не основной gameplay flow

## Что можно брать следующим шагом

Самые логичные следующие задачи:

1. Полировка UI торговца и прокачки под общий стиль нового лобби.
2. Отдельный звуковой pipeline для `ambience / UI / gameplay`, если понадобится ещё глубже разделить звук.
3. Чистая интеграция AAA-анимаций в основной игровой персонаж.
4. Сохранение состояния карты и разрушенной руды между сессиями.
5. Добавление визуальной обратной связи на успешное переназначение клавиш и тестовых звуков в настройках.

## Служебные заметки

- Если после C++ правок что-то "не появляется", почти всегда сначала нужен полный перезапуск редактора.
- Если `Esc` закрывает игру в `PIE`, смотреть не проект, а hotkey редактора.
- Если UI не виден, проверять сначала спавн правильного `Pawn` и `GameMode`, потом перезапуск редактора, потом `Compile/Save` у BP.
