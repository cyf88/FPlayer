#pragma once

#include <QObject>
#include <QLibrary>
#include <QString>

#include "tri_core_api.h"

class SDKManager : public QObject
{
    Q_OBJECT
public:
    explicit SDKManager(QObject *parent = nullptr);
    ~SDKManager() override;
    static SDKManager& instance();

    bool loadLibrary(const QString &dllPath = QStringLiteral("p35114_sdk.dll"));
    void unloadLibrary();
    bool isLibraryLoaded() const;
    bool isInitialized() const;

    unsigned long init(const QString &pin);
    unsigned long setConfigParameter(const QString &key, const QString &value);
    unsigned long login(QString &userId, QString &token);
    unsigned long getUKeyStatus(unsigned int &status);
    unsigned long cleanup();
    unsigned long getStreamSecDecode(unsigned long &decodeHandle);
    unsigned long getRealStreamSecDecode(unsigned long &decodeHandle, const QString &deviceId);
    unsigned long videoDataSecDecodeExt(unsigned long &decodeHandle,
                                        IMG_FRAME_UNIT *inFrame,
                                        IMG_FRAME_UNIT **outFrame,
                                        bool isSignVerify);
    unsigned long freeFrame(IMG_FRAME_UNIT **outFrame);
    unsigned long closeHandle(unsigned long handle);

    QString lastError() const { return m_lastError; }

private:
    // Reuse exact API signatures (including calling convention) from tri_core_api.h.
    using FnInit = decltype(&TRI_SUT_Init);
    using FnSetConfig = decltype(&TRI_SUT_SetConfigParameter);
    using FnLogin = decltype(&TRI_SUT_Login);
    using FnGetUKeyStatus = decltype(&TRI_SUT_GetUKeyStatus);
    using FnCleanup = decltype(&TRI_SUT_Cleanup);
    using FnGetStreamSecDecode = decltype(&TRI_SUT_GetStreamSecDecode);
    using FnGetRealStreamSecDecode = decltype(&TRI_SUT_GetRealStreamSecDecode);
    using FnVideoDataSecDecodeExt = decltype(&TRI_SUT_VideoDataSecDecodeExt);
    using FnFree = decltype(&TRI_SUT_Free);
    using FnClose = decltype(&TRI_SUT_Close);

    bool resolveFunctions();
    void clearFunctionPointers();
    static QString fromApiBuffer(char *buf, ULONG len);

private:
    QLibrary m_library;
    QString m_lastError;
    bool m_initialized = false;

    FnInit m_fnInit = nullptr;
    FnSetConfig m_fnSetConfig = nullptr;
    FnLogin m_fnLogin = nullptr;
    FnGetUKeyStatus m_fnGetUKeyStatus = nullptr;
    FnCleanup m_fnCleanup = nullptr;
    FnGetStreamSecDecode m_fnGetStreamSecDecode = nullptr;
    FnGetRealStreamSecDecode m_fnGetRealStreamSecDecode = nullptr;
    FnVideoDataSecDecodeExt m_fnVideoDataSecDecodeExt = nullptr;
    FnFree m_fnFree = nullptr;
    FnClose m_fnClose = nullptr;
};

