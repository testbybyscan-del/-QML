#include "gameengine.h"
#include <QtMath>

// Легенда уровня:
//  '#' кирпич, 'H' лестница, '-' верёвка, '$' золото,
//  'P' старт игрока, 'E' враг, 'X' вход/выход,
//  'x' защищённая клетка подхода к нему, '.' пусто
static const QVector<QString> kLevel1 = {
    "####################",
    "#..$....#$#.....$..#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#..$....#C#.....$..#",
    "#....####C####.....#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#....####C####.....#",
    "#.......#C#........#",
    "#..P..$........$.xX#",
    "####################"
};

// В нижней части уровня открыт боковой проход к призам (столбец 14).
// Каждый предусмотренный для толкания кристалл имеет доступную клетку назначения.
static const QVector<QString> kLevel2 = {
    "####################",
    "#..$....#.#....$...#",
    "#..###..#.#...###..#",
    "#....#...C...#.....#",
    "#...####.#.####.$..#",
    "#.###...#.#....###.#",
    "#..$..C.....#...$..#",
    "#...###......###...#",
    "#....#.............#",
    "#..#####...###.#...#",
    "#.$...#.....#..$$$.#",
    "#XxP..........$C.xX#",
    "####################"
};

// Третий уровень использует тот же формат карты, чтобы набор уровней
// позднее можно было заменить данными, создаваемыми генератором пейзажей.
static const QVector<QString> kLevel3 = {
    "####################",
    "#$..###....###...$.#",
    "#...#..C..C..#.....#",
    "#.###..C..C..###...#",
    "#...#..$...$..#....#",
    "##..###.C.###..##..#",
    "#...#..C.C..#......#",
    "#.$.#..C.C..#.$....#",
    "#...###...###...##.#",
    "#..$....C....$.....#",
    "#.####..C..####..$.#",
    "#XxP..#....#...$...#",
    "####################"
};

// Общая логика загрузки, снимков и переходов применяется к любому уровню списка.
static const QVector<QVector<QString>> kLevels = {kLevel1, kLevel2, kLevel3};

GameEngine::GameEngine(QObject *parent) : QObject(parent)
{
    m_levelSnapshots.resize(kLevels.size());
    loadLevel();
}

void GameEngine::loadLevel()
{
    for (QTimer *timer : m_dugTileTimers) {
        timer->stop();
        timer->deleteLater();
    }
    m_dugTileTimers.clear();

    const QVector<QString> &level = kLevels.at(m_currentLevel - 1);
    const LevelSnapshot &snapshot = m_levelSnapshots.at(m_currentLevel - 1);
    const bool restoringSnapshot = snapshot.valid;
    m_rows = level.size();
    m_columns = level.isEmpty() ? 0 : level.first().size();
    m_grid = restoringSnapshot ? snapshot.grid : level;
    if (restoringSnapshot) {
        m_playerX = snapshot.playerX;
        m_playerY = snapshot.playerY;
        m_prizesLeft = snapshot.prizesLeft;
        m_levelComplete = snapshot.levelComplete;
    } else {
        m_prizesLeft = 0;
        m_levelComplete = false;
    }

    QVector<TileModel::Tile> tiles;
    QVector<EnemyModel::Enemy> enemies;

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_columns; ++c) {
            QChar ch = m_grid[r][c];
            QString type;
            switch (ch.unicode()) {
            case '#': type = "brick"; break;
            case 'X': type = "exit"; break;
            case 'x': type = "empty"; break;
            case 'H': type = "ladder"; break;
            case '-': type = "rope"; break;
            case '$':
                type = "gold";
                if (!restoringSnapshot)
                    ++m_prizesLeft;
                break;
            case 'P':
                if (!restoringSnapshot) {
                    m_playerX = c;
                    m_playerY = r;
                    m_grid[r][c] = '.';
                }
                type = "empty";
                break;
            case 'C': type = "crystal"; break;
            case 'E':
                enemies.append({c, r, "run", true});
                type = "empty";
                break;
            default:
                type = "empty";
            }
            tiles.append({r, c, type});
        }
    }
    m_tilesModel.setTiles(tiles);
    m_enemiesModel.setEnemies(enemies);

    emit levelChanged();
    emit playerPositionChanged();
    emit playerStateChanged();
    emit scoreChanged();
    emit statusChanged();
}

QChar GameEngine::tileAt(int row, int col) const
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns) return '#';
    return m_grid[row][col];
}

void GameEngine::setTileAt(int row, int col, QChar ch)
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns) return;
    m_grid[row][col] = ch;
}

bool GameEngine::playerCanEnter(int row, int col) const
{
    QChar ch = tileAt(row, col);
    return ch != '#' && ch != 'C';
}

