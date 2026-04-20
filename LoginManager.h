#pragma once

#include <QObject>
#include <QString>

#include "SDKManager.h"

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);

    // QML 中直接调用：LoginManager.login(...)
    Q_INVOKABLE bool login(const QString &ukeyType,
                           const QString &password,
                           const QString &gbId,
                           const QString &serverIp,
                           const QString &serverPort);
    Q_INVOKABLE QString lastError() const;

private:
    SDKManager& m_sdkManager;
    QString m_lastError;
};

