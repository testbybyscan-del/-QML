#include "gameengine.h"
#include "landscapegenerator.h"
#include <QtMath>
#include <QDebug>
#include <QSettings>

GameEngine::GameEngine(QObject *parent) : QObject(parent)
{
    QSettings settings;
    m_highScore = qMax(0, settings.value(QStringLiteral("highScore"), 0).toInt());
}

void GameEngine::setPaused(bool paused)
{
    if (m_paused == paused) return;
    m_paused = paused;
    if (paused) {
        for (auto it = m_dugTileTimers.cbegin(); it != m_dugTileTimers.cend(); ++it) {
            if (it.value()->isActive()) {
                m_pausedDugRemaining.insert(it.key(), qMax(1, it.value()->remainingTime()));
                it.value()->stop();
            }
        }
    } else {
        for (auto it = m_pausedDugRemaining.cbegin(); it != m_pausedDugRemaining.cend(); ++it) {
            if (auto *timer = m_dugTileTimers.value(it.key(), nullptr))
                timer->start(it.value());
        }
        m_pausedDugRemaining.clear();
    }
    emit pausedChanged();
}

void GameEngine::addScore(int points)
{
    if (points <= 0)
        return;

    m_score += points;
    if (m_score > m_highScore) {
        m_highScore = m_score;
        QSettings settings;
        settings.setValue(QStringLiteral("highScore"), m_highScore);
    }
    emit scoreChanged();
}

void GameEngine::initializeLevels(const QVariantList &authoredLevels)
{
    QVector<QVector<QString>> levels;
    levels.reserve(authoredLevels.size());

    for (const QVariant &levelValue : authoredLevels) {
        const QVariantList rows = levelValue.toList();
        if (rows.isEmpty()) {
            qWarning() << "Авторский QML-уровень не содержит строк";
            return;
        }

        QVector<QString> levelRows;
        levelRows.reserve(rows.size());
        int expectedWidth = -1;
        int playerCount = 0;
        int exitCount = 0;
        for (const QVariant &rowValue : rows) {
            const QString row = rowValue.toString();
            if (row.isEmpty() || (expectedWidth >= 0 && row.size() != expectedWidth)) {
                qWarning() << "Строки авторского QML-уровня должны иметь"
                              " одинаковую ненулевую ширину";
                return;
            }
            const QString validTiles = QStringLiteral("#.$PXCxSH-EKB*");
            for (QChar tile : row) {
                if (!validTiles.contains(tile)) {
                    qWarning() << "Неизвестный символ карты в QML-уровне:" << tile;
                    return;
                }
            }
            expectedWidth = row.size();
            playerCount += row.count(QLatin1Char('P'));
            exitCount += row.count(QLatin1Char('X'));
            levelRows.append(row);
        }
        if (playerCount != 1) {
            qWarning() << "В каждом авторском QML-уровне должна быть ровно"
                          " одна стартовая позиция P";
            return;
        }
        if (exitCount == 0) {
            qWarning() << "В каждом авторском QML-уровне должен быть хотя бы один выход X";
            return;
        }
        levels.append(levelRows);
    }

    if (levels.isEmpty()) {
        qWarning() << "GameEngine получил пустой список авторских QML-уровней";
        return;
    }

    // Ручные QML-карты передаются первыми. Сгенерированные уровни затем
    // добавляются в этот же список и используют общую загрузку и снимки.
    m_levels = levels;
    m_authoredLevelCount = levels.size();
    if (m_hasKey) {
        m_hasKey = false;
        emit hasKeyChanged();
    }
    m_keyRequiredNotice = false;
    m_keyCollectedNotice = false;
    m_safeOpenedNotice = false;
    m_keyNoticeShownForVisit = false;
    m_nearLockedSafe = false;
    m_levelSnapshots.clear();
    m_levelSnapshots.resize(m_levels.size());
    m_currentLevel = 1;
    m_playerMovedSinceCrystalUpdate = false;
    loadLevel();
}

