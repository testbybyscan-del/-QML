# Наборы тайлов Kenney

Здесь хранятся исходные архивы и распакованные изображения для экспериментов с внешним видом игровых клеток. Эти наборы содержат **графические PNG-тайлы, а не готовые QML-компоненты**. Их можно подключать к `Tile.qml` как текстуры и создавать несколько QML-вариантов представления, не меняя физические символы карты.

Все три набора рассчитаны на сетку 18×18 пикселей. В проекте клетка имеет размер 32×32, поэтому спрайты потребуется масштабировать. Для чёткого пиксельного вида в `Image` следует отключить сглаживание (`smooth: false`). Архивы и вложенные файлы лицензий сохранены рядом с распакованными изображениями.

| Набор | Варианты оформления | Официальная страница | Лицензия |
|---|---|---|---|
| Pixel Platformer 1.2 | Земля, трава, камень, вода, объекты и фоны | <https://kenney.nl/assets/pixel-platformer> | CC0 1.0 |
| Pixel Platformer Blocks 1.0 | Несколько видов камня и блоков | <https://kenney.nl/assets/pixel-platformer-blocks> | CC0 1.0 |
| Pixel Platformer: Industrial Expansion 1.0 | Металл, механические и промышленные блоки | <https://kenney.nl/assets/pixel-platformer-industrial-expansion> | CC0 1.0 |

В каждом распакованном наборе находится `License.txt` с условиями CC0. Указание автора не обязательно; источник и содержимое лицензии сохранены для удобства проекта.

## Файлы

Рядом с этим файлом лежат исходные zip-архивы `kenney_*.zip`. Распакованные наборы находятся в каталогах `pixel-platformer/`, `pixel-platformer-blocks/` и `pixel-platformer-industrial-expansion/`; в них оставлены PNG, листы тайлов (`Tilemap/`), превью, лицензии и таблицы. Вспомогательные демо-проекты сохранены только в исходных архивах.

## Выбранные изображения

Пути в таблице указаны относительно этого каталога. Все выбранные файлы — PNG размером 18×18 с альфа-каналом; фон превью не используется. Изображения проверены отдельно и в галерее при размере клетки 32×32. Границы и прозрачность можно сравнить на двухцветном фоне. При масштабировании задано `smooth: false`.

| Тема / объект | Файл | Источник и лицензия |
|---|---|---|
| Kenney / Кирпич | `pixel-platformer/Tiles/tile_0006.png` | Pixel Platformer 1.2, `pixel-platformer/License.txt`, CC0 |
| Kenney / Кристалл | `pixel-platformer/Tiles/tile_0067.png` | Pixel Platformer 1.2, CC0 |
| Kenney / Приз | `pixel-platformer/Tiles/tile_0151.png` | Pixel Platformer 1.2, CC0 |
| Industrial / Кирпич | `pixel-platformer-industrial-expansion/Tiles/tile_0038.png` | Industrial Expansion 1.0, `pixel-platformer-industrial-expansion/License.txt`, CC0 |
| Industrial / Кристалл (синий индикатор) | `pixel-platformer-industrial-expansion/Tiles/tile_0065.png` | Industrial Expansion 1.0, CC0 |
| Industrial / Приз (жёлтый жетон) | `pixel-platformer-industrial-expansion/Tiles/tile_0057.png` | Industrial Expansion 1.0, CC0 |
| Дополнительный вариант Blocks | `pixel-platformer-blocks/Tiles/Stone/tile_0000.png` | Blocks 1.0, `pixel-platformer-blocks/License.txt`, CC0 |

Шесть изображений тем и дополнительный Blocks включены в `qml.qrc`. Blocks показан отдельно как кандидат для будущего оформления. Индустриальные индикатор и жетон служат представлениями Кристалла и Приза; правила этих объектов задаёт движок.


## Персонажи

Выбраны спрайты из `pixel-platformer/Tiles/Characters/`. В отличие от тайлов поля они имеют размер **24×24**, что подтверждено таблицей `Tilesheet (Characters).txt` и размерами PNG. Все имеют прозрачный фон и включены в `qml.qrc`. Источник — Pixel Platformer 1.2, лицензия CC0 из `pixel-platformer/License.txt`.

| Вариант | Файлы в `Tiles/Characters/` |
|---|---|
| Зелёный космонавт | `tile_0000.png`, `tile_0001.png` |
| Розовый космонавт | `tile_0004.png`, `tile_0005.png` |
| Робот | `tile_0018.png`, `tile_0019.png`, `tile_0020.png` |

Классический зелёный Персонаж рисуется средствами QML и не использует PNG. Превью кадров проверены визуально; варианты показаны рядом в галерее.
