#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <QQmlEngine>

// ---------- Модель тайлов ----------
class TileModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum Roles { RowRole = Qt::UserRole + 1, ColRole, TypeRole };
    struct Tile { int row; int col; QString type; };

    explicit TileModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_tiles.size();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_tiles.size()) return {};
        const auto &t = m_tiles[index.row()];
        switch (role) {
        case RowRole:  return t.row;
        case ColRole:  return t.col;
        case TypeRole: return t.type;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {RowRole, "row"}, {ColRole, "col"}, {TypeRole, "type"} };
    }
    void setTiles(const QVector<Tile> &tiles) {
        beginResetModel();
        m_tiles = tiles;
        endResetModel();
    }
    void updateTile(int row, int col, const QString &type) {
        for (int i = 0; i < m_tiles.size(); ++i) {
            if (m_tiles[i].row == row && m_tiles[i].col == col) {
                m_tiles[i].type = type;
                emit dataChanged(index(i), index(i), {TypeRole});
                return;
            }
        }
    }
private:
    QVector<Tile> m_tiles;
};

// ---------- Модель врагов ----------
class EnemyModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum Roles { XRole = Qt::UserRole + 1, YRole, StateRole, FacingRightRole };
    struct Enemy { int x; int y; QString state; bool facingRight; };

    explicit EnemyModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_enemies.size();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_enemies.size()) return {};
        const auto &e = m_enemies[index.row()];
        switch (role) {
        case XRole:           return e.x;
        case YRole:           return e.y;
        case StateRole:       return e.state;
        case FacingRightRole: return e.facingRight;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {XRole, "x"}, {YRole, "y"}, {StateRole, "state"}, {FacingRightRole, "facingRight"} };
    }
    void setEnemies(const QVector<Enemy> &enemies) {
        beginResetModel();
        m_enemies = enemies;
        endResetModel();
    }
    QVector<Enemy> &enemies() { return m_enemies; }
    void refresh() {
        if (!m_enemies.isEmpty())
            emit dataChanged(index(0), index(m_enemies.size() - 1));
    }
private:
    QVector<Enemy> m_enemies;
};

// ---------- Игровой движок ----------
class GameEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int columns READ columns NOTIFY levelChanged)
    Q_PROPERTY(int rows READ rows NOTIFY levelChanged)
    Q_PROPERTY(int currentLevel READ currentLevel NOTIFY levelChanged)
    Q_PROPERTY(int authoredLevelCount READ authoredLevelCount NOTIFY levelChanged)
    Q_PROPERTY(int playerX READ playerX NOTIFY playerPositionChanged)
    Q_PROPERTY(int playerY READ playerY NOTIFY playerPositionChanged)
    Q_PROPERTY(QString playerState READ playerState NOTIFY playerStateChanged)
    Q_PROPERTY(bool playerFacingRight READ playerFacingRight NOTIFY playerStateChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int highScore READ highScore NOTIFY scoreChanged)
    Q_PROPERTY(int prizesLeft READ prizesLeft NOTIFY scoreChanged)
    Q_PROPERTY(int lives READ lives NOTIFY scoreChanged)
    Q_PROPERTY(bool hasKey READ hasKey NOTIFY hasKeyChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY statusChanged)
    Q_PROPERTY(bool levelComplete READ levelComplete NOTIFY statusChanged)
    Q_PROPERTY(bool keyRequiredNotice READ keyRequiredNotice NOTIFY statusChanged)
    Q_PROPERTY(bool keyCollectedNotice READ keyCollectedNotice NOTIFY statusChanged)
    Q_PROPERTY(bool safeOpenedNotice READ safeOpenedNotice NOTIFY statusChanged)
    Q_PROPERTY(TileModel* tilesModel READ tilesModel CONSTANT)
    Q_PROPERTY(EnemyModel* enemiesModel READ enemiesModel CONSTANT)