void GameEngine::restartGame()
{
    if (m_authoredLevelCount <= 0 || m_levels.isEmpty())
        return;

    m_levels.resize(m_authoredLevelCount);
    m_levelSnapshots.clear();
    m_levelSnapshots.resize(m_authoredLevelCount);
    m_currentLevel = 1;
    m_landscapeSeed = 0;
    m_score = 0;
    m_prizesLeft = 0;
    m_lives = 3;
    if (m_hasKey) {
        m_hasKey = false;
        emit hasKeyChanged();
    }
    m_gameOver = false;
    m_levelComplete = false;
    m_keyRequiredNotice = false;
    m_keyCollectedNotice = false;
    m_safeOpenedNotice = false;
    m_keyNoticeShownForVisit = false;
    m_nearLockedSafe = false;
    m_playerState = "idle";
    m_playerFacingRight = true;
    m_playerMovedSinceCrystalUpdate = false;
    m_fallingCrystalCells.clear();
    m_pitPrizeDugCellByPosition.clear();
    loadLevel();
}

void GameEngine::dismissKeyRequiredNotice()
{
    if (!m_keyRequiredNotice)
        return;
    m_keyRequiredNotice = false;
    emit statusChanged();
}

void GameEngine::dismissKeyCollectedNotice()
{
    if (!m_keyCollectedNotice)
        return;
    m_keyCollectedNotice = false;
    emit statusChanged();
}

void GameEngine::dismissSafeOpenedNotice()
{
    if (!m_safeOpenedNotice)
        return;
    m_safeOpenedNotice = false;
    emit statusChanged();
}

bool GameEngine::hasActiveNotice() const
{
    return m_keyRequiredNotice || m_keyCollectedNotice || m_safeOpenedNotice;
}

void GameEngine::loadLevel()
{
    for (QTimer *timer : m_dugTileTimers) {
        timer->stop();
        timer->deleteLater();
    }
    m_dugTileTimers.clear();
    m_pausedDugRemaining.clear();
    m_fallingCrystalCells.clear();
    m_keyNoticeShownForVisit = false;
    m_keyCollectedNotice = false;
    m_safeOpenedNotice = false;

    const QVector<QString> &level = m_levels.at(m_currentLevel - 1);
    const LevelSnapshot &snapshot = m_levelSnapshots.at(m_currentLevel - 1);
    const bool restoringSnapshot = snapshot.valid;
    m_rows = level.size();
    m_columns = level.isEmpty() ? 0 : level.first().size();
    m_grid = restoringSnapshot ? snapshot.grid : level;
    if (restoringSnapshot) {
        m_playerX = snapshot.playerX;
        m_playerY = snapshot.playerY;
        m_pitPrizeDugCellByPosition = snapshot.pitPrizeDugCellByPosition;
    } else {
        m_pitPrizeDugCellByPosition.clear();
    }
    m_prizesLeft = 0;
    m_fallingCrystalCells.clear();

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
            case 'B': type = "safe"; break;
            case 'K': type = "key"; break;
            case 'H': type = "ladder"; break;
            case '-': type = "rope"; break;
            case '$':
                type = "gold";
                ++m_prizesLeft;
                break;
            case 'S': type = "superPrize"; break;
            case '*': type = "bonusPrize"; break;
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
    m_levelComplete = m_prizesLeft == 0;
    m_nearLockedSafe = !m_hasKey && isAdjacentToLockedSafe(m_playerY, m_playerX);
    m_keyRequiredNotice = m_nearLockedSafe;
    m_keyNoticeShownForVisit = m_keyRequiredNotice;

    emit levelChanged();
    emit playerPositionChanged();
    emit playerStateChanged();
    emit scoreChanged();
    emit statusChanged();
    m_playerMovedSinceCrystalUpdate = false;
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
    if (row < 0 || row >= m_rows || col < 0 || col >= m_columns)
        return false;

    QChar ch = tileAt(row, col);
    if (ch == QLatin1Char('B') && !m_hasKey)
        return false;
    return ch != '#' && ch != 'C';
}

bool GameEngine::isAdjacentToLockedSafe(int row, int col) const
{
    if (m_hasKey)
        return false;

    return tileAt(row - 1, col) == 'B' || tileAt(row + 1, col) == 'B'
            || tileAt(row, col - 1) == 'B' || tileAt(row, col + 1) == 'B';
}

