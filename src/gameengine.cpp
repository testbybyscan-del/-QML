#include "gameengine.h"
#include <QtMath>

// Легенда уровня:
//  '#' кирпич, 'H' лестница, '-' верёвка, '$' золото,
//  'P' старт игрока, 'E' враг, '.' пусто
static const QVector<QString> kLevel = {
    "####################",
    "#.......#C#........#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#....####C####.....#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#.......#C#........#",
    "#....####C####.....#",
    "#.......#C#........#",
    "#..P...............#",
    "####################"
};

GameEngine::GameEngine(QObject *parent) : QObject(parent)
{
    loadLevel();
}

void GameEngine::loadLevel()
{
    for (QTimer *timer : m_dugTileTimers) {
        timer->stop();
        timer->deleteLater();
    }
    m_dugTileTimers.clear();

    m_rows = kLevel.size();
    m_columns = kLevel.isEmpty() ? 0 : kLevel.first().size();
    m_grid = kLevel;

    QVector<TileModel::Tile> tiles;
    QVector<EnemyModel::Enemy> enemies;

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_columns; ++c) {
            QChar ch = m_grid[r][c];
            QString type;
            switch (ch.unicode()) {
            case '#': type = "brick"; break;
            case 'H': type = "ladder"; break;
            case '-': type = "rope"; break;
            case '$': type = "gold"; break;
            case 'P':
                m_playerX = c; m_playerY = r;
                m_grid[r][c] = '.';
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
    if (m_gameOver || m_levelComplete) return;

    if (dx != 0) m_playerFacingRight = (dx > 0);

    int nx = m_playerX + dx;
    int ny = m_playerY + dy;

    if (playerCanEnter(ny, nx)) {
        m_playerX = nx;
        m_playerY = ny;

        // Собираем золото
        if (tileAt(ny, nx) == '$') {
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, "empty");
            m_score += 100;
            emit scoreChanged();
        }

        if (dy < 0) m_playerState = "climb";
        else if (dx != 0) m_playerState = "run";
        else m_playerState = "idle";

        emit playerPositionChanged();
        emit playerStateChanged();
    }
}

void GameEngine::dig()
{
    if (m_gameOver || m_levelComplete) return;

    int dx = m_playerFacingRight ? 1 : -1;
    int bx = m_playerX + dx;
    int by = m_playerY + 1; // копаем вниз-вбок (классика Lode Runner)

    if (tileAt(by, bx) == '#') {
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

        m_playerState = "dig";
        emit playerStateChanged();
    }
}

void GameEngine::updateCrystals()
{
    // A dug tile is stored as '.', so it remains available to crystal movement until rebuilt.
    const auto isEmpty = [this](int row, int col) {
        return tileAt(row, col) == '.' && !(row == m_playerY && col == m_playerX);
    };

    for (int row = m_rows - 2; row >= 1; --row) {
        for (int col = 1; col < m_columns - 1; ++col) {
            if (tileAt(row, col) != 'C')
                continue;

            if (tileAt(row + 1, col) == '.') {
                if (!isEmpty(row + 1, col)
                        || !isEmpty(row, col - 1)
                        || !isEmpty(row, col + 1))
                    continue;

                setTileAt(row, col, '.');
                setTileAt(row + 1, col, 'C');
                m_tilesModel.updateTile(row, col, "empty");
                m_tilesModel.updateTile(row + 1, col, "crystal");
                return;
            }

            if (tileAt(row + 1, col) != 'C')
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

            int chainTop = row;
            while (chainTop > 1 && tileAt(chainTop - 1, col) == 'C')
                --chainTop;

            setTileAt(row, col, '.');
            setTileAt(row + 1, col + tumbleDirection, 'C');
            m_tilesModel.updateTile(row, col, "empty");
            m_tilesModel.updateTile(row + 1, col + tumbleDirection, "crystal");

            for (int sourceRow = row - 1; sourceRow >= chainTop; --sourceRow) {
                const int destinationRow = sourceRow + 1;
                setTileAt(destinationRow, col, 'C');
                setTileAt(sourceRow, col, '.');
                m_tilesModel.updateTile(destinationRow, col, "crystal");
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
