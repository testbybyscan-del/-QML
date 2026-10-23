# Boulder Dash на Qt 5.15 + QML

Учебный проект: игра теперь называется **Boulder Dash**. Проект написан на C++ и QML; цель — освоить связку **C++ (логика) + QML (визуализация) + CMake (сборка) + VS Code (среда)**.

Исполняемый файл называется `boulderdash`, а C++-тип `GameEngine` регистрируется в QML под URI `BoulderDash`.

> Этот README содержит обзор проекта и справочные заметки. Пошаговые материалы для самостоятельной разработки собраны отдельно: [`LEARNING_01.md`](LEARNING_01.md) → [`LEARNING_02.md`](LEARNING_02.md) → [`LEARNING_03.md`](LEARNING_03.md) → [`LEARNING_04.md`](LEARNING_04.md).

---

## 📑 Содержание

1. [Что это за проект](#что-это-за-проект)
2. [Требования и установка](#требования-и-установка)
3. [Сборка и запуск](#сборка-и-запуск)
4. [Структура проекта](#структура-проекта)
5. [Управление в игре](#управление-в-игре)
6. [Как это работает: C++ ↔ QML](#как-это-работает-c--qml)
7. [Шпаргалка по QML](#шпаргалка-по-qml)
8. [Шпаргалка по C++/Qt](#шпаргалка-по-cqt)
9. [Шпаргалка по CMake](#шпаргалка-по-cmake)
10. [Отладка в VS Code](#отладка-в-vs-code)
11. [Работа с Git](#работа-с-git)
12. [Учебный план: что учить и в каком порядке](#учебный-план-что-учить-и-в-каком-порядке)
13. [Полезные ресурсы](#полезные-ресурсы)

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

В проекте пока нет активных врагов в отображаемых уровнях: `EnemyModel` и QML-компонент существуют как задел. Также пока нет звука, графических спрайтов и сохранения состояния уровней на диск; между запусками сохраняется только рекорд.

---

## Требования и установка

### Что нужно установить

| Компонент | Версия | Зачем |
|-----------|--------|-------|
| **Qt** | 5.15 или новее | Модули Core, Quick, Gui и Qml (`Qt5::...`) |
| **CMake** | 3.16+ | Система сборки |
| **Компилятор** | C++17 | Сборка проекта |
| **Ninja** | любая | Быстрый генератор сборки |
| **Git** | любая | Версионирование |
| **VS Code** | последняя | Редактор |

### Установка Qt на Linux (Ubuntu/Debian)

Самый надёжный способ — **Qt Online Installer** с [qt.io/download](https://www.qt.io/download-qt-installer). Выберите:

- Qt 5.15
- Компоненты: **Desktop gcc 64-bit**, Qt Quick/QML и средства разработки

Альтернатива — через `aqtinstall`:

```bash
pip install aqtinstall
aqt install-qt linux desktop 5.15.2 gcc_64
```

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
cmake --build build-qt5

# Запуск
./build-qt5/boulderdash
```

Или выполните `./build_and_run.sh`. Подробности установки зависимостей находятся в [`README_RUN.md`](README_RUN.md).

Тесты игровой логики собираются вместе с проектом и запускаются через CTest:

```bash
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
```

Пошаговое руководство по тестам находится в [`LEARNING_04.md`](LEARNING_04.md).

### Из VS Code

1. `Ctrl+Shift+P` → **CMake: Configure** (выбрать kit с Qt).
2. `Ctrl+Shift+P` → **CMake: Build** (или `F7`).
3. `F5` — запуск с отладчиком.

### Если CMake не находит Qt

Укажите путь явно:

```bash
cmake -S . -B build-qt5 -G Ninja -DCMAKE_PREFIX_PATH=/opt/Qt/5.15.2/gcc_64
```

---

## Структура проекта

```
.
├── CMakeLists.txt              # Сборка под Qt 5.15 и список исходников
├── main.cpp                    # Создание приложения и регистрация GameEngine
├── qml.qrc                     # Включение QML-файлов в ресурсы приложения
├── build_and_run.sh            # Настройка, сборка и запуск
├── LEARNING_01.md              # Учебная часть 1
├── LEARNING_02.md              # Учебная часть 2
├── LEARNING_03.md              # Руководство по генератору
├── LEARNING_04.md              # Qt Test и CTest для логики игры
├── src/
│   ├── gameengine.h/.cpp       # Игровое состояние, модели и игровые правила
│   └── landscapegenerator.h/.cpp # Создание и валидация новых пейзажей
├── tests/
│   └── test_gameengine.cpp     # Автоматические проверки правил движка
└── qml/
    ├── main.qml                # Окно, ввод, HUD, игровое поле и таймер физики
    ├── levels/                 # Ручные уровни в отдельных QML-файлах
    │   ├── Level1.qml
    │   ├── Level2.qml
    │   ├── Level3.qml
    │   ├── Level4.qml
    │   └── Level5.qml
    ├── components/             # Tile.qml, Player.qml, Enemy.qml
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
| `Esc` | Выход |

Соберите все обязательные Призы, затем самостоятельно дойдите до выхода в правом нижнем углу. На уровнях после первого нижний левый портал возвращает на предыдущий экран.

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

- Спрайты: `SpriteSequence`, PNG-атласы.
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
| Программа падает при старте | Скорее всего, null pointer или отсутствующий QML | Запустить под отладчиком, посмотреть стек |
| `Permission denied (publickey)` при git push | SSH-ключ не добавлен на GitHub | См. раздел «Работа с Git» |

Всегда смотрите **вывод консоли** — Qt много и полезно ругается. Ошибки QML почти всегда указывают точный файл и строку.

---

## Лицензия

Учебный проект. Используйте и меняйте свободно.

---

*Последнее обновление: см. историю git.*