void GameEngine::generateNextLandscape()
{
    if (m_paused || m_gameOver || hasActiveNotice())
        return;

    // Seed скрыт от игрока и увеличивается при каждой попытке генерации.
    // Если все кандидаты будут отклонены, повторный вход в выход запустит
    // новую воспроизводимую попытку с другим seed.
    const int nextSeed = m_landscapeSeed + 1;
    m_landscapeSeed = nextSeed;
    QString error;
    const QVector<QString> generated = LandscapeGenerator::generate(
            static_cast<quint32>(nextSeed), &error);
    if (generated.isEmpty()) {
        // Если правила генератора не смогли собрать корректную карту,
        // текущий уровень и позиция игрока остаются без изменений.
        qWarning() << "Landscape generation failed:" << error;
        return;
    }

    // Персонаж уже вошёл в выходную клетку только после сбора всех обязательных
    // Призов. Сохраняем текущую карту, чтобы левый портал позволял вернуться.
    if (m_currentLevel >= 1 && m_currentLevel <= m_levels.size())
        saveCurrentLevel();

    // Каждый выход создаёт следующий уровень, не затирая предыдущий.
    // Благодаря снимкам игрок может вернуться через левый портал.
    m_levels.append(generated);
    m_levelSnapshots.append(LevelSnapshot{});
    m_currentLevel = m_levels.size();
    m_playerState = "idle";
    loadLevel();
}

void GameEngine::movePlayer(int dx, int dy)
{
    if (m_paused || m_gameOver || hasActiveNotice()) return;

    if (dx != 0) m_playerFacingRight = (dx > 0);

    int nx = m_playerX + dx;
    int ny = m_playerY + dy;

    if (tileAt(ny, nx) == 'B' && !m_hasKey) {
        m_nearLockedSafe = true;
        if (!m_keyNoticeShownForVisit) {
            m_keyNoticeShownForVisit = true;
            m_keyRequiredNotice = true;
            emit statusChanged();
        }
        return;
    }

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
        m_fallingCrystalCells.remove(ny * m_columns + nx);
        m_fallingCrystalCells.remove(ny * m_columns + destinationX);
        setTileAt(ny, nx, '.');
        setTileAt(ny, destinationX, 'C');
        m_tilesModel.updateTile(ny, nx, "empty");
        m_tilesModel.updateTile(ny, destinationX, "crystal");
    }

    if (playerCanEnter(ny, nx)) {
        m_playerX = nx;
        m_playerY = ny;
        m_playerMovedSinceCrystalUpdate = true;

        // Обычный Приз участвует в условии перехода, дополнительный Приз — нет.
        const QChar collectedTile = tileAt(ny, nx);
        const int destinationCellKey = ny * m_columns + nx;
        if (collectedTile == '$' || collectedTile == '*') {
            int dugCellKey = -1;
            if (collectedTile == '*') {
                auto pitCell = m_pitPrizeDugCellByPosition.find(destinationCellKey);
                if (pitCell != m_pitPrizeDugCellByPosition.end()) {
                    dugCellKey = pitCell.value();
                    m_pitPrizeDugCellByPosition.erase(pitCell);
                }
            }
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, dugCellKey >= 0 ? "dug" : "empty");
            if (collectedTile == '$')
                --m_prizesLeft;
            addScore(100);
            if (dugCellKey >= 0) {
                const int dugRow = dugCellKey / m_columns;
                const int dugColumn = dugCellKey % m_columns;
                m_tilesModel.updateTile(dugRow, dugColumn, "dug");
                startDugTileTimer(dugCellKey / m_columns, dugCellKey % m_columns);
            }
            if (collectedTile == '$' && m_prizesLeft == 0) {
                // Сбор наград открывает возможность продолжить, но сам по себе
                // не покидает экран: переход совершается только у правого выхода.
                m_levelComplete = true;
                emit statusChanged();
            }
        }

        // Суперприз — отдельная награда: он даёт 25 очков, но не уменьшает
        // количество обязательных обычных Призов для завершения уровня.
        if (tileAt(ny, nx) == 'S') {
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, "empty");
            addScore(25);
        }

        // Ключ переносится между экранами: его символ удаляется с карты,
        // а состояние хранится в GameEngine до открытия Сейфа.
        if (tileAt(ny, nx) == 'K') {
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, "empty");
            m_hasKey = true;
            m_nearLockedSafe = false;
            m_keyRequiredNotice = false;
            m_keyCollectedNotice = true;
            emit hasKeyChanged();
            emit statusChanged();
        }

        // Сейф остаётся непроходимым без Ключа. При открытии он исчезает,
        // Ключ расходуется, а игрок получает отдельные 100 очков.
        if (tileAt(ny, nx) == 'B' && m_hasKey) {
            setTileAt(ny, nx, '.');
            m_tilesModel.updateTile(ny, nx, "empty");
            m_hasKey = false;
            m_safeOpenedNotice = true;
            emit hasKeyChanged();
            addScore(100);
            emit statusChanged();
        }

        const bool nowNearLockedSafe = isAdjacentToLockedSafe(m_playerY, m_playerX);
        if (nowNearLockedSafe && !m_nearLockedSafe) {
            if (!m_keyNoticeShownForVisit) {
                m_keyNoticeShownForVisit = true;
                m_keyRequiredNotice = true;
                emit statusChanged();
            }
        } else if (!nowNearLockedSafe && m_keyRequiredNotice) {
            m_keyRequiredNotice = false;
            emit statusChanged();
        }
        m_nearLockedSafe = nowNearLockedSafe;

        if (dy < 0) m_playerState = "climb";
        else if (dx != 0) m_playerState = "run";
        else m_playerState = "idle";

        emit playerPositionChanged();
        emit playerStateChanged();

        if (m_playerY == m_rows - 2 && m_playerX == 1
                && tileAt(m_playerY, m_playerX) == 'X' && m_currentLevel > 1) {
            switchToLevel(m_currentLevel - 1);
        } else if (m_playerY == m_rows - 2 && m_playerX == m_columns - 2
                   && tileAt(m_playerY, m_playerX) == 'X' && m_prizesLeft == 0) {
            // Сначала проходим заранее созданные ручные карты. Когда следующего
            // уровня ещё нет, выход запускает генератор и добавляет новый.
            const int nextLevel = m_currentLevel + 1;
            if (nextLevel <= m_levels.size())
                switchToLevel(nextLevel);
            else
                generateNextLandscape();
        }
    }
}

