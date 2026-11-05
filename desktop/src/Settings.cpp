#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

QString defaultModel()
{
    return QStringLiteral("deepseek/deepseek-flash-latest");
}

QString configDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir;
}

} // namespace

Settings &Settings::instance()
{
    static Settings s;
    return s;
}

Settings::Settings()
    : m_settings(configDir() + QStringLiteral("/config.ini"), QSettings::IniFormat)
{
}

QString Settings::apiKey() const
{
    if (rememberKey()) {
        const QString stored = m_settings.value(QStringLiteral("api/key")).toString();
        if (!stored.isEmpty())
            return stored;
    }
    return m_sessionKey;
}

void Settings::setApiKey(const QString &key)
{
    m_sessionKey = key;
}

QString Settings::model() const
{
    const QString v = m_settings.value(QStringLiteral("api/model")).toString().trimmed();
    return v.isEmpty() ? defaultModel() : v;
}

void Settings::setModel(const QString &model)
{
    m_settings.setValue(QStringLiteral("api/model"), model.trimmed());
}

bool Settings::rememberKey() const
{
    return m_settings.value(QStringLiteral("api/rememberKey"), false).toBool();
}

void Settings::setRememberKey(bool remember)
{
    m_settings.setValue(QStringLiteral("api/rememberKey"), remember);
}

void Settings::save()
{
    m_settings.setValue(QStringLiteral("api/model"), model());
    m_settings.setValue(QStringLiteral("api/rememberKey"), rememberKey());
    if (rememberKey() && !m_sessionKey.isEmpty())
        m_settings.setValue(QStringLiteral("api/key"), m_sessionKey);
    if (!rememberKey())
        m_settings.remove(QStringLiteral("api/key"));
    m_settings.sync();

    const QString path = m_settings.fileName();
    if (QFileInfo::exists(path)) {
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    }
}
