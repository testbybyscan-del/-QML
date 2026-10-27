# Boulder Dash на Qt 5.15 + QML

**Boulder Dash** — учебная игра на клеточном поле. Проект написан на C++ и QML; цель — освоить связку **C++ (логика) + QML (визуализация) + CMake (сборка) + VS Code (среда)**.

Исполняемый файл называется `boulderdash`, а C++-тип `GameEngine` регистрируется в QML под URI `BoulderDash`.

> Этот README содержит обзор проекта и справочные заметки. Пошаговые материалы для самостоятельной разработки собраны отдельно: [`LEARNING_01.md`](docs/LEARNING_01.md) → [`LEARNING_02.md`](docs/LEARNING_02.md) → [`LEARNING_03.md`](docs/LEARNING_03.md) → [`LEARNING_04.md`](docs/LEARNING_04.md) → [`LEARNING_05.md`](docs/LEARNING_05.md) → [`LEARNING_06.md`](docs/LEARNING_06.md) → [`LEARNING_07.md`](docs/LEARNING_07.md) → [`LEARNING_08.md`](docs/LEARNING_08.md) → [`LEARNING_9.md`](docs/LEARNING_9.md).

---

## Содержание

1. [Что это за проект](#что-это-за-проект)
2. [Требования и установка](#требования-и-установка)
3. [Сборка и запуск](#сборка-и-запуск)
4. [Структура проекта](#структура-проекта)
5. [Управление в игре](#управление-в-игре)
6. [Библиотека визуальных тайлов](#библиотека-визуальных-тайлов)
7. [Варианты Персонажа](#варианты-персонажа)
8. [Проверки](#проверки)
9. [Как это работает: C++ ↔ QML](#как-это-работает-c--qml)
10. [Шпаргалка по QML](#шпаргалка-по-qml)
11. [Шпаргалка по C++/Qt](#шпаргалка-по-cqt)
12. [Шпаргалка по CMake](#шпаргалка-по-cmake)
13. [Отладка в VS Code](#отладка-в-vs-code)
14. [Работа с Git](#работа-с-git)
15. [Учебный план: что учить и в каком порядке](#учебный-план-что-учить-и-в-каком-порядке)
16. [Полезные ресурсы](#полезные-ресурсы)
17. [Что делать, если что-то не работает](#что-делать-если-что-то-не-работает)
18. [Лицензия](#лицензия)

---

## Что это за проект

В текущей реализации Персонаж перемещается по сетке, собирает обязательные Призы, толкает Кристаллы и копает кирпичи. После сбора всех обязательных Призов нижний правый выход становится доступен и подсвечивается; переход происходит, когда Персонаж входит в него.

Проект реализует:

- **Пять ручных уровней в QML** и автоматически создаваемые пейзажи размером 20×13 клеток.
- **Персонажа** с управлением с клавиатуры, копанием и столкновениями; после гибели игра показывает счёт, рекорд, жизни, уровень, оставшиеся обязательные Призы и наличие Ключа, предлагая перезапуск или выход под спокойным сообщением.
- **Кристаллы, Призы и Суперпризы** с пошаговой физикой; падающий Кристалл убивает Персонажа. Кристалл падает через выкопанную клетку, если ниже свободно. Если он остаётся в яме при срабатывании таймера, то превращается в дополнительный Приз и остаётся там до захвата. Захват запускает новый полный таймер восстановления Кирпича; если в яме снова находится Кристалл, он также превращается в Приз. Цикл можно повторять, а дополнительные Призы не участвуют в условии перехода.
- **Ключ и Сейф**: Ключ `K` лежит на четвёртом уровне (строка 10, столбец 15), Сейф `B` — на втором (строка 8, столбец 10). При подходе к запертому Сейфу он мигает и один раз за посещение уровня появляется подсказка. Подбор Ключа и открытие Сейфа показывают отдельные окна; сообщения оформлены пиксельным шрифтом Unifont с поддержкой кириллицы. Персонаж переносит Ключ между экранами, открывает Сейф и получает 100 очков. HUD сразу отражает наличие Ключа.
- **Список объектов**: Персонаж, Кирпичи, Кристаллы, обязательные и дополнительные Призы, Суперпризы, выходы, порталы, Ключ и Сейф.
- **Генератор пейзажей** с воспроизводимым seed и проверкой кандидатов перед загрузкой.
- **Переходы между уровнями** с сохранением состояния при возвращении через левый портал.
- **HUD** со счётчиком Призов, очками, рекордом, жизнями и наличием Ключа; рекорд сохраняется через `QSettings` между запусками.
- **Три темы тайлов**: Classic, Kenney и Industrial для Кирпичей, Кристаллов и обычных Призов.
- **Четыре варианта Персонажа**: классический, зелёный космонавт, розовый космонавт и робот; оформление выбирается независимо от темы тайлов.
- **Галерею** без игрового движка: сравнение графики, размеров, анимации, действий и направлений Персонажа.
- **Меню и паузу**: начальный экран, продолжение, новая игра, настройки и выход. Пауза блокирует ввод и физику, сохраняет остаток времени восстановления каждого Кирпича.
- **Настройки оформления** через C++ `GameSettings` и `QSettings`: тема, герой, разрешение и скорость анимации сохраняются между запусками.
- **Автоматические проверки** настроек, игровой логики и QML-компонентов через Qt Test и CTest.

В проекте пока нет активных врагов в отображаемых уровнях: `EnemyModel` и QML-компонент существуют как задел. Графические спрайты тайлов и Персонажа уже подключены. Звука и сохранения состояния уровней на диск пока нет; между запусками сохраняются рекорд и настройки оформления. Прохождение, позиция и текущие очки не сохраняются.

---

## Требования и установка

### Что нужно установить

| Компонент | Версия | Зачем |
|-----------|--------|-------|
| **Qt** | 5.15.x | Core, Quick, Gui, Qml; Test для тестов (`Qt5::...`). Qt 6 текущей сборкой не поддерживается |
| **CMake** | 3.16+ | Система сборки |
| **Компилятор** | C++17 | Сборка проекта |
| **Ninja** | любая | Быстрый генератор сборки |
| **Git** | любая | Версионирование |
| **VS Code / Qt Creator** | необязательно | Редактор и отладка; сборка работает из терминала |

### Установка Qt на Linux (Ubuntu/Debian)

Для Ubuntu 24.04 можно установить инструменты сборки и используемые QML-модули из пакетов:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qtbase5-dev qtdeclarative5-dev \
    qml-module-qtquick2 qml-module-qtquick-window2 qml-module-qtqml qml-module-qtqml-models2
```

При другой установке Qt укажите путь к комплекту Qt 5.15 через `CMAKE_PREFIX_PATH`. Для уже подготовленного локального окружения H2 установка системных пакетов не требуется; команды приведены ниже.

### Расширения VS Code

- **CMake Tools** (Microsoft) — сборка через CMake.
- **C/C++** (Microsoft) или **clangd** — IntelliSense для C++.
- **Qt Extension Pack** — подсветка QML, интеграция с Qt.
- **QML** (от The Qt Company) — синтаксис и автодополнение QML.

---

## Сборка и запуск

### Из терминала

```bash
# Конфигурация проекта (Qt 5.15)
cmake -S . -B build-qt5 -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Сборка
cmake --build build-qt5 --parallel 2

# Запуск
./build-qt5/boulderdash
```

Или выполните `./build_and_run.sh`. Подробности установки зависимостей находятся в [`README_RUN.md`](README_RUN.md).

Тесты игровой логики и QML-компонентов собираются вместе с проектом (`BUILD_TESTING=ON` по умолчанию) и запускаются через CTest:

```bash
cmake --build build-qt5 --parallel 2
ctest --test-dir build-qt5 --output-on-failure
```

Руководство по игровым тестам: [`LEARNING_04.md`](docs/LEARNING_04.md), по тайлам и Персонажам: [`LEARNING_05.md`](docs/LEARNING_05.md), по настройкам, меню и паузе: [`LEARNING_06.md`](docs/LEARNING_06.md).

### Запуск с выбранного уровня

```bash
./build-qt5/boulderdash --level 6
./build-qt5/boulderdash --level 50
```

`--level N` сразу начинает игру с номера 1–50. Сейчас карты 1–6 авторские, последующие номера получают сгенерированные пейзажи; промежуточные карты подготавливаются для сохранения нумерации и последовательности seed. Тестовый старт задаёт нижнюю границу N: уход ниже неё запрещён, переход вперёд после сбора обязательных Призов и возврат с последующих карт до N разрешены. По мере подключения новых авторских карт их номера будут открывать эти карты. Без флага открывается обычное меню, «Новая игра» возвращает на уровень 1 и снимает ограничение тестового старта. Неверное значение и сочетание с `--tile-gallery` приводят к ошибке. Справка: `--help`. Разбор реализации и проверок — [LEARNING_08.md](docs/LEARNING_08.md).

### Галерея без запуска игры

```bash
./build-qt5/boulderdash --tile-gallery
```

### Локальное окружение H2 (`belka`)

На H2 зависимости распакованы в `/home/majorov/.local/share/qml-build-deps/root`. Для существующего каталога сборки:

```bash
cd /home/majorov/DEV/QML
source build-qt5/env.sh
cmake --build build-qt5 --parallel 2
ctest --test-dir build-qt5 --output-on-failure
./build-qt5/run.sh
# Сразу шестая авторская карта:
./build-qt5/run.sh --level 6
# Только галерея:
./build-qt5/run.sh --tile-gallery
```

`env.sh` задаёт пути к локальным инструментам, библиотекам, плагинам и QML-модулям; `run.sh` подхватывает это окружение при запуске. Оба файла созданы только на H2 в игнорируемом каталоге `build-qt5`, не входят в репозиторий и не восстанавливаются автоматически после его удаления. Для чистой установки используйте системные зависимости и стандартные команды выше. `build_and_run.sh` рассчитан на доступные в окружении инструменты и Qt; на H2 перед ним нужно выполнить `source build-qt5/env.sh`.

### Из VS Code

1. `Ctrl+Shift+P` → **CMake: Configure** (выбрать kit с Qt).
2. `Ctrl+Shift+P` → **CMake: Build** (или `F7`).
3. `F5` — запуск с отладчиком.

На H2 VS Code должен наследовать окружение `build-qt5/env.sh` либо иметь соответствующие переменные в настройках запуска и задач.

### Если CMake не находит Qt

Укажите путь явно:

```bash
cmake -S . -B build-qt5 -G Ninja -DCMAKE_PREFIX_PATH=/opt/Qt/5.15.2/gcc_64
```

---

## Структура проекта

Учебные материалы находятся в `docs/`. [LEARNING_07.md](docs/LEARNING_07.md) сохраняет исторический разбор ошибок подключения Level6. Эти ошибки исправлены: шестая авторская карта подключена, её формат и выходы проверены. [LEARNING_08.md](docs/LEARNING_08.md) описывает результат, запуск с уровня 1–50, исправление генератора и выполненные проверки.

[LEARNING_9.md](docs/LEARNING_9.md) — сводный пополняемый материал о генерации уровней: seed и воспроизводимость, алгоритм построения карт, хранение в памяти и состояние прохождения, тесты и ограничения. Последующие пояснения и результаты работ по генерации добавляются в этот документ с сохранением истории.

```
.
├── CMakeLists.txt              # Сборка под Qt 5.15 и список исходников
├── main.cpp                    # Создание приложения и регистрация GameEngine
├── qml.qrc                     # QML, шрифт и выбранные PNG в ресурсах приложения
├── build_and_run.sh            # Настройка, сборка и запуск
├── docs/                      # Учебные Markdown-материалы проекта
│   ├── LEARNING_01.md          # Основы C++/Qt/QML
│   ├── LEARNING_02.md          # Архитектура и игровые правила
│   ├── LEARNING_03.md          # Генератор пейзажей
│   ├── LEARNING_04.md          # Qt Test и CTest
│   ├── LEARNING_05.md          # Библиотека тайлов и варианты Персонажа
│   ├── LEARNING_06.md          # Настройки, меню и пауза
│   ├── LEARNING_07.md          # Исторический разбор ошибок подключения Level6
│   ├── LEARNING_08.md          # Исправления Level6 и запуск --level 1–50
│   └── LEARNING_9.md           # Пополняемый разбор генерации, seed, хранения и проверок
├── src/
│   ├── gamesettings.h/.cpp     # Проверка, сохранение и уведомления настроек
│   ├── gameengine.h/.cpp       # Игровое состояние, модели и игровые правила
│   └── landscapegenerator.h/.cpp # Создание и валидация новых пейзажей
├── tests/
│   ├── test_gamesettings.cpp   # Сохранение, значения и сигналы настроек
│   ├── test_gameengine.cpp     # Автоматические проверки правил движка
│   ├── test_tilelibrary.cpp    # Тайлы, Персонажи, галерея и переключатели
│   └── test_startup.cpp        # Запуск с выбранного уровня и ошибки аргументов
├── assets/
│   └── tilesets/                # Лицензированные наборы PNG для QML-тайлов
└── qml/
    ├── main.qml                # Окно, ввод, HUD, игровое поле и таймер физики
    ├── levels/                 # Ручные уровни в отдельных QML-файлах
    │   ├── Level1.qml
    │   ├── Level2.qml
    │   ├── Level3.qml
    │   ├── Level4.qml
    │   ├── Level5.qml
    │   └── Level6.qml
    ├── components/
    │   ├── Tile.qml            # Выбор представления клетки через Loader
    │   ├── Player.qml          # Отрисовка героя по состоянию и варианту
    │   ├── GameMenu.qml        # Начальное меню и пауза
    │   ├── SettingsPanel.qml   # Настройки и предпросмотр
    │   ├── MenuButton.qml      # Общая кнопка меню
    │   ├── PlayerStyles.qml    # Каталог вариантов и кадров Персонажа
    │   ├── Enemy.qml           # Задел для врагов
    │   └── tiles/              # GameTile, BrickTile, CrystalTile, PrizeTile и темы
    ├── examples/
    │   ├── TileGallery.qml     # Окно сравнения графики
    │   └── PlayerGallery.qml   # Предпросмотр вариантов Персонажа
    └── fonts/                  # Пиксельный шрифт и лицензия
```

### Принцип разделения

- **C++** — игровое состояние, правила движения и сбора, физика и генерация уровней.
- **QML** — визуализация, клавиатурный ввод, подсветка выхода и данные ручных экранов в `qml/levels/`.
- **CMake** — собирает C++ и QML-ресурсы; проект включает QML через `qml.qrc`.

---

## Управление в игре

| Клавиша | Действие |
|---------|----------|
| Стрелки или `W` `A` `S` `D` | Перемещение по клеткам |
| `Space` | Копать кирпич по направлению взгляда |
| `Esc` или `P` | Пауза / продолжение; из настроек — назад |

| Кнопка в окне | Действие |
|---|---|
| «Пауза» | Меню продолжения, новой игры и выхода |
| «Настройки» | Оформление, анимация и скорость предпросмотра |
| «Тема» | Classic → Kenney → Industrial |
| «Герой» | Классический → Космонавт → Розовый космонавт → Робот |
| «Галерея тайлов» | Сравнение тайлов и Персонажей в отдельном окне |

Смена оформления применяется сразу и сохраняет текущую игровую позицию и счёт. Тема, герой, разрешение и интервал анимации сохраняются между запусками. При запуске появляется меню; выберите «Начать игру». Выход доступен в меню паузы.

Соберите все обязательные Призы, затем самостоятельно дойдите до выхода в правом нижнем углу. На уровнях после первого нижний левый портал возвращает на предыдущий экран, если это разрешено нижней границей сессии. При `--level N` уход ниже N запрещён; подсказка сообщает об ограничении, а левый выход стартовой карты не подсвечивается как доступный.

---

## Библиотека визуальных тайлов

В `qml/components/tiles/` находятся `GameTile`, `BrickTile`, `CrystalTile`, `PrizeTile`, базовый `TileTheme` и темы `ClassicTheme`, `KenneyTheme`, `IndustrialTheme`. Это файловая библиотека QML; установка отдельного QML-модуля не требуется.

| Свойство тайла | Значение по умолчанию | Назначение |
|---|---|---|
| `cellSize` | `32` | Ширина и высота клетки |
| `theme` | `"classic"` | `"classic"`, `"kenney"` или `"industrial"`; неизвестная тема использует Classic |
| `highlighted` | `false` | Зелёная рамка для просмотра и отладки |
| `animated` | `true` | Пульсация спрайта Приза; `false` фиксирует изображение |
| `animationInterval` | `140` | Масштаб длительности визуальных эффектов относительно 140 мс |
| `crystalColor` | `"#35d6c4"` | Только CrystalTile: цвет геометрического кристалла Classic |

Пример файлового подключения из `qml/main.qml`:

```qml
import "components/tiles" as Tiles

Tiles.BrickTile {
    cellSize: 32
    theme: "kenney"
    animated: false
}
```

`TileTheme` задаёт `name`, `brickSource`, `crystalSource` и `prizeSource`. `GameTile` выбирает тему и общий способ отображения; специализированные компоненты сохраняют геометрическое оформление Classic. При переносе библиотеки скопируйте каталог компонентов, выбранные PNG и лицензии, а также соответствующие записи `qml.qrc`: темы используют пути `qrc:/assets/tilesets/kenney/...`.

В игре кнопка «Тема» циклически меняет оформление, кнопка «Галерея тайлов» открывает отдельное окно. `Tile.qml` передаёт размер, тему и визуальное состояние через `Loader`. Значение `root.tileTheme` — единственный переключатель оформления игрового поля. Алиас `root.tileTheme` связан с `GameSettings.tileTheme` и сохраняется через C++. Сейф, Ключ, выход, яма и специальные Призы сохраняют прежнюю отрисовку. Карта, типы клеток и физика остаются в C++.

Галерея `qml/examples/TileGallery.qml` не создаёт GameEngine. Она показывает три темы рядом, дополнительный тайл Blocks, размеры 18/32/48/64, подсветку и отключаемую анимацию. На узком окне карточки переносятся, содержимое прокручивается. Запуск после стандартной сборки:

```bash
./build-qt5/boulderdash --tile-gallery
```

На H2 с локально распакованными зависимостями используйте `./build-qt5/run.sh --tile-gallery`. Для пересборки и тестирования в этом окружении сначала выполните `source build-qt5/env.sh`.

Каталог выбранных файлов и лицензий: [assets/tilesets/kenney/README.md](assets/tilesets/kenney/README.md). Снимок галереи на H2 сохранён в `/home/majorov/docs/QML-tile-gallery.png`.

## Варианты Персонажа

Кнопка «Герой» переключает четыре оформления независимо от темы тайлов: Классический → Космонавт → Розовый космонавт → Робот. Вариант применяется сразу, не меняя координаты, направление, счёт и правила движения; выбор сохраняется между запусками через `GameSettings`.

`qml/components/PlayerStyles.qml` содержит ключи `classic`, `astronaut`, `pink`, `robot`, названия и номера кадров. `Player.qml` принимает `variant`, `size`, `state`, `facingRight`, `animated`, `animationInterval`. Состояние и направление по-прежнему приходят из `GameEngine`; единственный выбор оформления в игровом окне — `root.playerVariant`. Неизвестный ключ использует классический вид.

Варианты Kenney используют прозрачные PNG размером 24×24 из `pixel-platformer/Tiles/Characters/`, масштабируются с сохранением пропорций и `smooth: false`, поворачиваются зеркально при взгляде влево. Для `run`, `climb`, `dig` перебираются имеющиеся кадры с настраиваемым интервалом (по умолчанию 140 мс); копание и падение дополнительно обозначаются наклоном. Это общие кадры набора, а не отдельные авторские последовательности для каждого действия. При `animated: false` отображается первый кадр; наклон по состоянию сохраняется.

Внизу галереи есть сравнение Персонажей. Общие кнопки размера и анимации действуют также на них; переключатели `idle/run/climb/dig/fall` и направления меняют только предпросмотр.

Подробный разбор контрактов, кадров и добавления нового героя находится в [`LEARNING_05.md`](docs/LEARNING_05.md).

### Настройки и пауза

`GameSettings` хранит четыре свойства с `READ/WRITE/NOTIFY`: `tileTheme`, `playerVariant`, `animationsEnabled`, `animationInterval`. Сеттеры проверяют значения, сохраняют изменения и уведомляют QML. UI предлагает интервал кадров 80/140/220 мс; C++ допускает 60–500 мс. Скорость симуляции не меняется.

В стандартном Linux-окружении настройки находятся в `~/.config/Majorov/BoulderDash.conf`, группа `appearance`; при заданном `XDG_CONFIG_HOME` путь меняется. «Сбросить оформление» возвращает Classic, классического героя, включённую анимацию и 140 мс, сохраняя рекорд.

Меню и настройки включают `GameEngine.paused`. Движок блокирует движения, копание и шаги физики. Таймеры восстановления Кирпичей останавливаются с сохранением `remainingTime()` и продолжаются с тем же остатком; после новой игры старые таймеры удаляются. QML-таймер физики после продолжения начинает новый период 250 мс. Предпросмотр в настройках продолжает работать, пока поле на паузе.

После настроек из меню игра остаётся на паузе; после настроек из игрового поля — продолжается. Новая игра сбрасывает прохождение, сохраняя оформление. Для завершения приложения откройте меню и выберите «Выход».

Учебный этап: [LEARNING_06.md](docs/LEARNING_06.md). Учебные Markdown-файлы хранятся в `docs/`. Снимок настроек сохранён локально на H2 в `/home/majorov/docs/QML-settings.png` и не входит в репозиторий.

## Проверки

| Группа CTest | Что проверяет |
|---|---|
| `gamesettings` | Сохранение и повторное чтение настроек, значения по умолчанию, сигналы, отказ от неверных значений и сброс без удаления рекорда |
| `gameengine` | Игровые правила и пауза; запуск авторских/генерируемых уровней, нижняя граница тестового старта, переход вперёд и возврат до неё, сброс сессии, неверные номера и воспроизводимость/опоры предметов для seed 1–45 |
| `startup` | Запуск отдельного процесса на уровнях 1/6/7/50, отказ от неверных, пропущенных и повторных аргументов и конфликта с галереей |
| `tilelibrary` | Три типа тайлов в трёх темах и размерах 18/32/64, загрузку изображений и смену темы |
| `tilelibrary` | Четыре варианта Персонажа, пять состояний, оба направления, размеры 32/64 и возврат к Classic при неизвестном варианте |
| `tilelibrary` | Перебор кадров робота и отключение анимации, загрузку галереи и изменение размеров окна |
| `tilelibrary` | Начальное меню, настройки, возврат, сохранение, Esc/P и продолжение; переключение темы и реальные нажатия кнопки «Герой» без изменения позиции, счёта и числа Призов; привязки действия и направления |

```bash
ctest --test-dir build-qt5 --output-on-failure
```

Тесты QML запускаются без видимого окна: CTest задаёт `QT_QPA_PLATFORM=offscreen` и `QT_QUICK_BACKEND=software`. Они отслеживают предупреждения `QQmlEngine::warnings`. QML-тесты используют временный каталог настроек, не изменяя пользовательский файл. На H2 сначала выполните `source build-qt5/env.sh`.

Необязательно: сохранить снимок галереи тайлов и Персонажей на H2:

```bash
QML_GALLERY_SCREENSHOT=/home/majorov/docs/QML-tile-gallery.png \
    ctest --test-dir build-qt5 -R tilelibrary --output-on-failure
```

Для первоначальной версии этапа [LEARNING_08.md](docs/LEARNING_08.md) сборка и все четыре группы тестов прошли. После уточнения нижней границы тестового старта сборка и повторный прогон также успешны: **4/4 группы CTest**, 6,77 с. Проверены запрет ухода ниже N, переход вперёд после Призов, возврат до N, сброс ограничения и доступность выходов в QML. Галерея проверялась по снимку на предыдущем этапе. Полное ручное прохождение после изменения графики остаётся отдельной проверкой: переключите тему и героя, проверьте движение, копание и восстановление Кирпича, падение Кристалла, сбор Приза, Ключ, Сейф и переходы между уровнями. Проверьте маленькое окно и консоль на ошибки QML.

---

## Как это работает: C++ ↔ QML

Это ключевой момент, который нужно понять в первую очередь.

### 1. Регистрация C++ типа в QML

В `src/gameengine.h`:

```cpp
class GameEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int playerX READ playerX NOTIFY playerPositionChanged)
    // ...
};
```

В текущем проекте используется Qt 5 и ручная регистрация типа в `main.cpp` через `qmlRegisterType<GameEngine>("BoulderDash", 1, 0, "GameEngine")`. Макрос `QML_ELEMENT` в заголовке сам по себе не выполняет регистрацию в этой конфигурации.

### 2. Использование C++ типа в QML

В `main.qml`:

```qml
import BoulderDash 1.0   // URI, переданный в qmlRegisterType в main.cpp

GameEngine {
    id: gameEngine
}

Text {
    text: "Player: " + gameEngine.playerX + ", " + gameEngine.playerY
}
```

`import BoulderDash 1.0` соответствует URI, версии и имени типа из `qmlRegisterType` в `main.cpp`.

### 3. Свойства, сигналы, слоты

Три механизма связи C++ и QML:

| Механизм | Направление | Пример |
|----------|-------------|--------|
| `Q_PROPERTY` | C++ → QML (чтение) | `gameEngine.playerX` |
| `NOTIFY` сигнал | C++ уведомляет QML об изменении | `emit playerPositionChanged()` |
| `Q_INVOKABLE` | QML → C++ (вызов метода) | `gameEngine.movePlayer(-1, 0)` |

Когда C++ эмитит сигнал из `NOTIFY`, QML автоматически перерисовывает всё, что привязано к этому свойству. Это основа реактивного UI.

### 4. Модели данных

Для списков (тайлы, враги) используется `QAbstractListModel`. В QML он подключается через `Repeater`:

```qml
Repeater {
    model: gameEngine.tilesModel
    delegate: Tile {
        x: model.col * tileSize
        y: model.row * tileSize
        type: model.type
    }
}
```

`model.col`, `model.row`, `model.type` — это роли, объявленные в `roleNames()` в C++.

---

## Шпаргалка по QML

### Базовые элементы

```qml
import QtQuick

Item {              // пустой контейнер, невидимый
    id: root
    width: 100; height: 100

    Rectangle {     // прямоугольник
        anchors.fill: parent
        color: "red"
        radius: 8
        border.color: "black"
        border.width: 1
    }

    Text {
        anchors.centerIn: parent
        text: "Hello"
        color: "white"
        font.pixelSize: 20
    }

    Image {
        source: "qrc:/assets/player.png"
        width: 32; height: 32
    }
}
```

### Anchors (привязки)

```qml
Rectangle {
    anchors.centerIn: parent
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.margins: 8
    width: 100; height: 100
}
```

Заменители `fill: parent` = `width + height + x + y` по родителю.

### Свойства

```qml
Item {
    property int score: 0
    property string name: "player"
    property bool alive: true
    property var data: [1, 2, 3]

    // вычисляемые свойства
    readonly property int halfWidth: width / 2
}
```

### Обработка сигналов

```qml
MouseArea {
    anchors.fill: parent
    onClicked: console.log("clicked!")
    onPressed: (mouse) => console.log(mouse.x, mouse.y)
}

Timer {
    interval: 16
    running: true
    repeat: true
    onTriggered: console.log("tick")
}
```

### Обработка клавиатуры

```qml
Item {
    focus: true   // ← без этого не работает
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Left) { /* ... */ event.accepted = true }
    }
}
```

### Repeater и модели

```qml
Repeater {
    model: 5
    delegate: Rectangle { width: 20; height: 20; color: "blue" }
}

Repeater {
    model: ListModel {
        ListElement { name: "A"; value: 1 }
        ListElement { name: "B"; value: 2 }
    }
    delegate: Text { text: name + ": " + value }
}
```

### Loader — динамическая загрузка компонентов

```qml
Loader {
    sourceComponent: condition ? componentA : componentB
}

Component {
    id: componentA
    Rectangle { color: "red"; width: 50; height: 50 }
}
```

### Анимации

```qml
Rectangle {
    id: box
    width: 50; height: 50; color: "green"
    x: 0

    NumberAnimation on x {
        to: 200
        duration: 1000
        loops: Animation.Infinite
        running: true
    }

    Behavior on x { NumberAnimation { duration: 200 } }
}
```

### Состояния и переходы

```qml
Item {
    id: player
    state: "idle"

    states: [
        State { name: "idle"; PropertyChanges { target: sprite; source: "idle.png" } },
        State { name: "run";  PropertyChanges { target: sprite; source: "run.png"  } }
    ]

    transitions: Transition {
        NumberAnimation { properties: "x,y"; duration: 100 }
    }
}
```

---

## Шпаргалка по C++/Qt

### Q_OBJECT и Q_PROPERTY

```cpp
class MyClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    int value() const { return m_value; }
    void setValue(int v) {
        if (m_value == v) return;
        m_value = v;
        emit valueChanged();
    }

signals:
    void valueChanged();

private:
    int m_value = 0;
};
```

### Q_INVOKABLE — метод, вызываемый из QML

```cpp
Q_INVOKABLE void doSomething(int arg);
```

Из QML: `myObject.doSomething(42)`.

### Сигналы и слоты

```cpp
// старый синтаксис
connect(sender, SIGNAL(clicked()), receiver, SLOT(handle()));

// новый синтаксис (рекомендуется)
connect(sender, &QPushButton::clicked, receiver, &MyClass::handle);

// лямбда
connect(timer, &QTimer::timeout, this, [this]() { /* ... */ });
```

### QAbstractListModel — минимальный шаблон

```cpp
class MyModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { NameRole = Qt::UserRole + 1 };

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_items.size();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid()) return {};
        const auto &item = m_items[index.row()];
        switch (role) {
            case NameRole: return item.name;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {NameRole, "name"} };
    }

private:
    struct Item { QString name; };
    QVector<Item> m_items;
};
```

### Умные указатели

```cpp
#include <memory>

auto ptr = std::make_unique<MyClass>();   // единственный владелец
auto shared = std::make_shared<MyClass>(); // разделяемый
QScopedPointer<MyClass> qptr(new MyClass); // Qt-аналог unique_ptr
```

### Строки, списки, словари

```cpp
QString s = "hello";
QStringList list = { "a", "b", "c" };
QVector<int> vec;
QHash<QString, int> map;
```

---

## Шпаргалка по CMake

### Фрагмент сборки текущего проекта на Qt 5

```cmake
cmake_minimum_required(VERSION 3.16)
project(boulderdash VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.15 COMPONENTS Core Quick Gui Qml REQUIRED)

qt5_add_resources(QML_RESOURCES qml.qrc)

add_executable(boulderdash
    main.cpp
    src/gameengine.cpp
    src/gameengine.h
    src/landscapegenerator.cpp
    src/landscapegenerator.h
    ${QML_RESOURCES}
)

target_link_libraries(boulderdash PRIVATE
    Qt5::Core Qt5::Quick Qt5::Gui Qt5::Qml
)
```

### Ключевые команды

- `find_package(Qt5 5.15 ... REQUIRED)` — находит требуемые модули Qt 5.
- `qt5_add_resources(...)` — включает файлы из `qml.qrc` в ресурсы исполняемого файла.
- `add_executable(...)` — создаёт цель приложения и перечисляет C++ исходники вместе с ресурсами.
- `target_link_libraries(...)` — линкует с Qt-модулями.
- `qmlRegisterType` в `main.cpp` регистрирует `GameEngine`; это выполняется кодом приложения, а не CMake.

### Отладочная печать переменных

```cmake
message(STATUS "Qt found at: ${Qt5_DIR}")
message(FATAL_ERROR "Something is wrong")
```

### Типы сборки

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release
```

---

## Отладка в VS Code

### `.vscode/launch.json`

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Boulder Dash",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build-qt5/boulderdash",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "setupCommands": [
                { "text": "-enable-pretty-printing", "ignoreFailures": true }
            ],
            "preLaunchTask": "CMake: Build"
        }
    ]
}
```

### Точки останова (breakpoints)

- Кликните слева от номера строки в C++ файле.
- `F5` — запуск под отладчиком.
- `F10` — шаг с обходом, `F11` — шаг с заходом, `Shift+F5` — стоп.

### Как отлаживать QML

Добавьте в `.vscode/launch.json` вторую конфигурацию:

```json
{
    "name": "QML Debug",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/build-qt5/boulderdash",
    "args": ["-qmljsdebugger=port:10000,block"],
    "MIMode": "gdb"
}
```

И установите расширение **Qt QML Debug Adapter**.

### Отладочная печать

В C++: `qDebug() << "playerX:" << m_playerX;`
В QML: `console.log("state:", state)`

---

## Работа с Git

### Первый push в новый репозиторий

```bash
git init
git branch -M main
git add .
git commit -m "Initial commit"
git remote add origin git@github.com:USER/REPO.git
git push -u origin main
```

### Ежедневная работа

```bash
git status                # что изменилось
git diff                  # что именно изменилось в строках
git add .                 # подготовить всё к коммиту
git add file.cpp          # или конкретный файл
git commit -m "Fix: ..."  # коммит
git push                  # отправить в origin
git pull                  # забрать изменения
```

### Полезные настройки

```bash
git config --global init.defaultBranch main
git config --global push.autoSetupRemote true
git config --global user.name "Ваше Имя"
git config --global user.email "you@example.com"
```

### SSH-ключ для GitHub

```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_github -C "github-USERNAME"
cat ~/.ssh/id_ed25519_github.pub
# добавить содержимое на https://github.com/settings/keys
```

В `~/.ssh/config`:

```
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_github
    IdentitiesOnly yes
```

Проверка: `ssh -T git@github.com` → `Hi USERNAME!`.

---

## Учебный план: что учить и в каком порядке

Разбейте освоение на 6 этапов. Каждый этап — 1–2 недели в комфортном темпе.

### Этап 1. Основы C++ (2 недели)

Что учить:

- Типы: `int`, `double`, `bool`, `QString`, `QVector`, `QHash`.
- Классы, методы, конструкторы, деструкторы.
- Наследование и виртуальные функции (`virtual`, `override`).
- Указатели, ссылки, `nullptr`, `const`.
- Умные указатели: `std::unique_ptr`, `std::shared_ptr`.
- Лямбды: `[this]() { ... }`.
- Пространства имён (`namespace`, `using`).

Практика: напишите класс `Player` с полями `x, y, health` и методом `move(dx, dy)`, без Qt.

### Этап 2. Основы Qt-объектов (1 неделя)

Что учить:

- `QObject`, родитель-потомок, автоматическое удаление.
- Сигналы и слоты, `connect`.
- `Q_PROPERTY`, `Q_INVOKABLE`, ручная регистрация C++ типов в QML.
- `QTimer` — основа игрового цикла.

Практика: сделайте счётчик на C++, который эмитит сигнал раз в секунду и обновляет `QLabel`.

### Этап 3. Основы QML (2 недели)

Что учить:

- `Item`, `Rectangle`, `Text`, `Image`, `MouseArea`.
- Anchors, `width`, `height`, `x`, `y`.
- `property`, сигналы, `onClicked`, `onPressed`.
- `Repeater`, `ListView`, `ListModel`.
- `Timer`, `Animation`, `Behavior`, `states`, `transitions`.
- `Loader` и `Component`.

Практика: сделайте экран с кнопкой, счётчиком и анимацией.

### Этап 4. Связка C++ и QML (1 неделя)

Что учить:

- `QQmlApplicationEngine`, `load` и загрузка компонента из `qrc:/`.
- `import <ModuleURI> 1.0`.
- `Q_PROPERTY` + `NOTIFY` → реактивное обновление QML.
- `Q_INVOKABLE` — вызов методов из QML.
- `QAbstractListModel` — передача списков.

Практика: замените счётчик из этапа 2 на `Q_PROPERTY` и покажите его в QML.

### Этап 5. Игровая логика Boulder Dash (2–3 недели)

Что делать:

- Сетка уровня: `QVector<QString>` как карта.
- Коллизии: «можно ли игроку войти в клетку».
- Гравитация: если под игроком пусто — падать.
- Копание: изменить тайл, обновить модель.
- Сбор золота: удалить тайл, увеличить счёт.
- Генератор: seed, каркас карты, размещение объектов и проверка достижимости.
- Переходы: сбор Призов, доступность выхода, сохранение состояния экранов.

### Этап 6. Улучшения (по вкусу)

- Графика: тайлы и варианты Персонажа уже используют PNG; следующий шаг — отдельные последовательности кадров для действий или PNG-атласы.
- Звук: `QSoundEffect` в QML.
- Доработка уровней: генератор пейзажей уже реализован, его правила можно развивать.
- Враги: `EnemyModel` и визуальный компонент есть, активного поведения в текущих уровнях нет.
- Меню: отдельный QML-экран с кнопками.
- Сохранение: `QSettings` или JSON.

### Правила освоения

1. **Не читайте всё подряд.** Читайте ровно то, что нужно для следующего шага.
2. **Пишите код сами.** Копипаста не работает.
3. **Ломайте и починяйте.** Ошибки — лучшие учителя.
4. **Один коммит — одно изменение.** История Git должна читаться как книга.
5. **Ведите заметки.** Например, прямо в этом README.

---

## Полезные ресурсы

### Официальная документация Qt

- [Qt 5.15 Documentation](https://doc.qt.io/archives/qt-5.15/) — версия, используемая проектом.
- [Qt 5 QML Reference](https://doc.qt.io/archives/qt-5.15/qtqml-index.html) — документация QML для Qt 5.
- [Qt Quick Examples](https://doc.qt.io/archives/qt-5.15/qtquick-examples.html) — примеры для Qt 5.15.
- [Qt 6 Documentation](https://doc.qt.io/qt-6/) — материалы для отдельного изучения Qt 6; команды сборки проекта к Qt 6 не относятся.

### Учебники

- **Qt Academy** — [academy.qt.io](https://academy.qt.io/) — бесплатные курсы от Qt.
- **Same Game Tutorial** — встроенный пример Qt, отличная отправная точка для QML-игр.
- **Qt for Beginners** — [wiki.qt.io/Qt_for_Beginners](https://wiki.qt.io/Qt_for_Beginners).

### Книги

- *Game Programming using Qt 5/6 Beginner's Guide* — пошагово строит игру.
- *C++ Primer* (Lippman) — фундамент C++.
- *Effective Modern C++* (Meyers) — современные практики.

### Видео

- **KDAB TV** — короткие ролики по Qt.
- **VoidRealms** — большой C++/Qt канал.
- **Qt Company** — официальный канал с записями конференций.

### Открытый код для изучения

- [KGoldrunner](https://invent.kde.org/games/kgoldrunner) — классический платформер на Qt Widgets.
- [Same Game](https://code.qt.io/cgit/qt/qtquickcontrols2.git/) — простой пример на QML.

### Инструменты

- **Qt Creator** — рекомендую поставить рядом с VS Code. Он умеет всё из коробки: сборка, отладка QML, дизайнер.
- **qmllint** — линтер QML.
- **qmlformat** — форматирование QML.
- **clang-format** — форматирование C++.

---

## Что делать, если что-то не работает

| Симптом | Вероятная причина | Решение |
|---------|-------------------|---------|
| `Cannot assign to non-existent property` | C++ тип или компонент не зарегистрирован/не загружен | Проверить `qmlRegisterType`, имя типа и компонент |
| `module "BoulderDash" is not installed` | Не совпадает URI или версия QML-модуля | Сверить `qmlRegisterType` в `main.cpp` и `import` в QML |
| `undefined reference to vtable` | Забыт `Q_OBJECT` или moc не прогнан | Запустить CMake заново, проверить CMakeLists |
| QML не обновляется при изменении | Не испущен `NOTIFY` сигнал или модель не отправила уведомление | Проверить сигнал свойства, `dataChanged()` и reset модели |
| Ошибка про `QApplication` | Подключён неподходящий класс приложения для Qt Quick | В этом проекте используется `QGuiApplication`; проверить заголовок и CMake-модули |
| На H2 не найдены Qt-библиотеки, платформенный плагин или QML-модуль | Не подхвачены локальные пути | Запустить `./build-qt5/run.sh` или выполнить `source build-qt5/env.sh` |
| PNG не загружается | URL не совпадает с ресурсом | Проверить путь и alias в `qml.qrc`, затем пересобрать приложение |
| Программа падает при старте | Скорее всего, null pointer или отсутствующий QML | Запустить под отладчиком, посмотреть стек |
| `Permission denied (publickey)` при git push | SSH-ключ не добавлен на GitHub | См. раздел «Работа с Git» |

Всегда смотрите **вывод консоли** — Qt много и полезно ругается. Ошибки QML почти всегда указывают точный файл и строку.

---

## Лицензия

Учебный проект. Используйте и меняйте свободно.

Графика Kenney распространяется по CC0; источники и файлы лицензий перечислены в [каталоге графики](assets/tilesets/kenney/README.md). У шрифта Unifont отдельная лицензия: [UNIFONT-LICENSE.txt](qml/fonts/UNIFONT-LICENSE.txt).

---

*Последнее обновление: см. историю git.*