void GameEngine::saveCurrentLevel()
{
    LevelSnapshot &snapshot = m_levelSnapshots[m_currentLevel - 1];
    snapshot.grid = m_grid;
    snapshot.playerX = m_playerX;
    snapshot.playerY = m_playerY;
    snapshot.pitPrizeDugCellByPosition = m_pitPrizeDugCellByPosition;
    snapshot.valid = true;
}

void GameEngine::switchToLevel(int targetLevel)
{
    if (targetLevel < 1 || targetLevel > m_levels.size()
            || targetLevel == m_currentLevel || m_gameOver || hasActiveNotice())
        return;

    saveCurrentLevel();
    m_currentLevel = targetLevel;
    m_playerState = "idle";
    loadLevel();
}

void GameEngine::startDugTileTimer(int row, int col)
{
    const int tileKey = row * m_columns + col;
    auto existingTimer = m_dugTileTimers.find(tileKey);
    if (existingTimer != m_dugTileTimers.end()) {
        existingTimer.value()->stop();
        existingTimer.value()->deleteLater();
        m_dugTileTimers.erase(existingTimer);
    }

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    m_dugTileTimers.insert(tileKey, timer);
    connect(timer, &QTimer::timeout, this, [this, row, col, tileKey, timer]() {
        if (m_paused) {
            m_pausedDugRemaining.insert(tileKey, 1);
            return;
        }
        m_dugTileTimers.remove(tileKey);
        if (m_gameOver) {
            timer->deleteLater();
            return;
        }
        if (hasActiveNotice()) {
            m_dugTileTimers.insert(tileKey, timer);
            timer->start(100);
            return;
        }
        if (tileAt(row, col) == '.') {
            setTileAt(row, col, '#');
            m_tilesModel.updateTile(row, col, "brick");
            if (m_playerX == col && m_playerY == row)
                loseLife();
        } else if (tileAt(row, col) == 'C') {
            // Оставшийся в яме Кристалл становится Призом. Захват Приза
            // запустит новый полный таймер восстановления Кирпича.
            setTileAt(row, col, '*');
            m_pitPrizeDugCellByPosition.insert(tileKey, tileKey);
            m_fallingCrystalCells.remove(tileKey);
            m_tilesModel.updateTile(row, col, "bonusPrize");
        }
        timer->deleteLater();
    });
    if (m_paused) m_pausedDugRemaining.insert(tileKey, 15000);
    else timer->start(15000);
}