void GameEngine::movePlayer(int dx, int dy)
{
    if (m_gameOver) return;

    if (dx != 0) m_playerFacingRight = (dx > 0);

    int nx = m_playerX + dx;
    int ny = m_playerY + dy;

    if (dy == 0 && dx != 0 && tileAt(ny, nx) == 'C') {
        const int destinationX = nx + dx;
        const auto enemyAt = [this, ny](int col) {
            for (const auto &enemy : m_enemiesModel.enemies()) {
                if (enemy.x == col && enemy.y == ny)
                    return true;
            }
            return false;
        };

        if (tileAt(ny, destinationX) != '.'
                || (ny == m_playerY && destinationX == m_playerX)
                || enemyAt(destinationX))
            return;

        // Push exactly one crystal by one cell; the crystal above it then follows normal gravity.
        setTileAt(ny, nx, '.');
        setTileAt(ny, destinationX, 'C');
        m_tilesModel.updateTile(ny, nx, "empty");
        m_tilesModel.updateTile(ny, destinationX, "crystal");
    }

    if (playerCanEnter(ny, nx)) {
        m_playerX = nx;
        m_playerY = ny;

        // Собираем золото
        if (tileAt(ny, nx) == '$') {
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, "empty");
            m_score += 100;
            --m_prizesLeft;
            emit scoreChanged();
            if (m_prizesLeft == 0) {
                if (m_currentLevel < kLevels.size()) {
                    switchToLevel(m_currentLevel + 1);
                    return;
                }
                m_levelComplete = true;
                emit statusChanged();
            }
        }

        if (dy < 0) m_playerState = "climb";
        else if (dx != 0) m_playerState = "run";
        else m_playerState = "idle";

        emit playerPositionChanged();
        emit playerStateChanged();

        if (m_playerY == m_rows - 2 && m_playerX == 1 && m_currentLevel > 1)
            switchToLevel(m_currentLevel - 1);
        else if (m_playerY == m_rows - 2 && m_playerX == m_columns - 2
                 && m_prizesLeft == 0 && m_currentLevel < kLevels.size())
            switchToLevel(m_currentLevel + 1);
    }
}

void GameEngine::saveCurrentLevel()
{
    LevelSnapshot &snapshot = m_levelSnapshots[m_currentLevel - 1];
    snapshot.grid = m_grid;
    snapshot.playerX = m_playerX;
    snapshot.playerY = m_playerY;
    snapshot.prizesLeft = m_prizesLeft;
    snapshot.levelComplete = m_levelComplete;
    snapshot.valid = true;
}

void GameEngine::switchToLevel(int targetLevel)
{
    if (targetLevel < 1 || targetLevel > kLevels.size()
            || targetLevel == m_currentLevel || m_gameOver)
        return;

    saveCurrentLevel();
    m_currentLevel = targetLevel;
    m_playerState = "idle";
    loadLevel();
}

void GameEngine::dig()
{
    if (m_gameOver || m_levelComplete) return;

    int dx = m_playerFacingRight ? 1 : -1;
    int bx = m_playerX + dx;
    int by = m_playerY + 1; // копаем вниз-вбок (классика Lode Runner)

    const bool insidePerimeter = by > 0 && by < m_rows - 1
            && bx > 0 && bx < m_columns - 1;
    if (insidePerimeter && tileAt(by, bx) == '#') {
        // Dug cells become empty in the simulation grid, so crystals can fall or tumble into them.
        setTileAt(by, bx, '.');
        m_tilesModel.updateTile(by, bx, "dug");

        const int tileKey = by * m_columns + bx;
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_dugTileTimers.insert(tileKey, timer);
        connect(timer, &QTimer::timeout, this, [this, by, bx, tileKey, timer]() {
            m_dugTileTimers.remove(tileKey);
            if (tileAt(by, bx) == '.') {
                setTileAt(by, bx, '#');
                m_tilesModel.updateTile(by, bx, "brick");
            }
            timer->deleteLater();
        });
        timer->start(15000);

        // Освобождённая клетка сразу участвует в симуляции:
        // предмет над ней начинает падать в этом же действии копания.
        updateCrystals();

        m_playerState = "dig";
        emit playerStateChanged();
    }
}

