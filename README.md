# LodeRunner на Qt 6 + QML

Учебный проект: реализация классической игры Lode Runner (в духе KGoldrunner) на C++ и QML. Цель проекта — не столько сама игра, сколько освоение связки **C++ (логика) + QML (визуализация) + CMake (сборка) + VS Code (среда)**.

> Этот README одновременно является шпаргалкой и учебным планом. Читайте сверху вниз, возвращайтесь к разделам по мере необходимости.

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

Lode Runner — платформенная игра 1983 года. Игрок бегает по кирпичной сетке, собирает золото, роет ямы в кирпичах, чтобы ловить врагов, и должен собрать всё золото, чтобы пройти уровень.

Проект реализует:

- **Сетку уровня** 20×12 тайлов (кирпич, лестница, верёвка, золото, пустота).
- **Игрока** с гравитацией, движением и копанием.
- **Врагов** с простейшим ИИ (движение, разворот у стены, погоня).
- **Игровой цикл** ~60 FPS через `QTimer`.
- **HUD**: счёт, жизни, сообщения о победе/проигрыше.

Что **не реализовано** (можно добавить самостоятельно как упражнение):

- Анимация спрайтов через `SpriteSequence`.
- Звук.
- Несколько уровней с переходом между ними.
- Полноценное поведение врагов (лазание по лестницам, падение в вырытые ямы).
- Сохранение прогресса.

---

## Требования и установка

### Что нужно установить

| Компонент | Версия | Зачем |
|-----------|--------|-------|
| **Qt 6.5+** | 6.5 или новее | Основной фреймворк (`Qt6::Quick`, `Qt6::Qml`, `Qt6::Gui`) |
| **CMake** | 3.21+ | Система сборки |
| **Компилятор** | GCC 11+, Clang 14+ | Сборка C++17 |
| **Ninja** | любая | Быстрый генератор сборки |
| **Git** | любая | Версионирование |
| **VS Code** | последняя | Редактор |

### Установка Qt на Linux (Ubuntu/Debian)

