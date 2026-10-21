#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QTimer>
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

    Q_PROPERTY(int columns READ columns NOTIFY levelChanged)
    Q_PROPERTY(int rows READ rows NOTIFY levelChanged)
    Q_PROPERTY(int currentLevel READ currentLevel NOTIFY levelChanged)
    Q_PROPERTY(int playerX READ playerX NOTIFY playerPositionChanged)
    Q_PROPERTY(int playerY READ playerY NOTIFY playerPositionChanged)
    Q_PROPERTY(QString playerState READ playerState NOTIFY playerStateChanged)
    Q_PROPERTY(bool playerFacingRight READ playerFacingRight NOTIFY playerStateChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int prizesLeft READ prizesLeft NOTIFY scoreChanged)
    Q_PROPERTY(int lives READ lives NOTIFY scoreChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY statusChanged)
    Q_PROPERTY(bool levelComplete READ levelComplete NOTIFY statusChanged)
    Q_PROPERTY(TileModel* tilesModel READ tilesModel CONSTANT)
    Q_PROPERTY(EnemyModel* enemiesModel READ enemiesModel CONSTANT)

public:
    explicit GameEngine(QObject *parent = nullptr);

    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    int currentLevel() const { return m_currentLevel; }
    int playerX() const { return m_playerX; }
    int playerY() const { return m_playerY; }
    QString playerState() const { return m_playerState; }
    bool playerFacingRight() const { return m_playerFacingRight; }
    int score() const { return m_score; }
    int prizesLeft() const { return m_prizesLeft; }
    int lives() const { return m_lives; }
    bool gameOver() const { return m_gameOver; }
    bool levelComplete() const { return m_levelComplete; }

    TileModel* tilesModel() { return &m_tilesModel; }
    EnemyModel* enemiesModel() { return &m_enemiesModel; }

    Q_INVOKABLE void movePlayer(int dx, int dy);
    Q_INVOKABLE void dig();
    Q_INVOKABLE void updateGame();
    Q_INVOKABLE void updateCrystals();

signals:
    void playerPositionChanged();
    void playerStateChanged();
    void scoreChanged();
    void statusChanged();
    void levelChanged();

private:
    struct LevelSnapshot {
        QVector<QString> grid;
        int playerX = 0;
        int playerY = 0;
        int prizesLeft = 0;
        bool levelComplete = false;
        bool valid = false;
    };

    void loadLevel();
    void saveCurrentLevel();
    void returnToFirstLevel();
    void returnToSecondLevel();
    QChar tileAt(int row, int col) const;
    void setTileAt(int row, int col, QChar ch);
    bool playerCanEnter(int row, int col) const;

    int m_columns = 0;
    int m_rows = 0;
    int m_currentLevel = 1;
    int m_playerX = 0;
    int m_playerY = 0;
    QString m_playerState = "idle";
    bool m_playerFacingRight = true;
    int m_score = 0;
    int m_prizesLeft = 0;
    int m_lives = 3;
    bool m_gameOver = false;
    bool m_levelComplete = false;

    QVector<QString> m_grid; // каждый QString — строка уровня
    LevelSnapshot m_level1Snapshot;
    LevelSnapshot m_level2Snapshot;

    QHash<int, QTimer*> m_dugTileTimers;

    TileModel m_tilesModel;
    EnemyModel m_enemiesModel;
};

#endif // GAMEENGINE_H