void GameEngine::updateCrystals()
{
    const auto isGem = [this](int row, int col) {
        const QChar tile = tileAt(row, col);
        return tile == 'C' || tile == '$';
    };
    const auto gemType = [](QChar tile) {
        return tile == '$' ? QStringLiteral("gold") : QStringLiteral("crystal");
    };
    // A dug tile is stored as '.', so it remains available to crystal movement until rebuilt.
    const auto isEmpty = [this](int row, int col) {
        return tileAt(row, col) == '.' && !(row == m_playerY && col == m_playerX);
    };

    for (int row = m_rows - 2; row >= 1; --row) {
        for (int col = 1; col < m_columns - 1; ++col) {
            if (!isGem(row, col))
                continue;
            const QChar fallingGem = tileAt(row, col);

            // A vertical chain falls as one unit when the cell under its bottom gem is free.
            int chainTop = row;
            while (chainTop > 1 && isGem(chainTop - 1, col))
                --chainTop;
            if (chainTop < row && isEmpty(row + 1, col)) {
                for (int sourceRow = row; sourceRow >= chainTop; --sourceRow) {
                    const int destinationRow = sourceRow + 1;
                    const QChar shiftedGem = tileAt(sourceRow, col);
                    setTileAt(destinationRow, col, shiftedGem);
                    setTileAt(sourceRow, col, '.');
                    m_tilesModel.updateTile(destinationRow, col, gemType(shiftedGem));
                    m_tilesModel.updateTile(sourceRow, col, "empty");
                }
                return;
            }

            // Для падения нужна свободная клетка именно снизу.
            // Боковые стены не должны удерживать объект над пустотой.
            if (isEmpty(row + 1, col)) {
                setTileAt(row, col, '.');
                setTileAt(row + 1, col, fallingGem);
                m_tilesModel.updateTile(row, col, "empty");
                m_tilesModel.updateTile(row + 1, col, gemType(fallingGem));
                return;
            }

            if (!isGem(row + 1, col))
                continue;

            // B sits on A. It can tumble around A if both cells on one side
            // of the pair are clear; the rest of their vertical chain follows down.
            int tumbleDirection = 0;
            for (int direction : {1, -1}) {
                if (isEmpty(row, col + direction)
                        && isEmpty(row + 1, col + direction)) {
                    tumbleDirection = direction;
                    break;
                }
            }
            if (tumbleDirection == 0)
                continue;

            const QChar upperGem = tileAt(row, col);
            const QChar tumblingGem = tileAt(row + 1, col);
            setTileAt(row, col, '.');
            setTileAt(row + 1, col + tumbleDirection, tumblingGem);
            setTileAt(row + 1, col, upperGem);
            m_tilesModel.updateTile(row, col, "empty");
            m_tilesModel.updateTile(row + 1, col + tumbleDirection, gemType(tumblingGem));
            m_tilesModel.updateTile(row + 1, col, gemType(upperGem));

            for (int sourceRow = row - 1; sourceRow >= chainTop; --sourceRow) {
                const int destinationRow = sourceRow + 1;
                const QChar shiftedGem = tileAt(sourceRow, col);
                setTileAt(destinationRow, col, shiftedGem);
                setTileAt(sourceRow, col, '.');
                m_tilesModel.updateTile(destinationRow, col, gemType(shiftedGem));
                m_tilesModel.updateTile(sourceRow, col, "empty");
            }
            return;
        }
    }
}

void GameEngine::updateGame()
{
    if (m_gameOver || m_levelComplete) return;

    // Простая гравитация игрока: если под ним пустота — падаем.
    QChar below = tileAt(m_playerY + 1, m_playerX);
    bool supported = (below == '#' || below == 'H' || below == '-');
    QChar current = tileAt(m_playerY, m_playerX);
    bool onClimbable = (current == 'H' || current == '-');

    if (!supported && !onClimbable && m_playerState != "climb") {
        // Падение
        if (playerCanEnter(m_playerY + 1, m_playerX)) {
            ++m_playerY;
            m_playerState = "fall";
            emit playerPositionChanged();
            emit playerStateChanged();
        }
    } else if (m_playerState == "fall") {
        m_playerState = "idle";
        emit playerStateChanged();
    }

    // Движение врагов — простейшее: шаг в сторону, разворот у стены.
    auto &enemies = m_enemiesModel.enemies();
    bool changed = false;
    for (auto &e : enemies) {
        int dx = e.facingRight ? 1 : -1;
        int nx = e.x + dx;
        if (tileAt(e.y, nx) == '#') {
            e.facingRight = !e.facingRight;
            changed = true;
        } else {
            // Гравитация для врага
            while (e.y + 1 < m_rows && tileAt(e.y + 1, e.x) != '#' && tileAt(e.y + 1, e.x) != 'H') {
                ++e.y;
                changed = true;
            }
            e.x = nx;
            changed = true;
        }

        // Ловим игрока
        if (e.x == m_playerX && e.y == m_playerY) {
            --m_lives;
            emit scoreChanged();
            if (m_lives <= 0) {
                m_gameOver = true;
                emit statusChanged();
            } else {
                // Возвращаем игрока на старт
                loadLevel();
                return;
            }
        }
    }
    if (changed) m_enemiesModel.refresh();
}