Самый надёжный способ — **Qt Online Installer** с [qt.io/download](https://www.qt.io/download-qt-installer). Выберите:

- Qt 6.5 или 6.6
- Компоненты: **Desktop gcc 64-bit**, **Qt Quick**, **Qt Debug Information Files** (опционально)

Альтернатива — через `aqtinstall`:

```bash
pip install aqtinstall
aqt install-qt linux desktop 6.6.0 gcc_64
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
# Первый раз — конфигурация
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Сборка
cmake --build build

# Запуск
./build/loderunner
```

### Из VS Code

1. `Ctrl+Shift+P` → **CMake: Configure** (выбрать kit с Qt).
2. `Ctrl+Shift+P` → **CMake: Build** (или `F7`).
3. `F5` — запуск с отладчиком.

### Если CMake не находит Qt

Укажите путь явно:

```bash
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=/opt/Qt/6.6.0/gcc_64
```

---

## Структура проекта

```
loderunner/
├── CMakeLists.txt              # Описание сборки
├── main.cpp                    # Точка входа приложения
├── README.md                   # Этот файл
├── .gitignore                  # Что не коммитить
├── .vscode/                    # Настройки VS Code
│   ├── settings.json
│   ├── tasks.json
│   └── launch.json
├── src/                        # C++ бэкенд
│   ├── gameengine.h            # Классы GameEngine, TileModel, EnemyModel
│   └── gameengine.cpp
├── qml/                        # QML фронтенд
│   ├── main.qml                # Корневое окно, игровой цикл, ввод
│   └── components/             # Переиспользуемые компоненты
│       ├── Player.qml
│       ├── Enemy.qml
│       └── Tile.qml
└── assets/                     # Ресурсы (пока пусто)
    ├── images/
    └── sounds/
```

### Принцип разделения

- **C++** — вся логика: сетка, коллизии, гравитация, ИИ, счёт.
- **QML** — визуализация и ввод: как рисовать тайлы, как реагировать на клавиши.
- **CMake** — связывает это в один исполняемый файл через `qt_add_qml_module`.

---

## Управление в игре

| Клавиша | Действие |
|---------|----------|
| `←` `→` | Движение влево/вправо |
| `↑` `↓` | Лазание по лестницам, спуск по верёвкам |
| `Space` | Рыть яму в направлении взгляда |
| `Esc` | Выход |

---

## Как это работает: C++ ↔ QML

Это ключевой момент, который нужно понять в первую очередь.

### 1. Регистрация C++ типа в QML

В `gameengine.h`:

```cpp
class GameEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT          // ← автоматическая регистрация в QML
    Q_PROPERTY(int playerX READ playerX NOTIFY playerPositionChanged)
    // ...
};
```

Макрос `QML_ELEMENT` (требует `qt_standard_project_setup` в CMake) сообщает CMake: «этот класс должен быть доступен из QML». Никакого ручного `qmlRegisterType` писать не нужно.

### 2. Использование C++ типа в QML

В `main.qml`:

```qml
import LodeRunner 1.0   // модуль, объявленный в qt_add_qml_module

GameEngine {
    id: gameEngine
}

Text {
    text: "Player: " + gameEngine.playerX + ", " + gameEngine.playerY
}
```

`import LodeRunner 1.0` — это URI модуля из `CMakeLists.txt`. Поскольку `GameEngine` помечен `QML_ELEMENT`, он доступен по имени в этом модуле.

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

### Минимальный проект на Qt 6 с QML

```cmake
cmake_minimum_required(VERSION 3.21)
project(myapp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 COMPONENTS Quick Gui Qml REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(myapp
    main.cpp
    src/gameengine.cpp
    src/gameengine.h
)

qt_add_qml_module(myapp
    URI MyApp
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/Player.qml
)

target_link_libraries(myapp PRIVATE
    Qt6::Quick
    Qt6::Gui
    Qt6::Qml
)
```

### Ключевые команды

- `find_package(Qt6 ... REQUIRED)` — находит Qt 6.
- `qt_standard_project_setup(REQUIRES 6.5)` — устанавливает политики Qt 6.5+, включает автоматическую обработку `QML_ELEMENT`.
- `qt_add_executable(...)` — создаёт цель-приложение (по сути `add_executable` + настройки Qt).
- `qt_add_qml_module(...)` — регистрирует QML-файлы как модуль. Без этого QML не найдёт C++ типы.
- `target_link_libraries(...)` — линкует с Qt-модулями.

### Отладочная печать переменных

```cmake
message(STATUS "Qt found at: ${Qt6_DIR}")
message(FATAL_ERROR "Something is wrong")
```

### Типы сборки

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

---

## Отладка в VS Code

### `.vscode/launch.json`

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug LodeRunner",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/loderunner",
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
    "program": "${workspaceFolder}/build/loderunner",
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
- `Q_PROPERTY`, `Q_INVOKABLE`, `QML_ELEMENT`.
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

- `QQmlApplicationEngine`, `loadFromModule`.
- `import <ModuleURI> 1.0`.
- `Q_PROPERTY` + `NOTIFY` → реактивное обновление QML.
- `Q_INVOKABLE` — вызов методов из QML.
- `QAbstractListModel` — передача списков.

Практика: замените счётчик из этапа 2 на `Q_PROPERTY` и покажите его в QML.

### Этап 5. Игровая логика LodeRunner (2–3 недели)

Что делать:

- Сетка уровня: `QVector<QString>` как карта.
- Коллизии: «можно ли игроку войти в клетку».
- Гравитация: если под игроком пусто — падать.
- Копание: изменить тайл, обновить модель.
- Сбор золота: удалить тайл, увеличить счёт.
- Враг: движение, разворот, погоня, ловля игрока.
- Победа/проигрыш: условия и переходы состояний.

### Этап 6. Улучшения (по вкусу)

- Спрайты: `SpriteSequence`, PNG-атласы.
- Звук: `QSoundEffect` в QML.
- Несколько уровней: массив карт, переход между ними.
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

- [Qt 6 Documentation](https://doc.qt.io/qt-6/) — основной справочник.
- [QML Reference](https://doc.qt.io/qt-6/qmlreference.html) — все элементы QML.
- [Qt Quick Examples](https://doc.qt.io/qt-6/qtquick-examples.html) — примеры кода.

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

- [KGoldrunner](https://invent.kde.org/games/kgoldrunner) — Lode Runner на Qt Widgets.
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
| `Cannot assign to non-existent property` | C++ тип не зарегистрирован в QML | Проверить `QML_ELEMENT` и `qt_standard_project_setup` |
| `module "LodeRunner" is not installed` | Не совпадает URI в CMake и import в QML | Сверить `URI` и `import` |
| `undefined reference to vtable` | Забыт `Q_OBJECT` или moc не прогнан | Запустить CMake заново, проверить CMakeLists |
| QML не обновляется при изменении | Не запускается QML-кэш | Удалить `build/`, пересобрать |
| `No such file or directory: QApplication` | Забыт `#include <QApplication>` | Добавить инклюд |
| Программа падает при старте | Скорее всего, null pointer или отсутствующий QML | Запустить под отладчиком, посмотреть стек |
| `Permission denied (publickey)` при git push | SSH-ключ не добавлен на GitHub | См. раздел «Работа с Git» |

Всегда смотрите **вывод консоли** — Qt много и полезно ругается. Ошибки QML почти всегда указывают точный файл и строку.

---

## Лицензия

Учебный проект. Используйте и меняйте свободно.

---

*Последнее обновление: см. историю git.*