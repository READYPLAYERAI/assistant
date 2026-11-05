#pragma once

#include <QSettings>
#include <QString>

class Settings {
public:
    static Settings &instance();

    QString apiKey() const;
    void setApiKey(const QString &key);

    QString model() const;
    void setModel(const QString &model);

    bool rememberKey() const;
    void setRememberKey(bool remember);

    void save();

private:
    Settings();
    Settings(const Settings &) = delete;
    Settings &operator=(const Settings &) = delete;

    QSettings m_settings;
    QString m_sessionKey;
};
