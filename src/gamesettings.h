#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include <QObject>
#include <QSettings>

class GameSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString tileTheme READ tileTheme WRITE setTileTheme NOTIFY tileThemeChanged)
    Q_PROPERTY(QString playerVariant READ playerVariant WRITE setPlayerVariant NOTIFY playerVariantChanged)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY animationsEnabledChanged)
    Q_PROPERTY(int animationInterval READ animationInterval WRITE setAnimationInterval NOTIFY animationIntervalChanged)
public:
    explicit GameSettings(QObject *parent = nullptr);
    explicit GameSettings(const QString &fileName, QObject *parent = nullptr);
    QString tileTheme() const { return m_tileTheme; }
    QString playerVariant() const { return m_playerVariant; }
    bool animationsEnabled() const { return m_animationsEnabled; }
    int animationInterval() const { return m_animationInterval; }
    void setTileTheme(const QString &value);
    void setPlayerVariant(const QString &value);
    void setAnimationsEnabled(bool value);
    void setAnimationInterval(int value);
    Q_INVOKABLE void resetDefaults();
signals:
    void tileThemeChanged();
    void playerVariantChanged();
    void animationsEnabledChanged();
    void animationIntervalChanged();
private:
    void load();
    void save(const QString &key, const QVariant &value);
    QSettings m_store;
    QString m_tileTheme = QStringLiteral("classic");
    QString m_playerVariant = QStringLiteral("classic");
    bool m_animationsEnabled = true;
    int m_animationInterval = 140;
};
#endif