public:
    explicit GameEngine(QObject *parent = nullptr);

    bool paused() const { return m_paused; }
    void setPaused(bool paused);

    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    int currentLevel() const { return m_currentLevel; }
    int authoredLevelCount() const { return m_authoredLevelCount; }
    int playerX() const { return m_playerX; }
    int playerY() const { return m_playerY; }
    QString playerState() const { return m_playerState; }
    bool playerFacingRight() const { return m_playerFacingRight; }
    int score() const { return m_score; }
    int highScore() const { return m_highScore; }
    int prizesLeft() const { return m_prizesLeft; }
    int lives() const { return m_lives; }
    bool hasKey() const { return m_hasKey; }
    bool gameOver() const { return m_gameOver; }
    bool levelComplete() const { return m_levelComplete; }
    bool keyRequiredNotice() const { return m_keyRequiredNotice; }
    bool keyCollectedNotice() const { return m_keyCollectedNotice; }
    bool safeOpenedNotice() const { return m_safeOpenedNotice; }

    TileModel* tilesModel() { return &m_tilesModel; }
    EnemyModel* enemiesModel() { return &m_enemiesModel; }

    Q_INVOKABLE void movePlayer(int dx, int dy);
    Q_INVOKABLE void dig();
    Q_INVOKABLE void updateGame();
    Q_INVOKABLE void updateCrystals();
    Q_INVOKABLE void initializeLevels(const QVariantList &authoredLevels);
    Q_INVOKABLE void restartGame();
    Q_INVOKABLE void dismissKeyRequiredNotice();
    Q_INVOKABLE void dismissKeyCollectedNotice();
    Q_INVOKABLE void dismissSafeOpenedNotice();

signals:
    void pausedChanged();
    void playerPositionChanged();
    void playerStateChanged();
    void scoreChanged();
    void hasKeyChanged();
    void statusChanged();
    void levelChanged();

private:
    struct LevelSnapshot {
        QVector<QString> grid;
        int playerX = 0;
        int playerY = 0;
        QHash<int, int> pitPrizeDugCellByPosition;
        bool valid = false;
    };

    void loadLevel();
    void startDugTileTimer(int row, int col);
    void addScore(int points);
    void loseLife();
    bool hasActiveNotice() const;
    bool isAdjacentToLockedSafe(int row, int col) const;
    void generateNextLandscape();
    void saveCurrentLevel();
    void switchToLevel(int targetLevel);
    QChar tileAt(int row, int col) const;
    void setTileAt(int row, int col, QChar ch);
    bool playerCanEnter(int row, int col) const;

    int m_columns = 0;
    int m_rows = 0;
    int m_currentLevel = 1;
    int m_authoredLevelCount = 0;
    int m_landscapeSeed = 0;
    int m_playerX = 0;
    int m_playerY = 0;
    QString m_playerState = "idle";
    bool m_playerFacingRight = true;
    int m_score = 0;
    int m_highScore = 0;
    int m_prizesLeft = 0;
    int m_lives = 3;
    bool m_hasKey = false;
    bool m_gameOver = false;
    bool m_levelComplete = false;
    bool m_keyRequiredNotice = false;
    bool m_keyCollectedNotice = false;
    bool m_safeOpenedNotice = false;
    bool m_keyNoticeShownForVisit = false;
    bool m_nearLockedSafe = false;
    bool m_playerMovedSinceCrystalUpdate = false;
    QSet<int> m_fallingCrystalCells;
    QHash<int, int> m_pitPrizeDugCellByPosition;

    QVector<QString> m_grid; // каждый QString — строка уровня
    QVector<QVector<QString>> m_levels;
    QVector<LevelSnapshot> m_levelSnapshots;

    bool m_paused = false;
    QHash<int, int> m_pausedDugRemaining;
    QHash<int, QTimer*> m_dugTileTimers;

    TileModel m_tilesModel;
    EnemyModel m_enemiesModel;
};

#endif // GAMEENGINE_H
