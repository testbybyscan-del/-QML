#include "gamesettings.h"
#include <QStringList>

namespace {
bool validTheme(const QString &value)
{
    return QStringList{"classic", "kenney", "industrial"}.contains(value);
}
bool validPlayer(const QString &value)
{
    return QStringList{"classic", "astronaut", "pink", "robot"}.contains(value);
}
}

GameSettings::GameSettings(QObject *parent) : QObject(parent) { load(); }
GameSettings::GameSettings(const QString &fileName, QObject *parent)
    : QObject(parent), m_store(fileName, QSettings::IniFormat) { load(); }

void GameSettings::load()
{
    m_store.beginGroup(QStringLiteral("appearance"));
    const auto theme = m_store.value("tileTheme", "classic").toString();
    const auto player = m_store.value("playerVariant", "classic").toString();
    if (validTheme(theme)) m_tileTheme = theme;
    if (validPlayer(player)) m_playerVariant = player;
    const auto animations = m_store.value("animationsEnabled", true).toString();
    if (animations == "true" || animations == "false") m_animationsEnabled = animations == "true";
    bool ok = false;
    const int interval = m_store.value("animationInterval", 140).toInt(&ok);
    if (ok && interval >= 60 && interval <= 500) m_animationInterval = interval;
}

void GameSettings::save(const QString &key, const QVariant &value)
{
    m_store.setValue(key, value);
    m_store.sync();
}
void GameSettings::setTileTheme(const QString &value)
{
    if (!validTheme(value) || value == m_tileTheme) return;
    m_tileTheme = value;
    save("tileTheme", value);
    emit tileThemeChanged();
}
void GameSettings::setPlayerVariant(const QString &value)
{
    if (!validPlayer(value) || value == m_playerVariant) return;
    m_playerVariant = value;
    save("playerVariant", value);
    emit playerVariantChanged();
}
void GameSettings::setAnimationsEnabled(bool value)
{
    if (value == m_animationsEnabled) return;
    m_animationsEnabled = value;
    save("animationsEnabled", value);
    emit animationsEnabledChanged();
}
void GameSettings::setAnimationInterval(int value)
{
    if (value < 60 || value > 500 || value == m_animationInterval) return;
    m_animationInterval = value;
    save("animationInterval", value);
    emit animationIntervalChanged();
}
void GameSettings::resetDefaults()
{
    setTileTheme("classic");
    setPlayerVariant("classic");
    setAnimationsEnabled(true);
    setAnimationInterval(140);
    // Исправить также некорректные значения, прочитанные с диска как значения по умолчанию.
    save("tileTheme", m_tileTheme);
    save("playerVariant", m_playerVariant);
    save("animationsEnabled", m_animationsEnabled);
    save("animationInterval", m_animationInterval);
}
