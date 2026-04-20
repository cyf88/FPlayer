#include "LoginManager.h"

#include "tri_core_api.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
    , m_sdkManager(SDKManager::instance())
{
}

bool LoginManager::login(const QString &ukeyType,
                         const QString &password,
                         const QString &gbId,
                         const QString &serverIp,
                         const QString &serverPort)
{
    m_lastError.clear();
    // 约定：passwordField 即 SDK 的 PIN
    // 登录成功：返回 true；失败：返回 false（由 QML 弹出提示）

    Q_UNUSED(ukeyType); // 如果后续你要根据国芯/渔翁映射配置项，再在这里补

    const QString dllName = QStringLiteral("p35114_sdk.dll");
    const QString appDirDllPath = QDir(QCoreApplication::applicationDirPath()).filePath(dllName);
    const QString dllPath = QFileInfo::exists(appDirDllPath) ? appDirDllPath : dllName;

    if (!m_sdkManager.loadLibrary(dllPath)) {
        m_lastError = QStringLiteral("加载 %1 失败（尝试路径: %2）: %3")
                          .arg(dllName, dllPath, m_sdkManager.lastError());
        return false;
    }

    const unsigned long initRet = m_sdkManager.init(password);
    if (initRet != 0) {
        m_lastError = QStringLiteral("SDK 初始化失败, code=%1").arg(initRet);
        return false;
    }

    // 关键配置：服务器 IP/端口（按 tri_core_api.h 的参数名）
    const unsigned long ipRet =
        m_sdkManager.setConfigParameter(QStringLiteral(TRI_PARAM_SERVER_IP), serverIp);
    if (ipRet != 0) {
        m_lastError = QStringLiteral("设置 SIPServerIP 失败, code=%1").arg(ipRet);
        return false;
    }

    const unsigned long portRet =
        m_sdkManager.setConfigParameter(QStringLiteral(TRI_PARAM_SERVER_PORT), serverPort);
    if (portRet != 0) {
        m_lastError = QStringLiteral("设置 SIPServerPort 失败, code=%1").arg(portRet);
        return false;
    }

    // server 国标ID
    const unsigned long gbIdRet =
        m_sdkManager.setConfigParameter(QStringLiteral(TRI_PARAM_GB_Code), gbId);
    if (gbIdRet != 0) {
        m_lastError = QStringLiteral("设置 server id 失败, code=%1").arg(gbIdRet);
        return false;
    }

    // KeepAliveTime（示例固定为 60）
    (void)m_sdkManager.setConfigParameter(QStringLiteral(TRI_PARAM_KAT), QStringLiteral("60"));

    QString userId;
    QString token;
    const unsigned long loginRet = m_sdkManager.login(userId, token);
    if (loginRet != 0) {
        m_lastError = QStringLiteral("SDK 登录失败, code=%1").arg(loginRet);
        return false;
    }
    return true;
}

QString LoginManager::lastError() const
{
    return m_lastError;
}