void GameEngine::dig()
{
    // Сбор всех обязательных Призов открывает выход, но не блокирует действия
    // игрока на текущем уровне до фактического перехода.
    if (m_paused || m_gameOver || hasActiveNotice()) return;

    int dx = m_playerFacingRight ? 1 : -1;
    int bx = m_playerX + dx;
    int by = m_playerY + 1; // копаем вниз-вбок

    const bool insidePerimeter = by > 0 && by < m_rows - 1
            && bx > 0 && bx < m_columns - 1;
    if (insidePerimeter && tileAt(by, bx) == '#') {
        // Dug cells become empty in the simulation grid, so crystals can fall or tumble into them.
        setTileAt(by, bx, '.');
        m_tilesModel.updateTile(by, bx, "dug");

        startDugTileTimer(by, bx);

        // Освобождённая клетка сразу участвует в симуляции:
        // предмет над ней начинает падать в этом же действии копания.
        m_playerState = "dig";
        emit playerStateChanged();
        updateCrystals();
    }
}

void GameEngine::updateCrystals()
{
    if (m_paused || m_gameOver || hasActiveNotice())
        return;

    const bool playerMovedSinceLastUpdate = m_playerMovedSinceCrystalUpdate;
    m_playerMovedSinceCrystalUpdate = false;

    const auto isGem = [this](int row, int col) {
        const QChar tile = tileAt(row, col);
        return tile == 'C' || tile == '$' || tile == 'S' || tile == '*';
    };
    const auto gemType = [](QChar tile) {
        if (tile == '$') return QStringLiteral("gold");
        if (tile == 'S') return QStringLiteral("superPrize");
        if (tile == '*') return QStringLiteral("bonusPrize");
        return QStringLiteral("crystal");
    };
    const auto cellKey = [this](int row, int col) {
        return row * m_columns + col;
    };
    const auto moveFallingState = [this, &cellKey](int fromRow, int fromCol,
                                                  int toRow, int toCol, QChar gem) {
        m_fallingCrystalCells.remove(cellKey(fromRow, fromCol));
        if (gem == 'C')
            m_fallingCrystalCells.insert(cellKey(toRow, toCol));
    };
    const auto movePitPrizeOrigin = [this, &cellKey](int fromRow, int fromCol,
                                                     int toRow, int toCol, QChar gem) {
        if (gem != '*')
            return;
        const int fromKey = cellKey(fromRow, fromCol);
        auto pitCell = m_pitPrizeDugCellByPosition.find(fromKey);
        if (pitCell == m_pitPrizeDugCellByPosition.end())
            return;
        const int dugCellKey = pitCell.value();
        m_pitPrizeDugCellByPosition.erase(pitCell);
        m_pitPrizeDugCellByPosition.insert(cellKey(toRow, toCol), dugCellKey);
    };
    // Dug tiles are traversable. The player's cell remains an obstacle to object
    // movement; lethal contact is checked only for a falling Crystal with a gap.
    const auto isEmpty = [this](int row, int col) {
        return tileAt(row, col) == '.' && !(row == m_playerY && col == m_playerX);
    };

    for (int row = m_rows - 2; row >= 1; --row) {
        for (int col = 1; col < m_columns - 1; ++col) {
            if (!isGem(row, col))
                continue;
            const QChar fallingGem = tileAt(row, col);
            const int sourceKey = cellKey(row, col);
            const bool wasFalling = fallingGem == 'C'
                    && m_fallingCrystalCells.contains(sourceKey);

            if (wasFalling && col == m_playerX && m_playerY == row + 1) {
                if (!playerMovedSinceLastUpdate) {
                    loseLife();
                    return;
                }
                // Персонаж только что переместился; пока он остаётся внизу,
                // Кристалл сохраняет состояние падения до следующего шага.
                return;
            }

            // A vertical chain falls as one unit when the cell under its bottom gem is free.
            int chainTop = row;
            while (chainTop > 1 && isGem(chainTop - 1, col))
                --chainTop;
            if (chainTop < row && isEmpty(row + 1, col)) {
                if (!playerMovedSinceLastUpdate && fallingGem == 'C' && col == m_playerX
                        && m_playerY - row >= 2) {
                    loseLife();
                    return;
                }
                for (int sourceRow = row; sourceRow >= chainTop; --sourceRow) {
                    const int destinationRow = sourceRow + 1;
                    const QChar shiftedGem = tileAt(sourceRow, col);
                    moveFallingState(sourceRow, col, destinationRow, col, shiftedGem);
                    movePitPrizeOrigin(sourceRow, col, destinationRow, col, shiftedGem);
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
                if (!playerMovedSinceLastUpdate && fallingGem == 'C' && col == m_playerX
                        && m_playerY - row >= 2) {
                    loseLife();
                    return;
                }
                setTileAt(row, col, '.');
                setTileAt(row + 1, col, fallingGem);
                moveFallingState(row, col, row + 1, col, fallingGem);
                movePitPrizeOrigin(row, col, row + 1, col, fallingGem);
                m_tilesModel.updateTile(row, col, "empty");
                m_tilesModel.updateTile(row + 1, col, gemType(fallingGem));
                return;
            }

            if (!isGem(row + 1, col))
            {
                m_fallingCrystalCells.remove(sourceKey);
                continue;
            }

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
            if (tumbleDirection == 0) {
                m_fallingCrystalCells.remove(sourceKey);
                continue;
            }

            const QChar upperGem = tileAt(row, col);
            const QChar tumblingGem = tileAt(row + 1, col);
            movePitPrizeOrigin(row + 1, col, row + 1, col + tumbleDirection, tumblingGem);
            movePitPrizeOrigin(row, col, row + 1, col, upperGem);
            m_fallingCrystalCells.remove(cellKey(row, col));
            m_fallingCrystalCells.remove(cellKey(row + 1, col));
            if (upperGem == 'C')
                m_fallingCrystalCells.insert(cellKey(row + 1, col));
            if (tumblingGem == 'C')
                m_fallingCrystalCells.insert(cellKey(row + 1, col + tumbleDirection));
            setTileAt(row, col, '.');
            setTileAt(row + 1, col + tumbleDirection, tumblingGem);
            setTileAt(row + 1, col, upperGem);
            m_tilesModel.updateTile(row, col, "empty");
            m_tilesModel.updateTile(row + 1, col + tumbleDirection, gemType(tumblingGem));
            m_tilesModel.updateTile(row + 1, col, gemType(upperGem));

            for (int sourceRow = row - 1; sourceRow >= chainTop; --sourceRow) {
                const int destinationRow = sourceRow + 1;
                const QChar shiftedGem = tileAt(sourceRow, col);
                moveFallingState(sourceRow, col, destinationRow, col, shiftedGem);
                movePitPrizeOrigin(sourceRow, col, destinationRow, col, shiftedGem);
                setTileAt(destinationRow, col, shiftedGem);
                setTileAt(sourceRow, col, '.');
                m_tilesModel.updateTile(destinationRow, col, gemType(shiftedGem));
                m_tilesModel.updateTile(sourceRow, col, "empty");
            }
            return;
        }
    }
}

void GameEngine::loseLife()
{
    if (m_gameOver)
        return;

    --m_lives;
    m_gameOver = true;
    m_playerState = "idle";
    emit scoreChanged();
    emit playerStateChanged();
    emit statusChanged();
}

void GameEngine::updateGame()
{
    if (m_paused || m_gameOver || m_levelComplete || hasActiveNotice()) return;

    // Простая гравитация игрока: если под ним пустота — падаем.
    QChar below = tileAt(m_playerY + 1, m_playerX);
    bool supported = (below == '#' || below == 'H' || below == '-');
    QChar current = tileAt(m_playerY, m_playerX);
    bool onClimbable = (current == 'H' || current == '-');

    if (!supported && !onClimbable && m_playerState != "climb") {
        // Падение
        if (playerCanEnter(m_playerY + 1, m_playerX)) {
            ++m_playerY;
            m_playerMovedSinceCrystalUpdate = true;
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
            loseLife();
            return;
        }
    }
    if (changed) m_enemiesModel.refresh();
}
