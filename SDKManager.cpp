#include "SDKManager.h"

#include <QByteArray>

SDKManager::SDKManager(QObject *parent)
    : QObject(parent)
{
}

SDKManager::~SDKManager()
{
    unloadLibrary();
}

SDKManager& SDKManager::instance()
{
    static SDKManager s_instance;
    return s_instance;
}

bool SDKManager::loadLibrary(const QString &dllPath)
{
    if (m_library.isLoaded())
        return true;

    m_library.setFileName(dllPath);
    if (!m_library.load()) {
        m_lastError = m_library.errorString();
        return false;
    }

    if (!resolveFunctions()) {
        unloadLibrary();
        return false;
    }
    return true;
}

void SDKManager::unloadLibrary()
{
    clearFunctionPointers();
    if (m_library.isLoaded())
        m_library.unload();
}

bool SDKManager::isLibraryLoaded() const
{
    return m_library.isLoaded();
}

unsigned long SDKManager::init(const QString &pin)
{
    if (!m_fnInit) {
        m_lastError = QStringLiteral("TRI_SUT_Init not resolved");
        return 1;
    }
    QByteArray pinBytes = pin.toLocal8Bit();
    return m_fnInit(pinBytes.data());
}

unsigned long SDKManager::setConfigParameter(const QString &key, const QString &value)
{
    if (!m_fnSetConfig) {
        m_lastError = QStringLiteral("TRI_SUT_SetConfigParameter not resolved");
        return 1;
    }
    QByteArray keyBytes = key.toLocal8Bit();
    QByteArray valueBytes = value.toLocal8Bit();
    return m_fnSetConfig(keyBytes.constData(), valueBytes.data());
}

unsigned long SDKManager::login(QString &userId, QString &token)
{
    userId.clear();
    token.clear();

    if (!m_fnLogin) {
        m_lastError = QStringLiteral("TRI_SUT_Login not resolved");
        return 1;
    }

    char *pUserId = nullptr;
    ULONG idLength = 0;
    char *pToken = nullptr;
    ULONG tokenLength = 0;

    const unsigned long ret = m_fnLogin(&pUserId, &idLength, &pToken, &tokenLength);
    if (ret != 0)
        return ret;

    userId = fromApiBuffer(pUserId, idLength);
    token = fromApiBuffer(pToken, tokenLength);
    return ret;
}

unsigned long SDKManager::getUKeyStatus(unsigned int &status)
{
    if (!m_fnGetUKeyStatus) {
        m_lastError = QStringLiteral("TRI_SUT_GetUKeyStatus not resolved");
        return 1;
    }
    UINT sdkStatus = 0;
    const unsigned long ret = m_fnGetUKeyStatus(&sdkStatus);
    status = static_cast<unsigned int>(sdkStatus);
    return ret;
}

unsigned long SDKManager::cleanup()
{
    if (!m_fnCleanup) {
        m_lastError = QStringLiteral("TRI_SUT_Cleanup not resolved");
        return 1;
    }
    return m_fnCleanup();
}

unsigned long SDKManager::getStreamSecDecode(unsigned long &decodeHandle)
{
    if (!m_fnGetStreamSecDecode) {
        m_lastError = QStringLiteral("TRI_SUT_GetStreamSecDecode not resolved");
        return 1;
    }
    ULONG handle = 0;
    const unsigned long ret = m_fnGetStreamSecDecode(&handle);
    decodeHandle = static_cast<unsigned long>(handle);
    return ret;
}

unsigned long SDKManager::getRealStreamSecDecode(unsigned long &decodeHandle, const QString &deviceId)
{
    if (!m_fnGetRealStreamSecDecode) {
        m_lastError = QStringLiteral("TRI_SUT_GetRealStreamSecDecode not resolved");
        return 1;
    }
    ULONG handle = decodeHandle;
    QByteArray deviceIdBytes = deviceId.toLocal8Bit();
    const unsigned long ret = m_fnGetRealStreamSecDecode(&handle, deviceIdBytes.data());
    decodeHandle = static_cast<unsigned long>(handle);
    return ret;
}

unsigned long SDKManager::videoDataSecDecodeExt(unsigned long &decodeHandle,
                                                IMG_FRAME_UNIT *inFrame,
                                                IMG_FRAME_UNIT **outFrame,
                                                bool isSignVerify)
{
    if (!m_fnVideoDataSecDecodeExt) {
        m_lastError = QStringLiteral("TRI_SUT_VideoDataSecDecodeExt not resolved");
        return 1;
    }
    if (!inFrame || !outFrame) {
        m_lastError = QStringLiteral("invalid decode input/output frame pointer");
        return 1;
    }

    ULONG handle = decodeHandle;
    const unsigned long ret = m_fnVideoDataSecDecodeExt(
            &handle,
            inFrame,
            outFrame,
            isSignVerify ? TRUE : FALSE);
    decodeHandle = static_cast<unsigned long>(handle);
    return ret;
}

unsigned long SDKManager::freeFrame(IMG_FRAME_UNIT **outFrame)
{
    if (!m_fnFree) {
        m_lastError = QStringLiteral("TRI_SUT_Free not resolved");
        return 1;
    }
    if (!outFrame) {
        m_lastError = QStringLiteral("invalid outFrame pointer");
        return 1;
    }
    return m_fnFree(outFrame);
}

unsigned long SDKManager::closeHandle(unsigned long handle)
{
    if (!m_fnClose) {
        m_lastError = QStringLiteral("TRI_SUT_Close not resolved");
        return 1;
    }
    return m_fnClose(static_cast<ULONG>(handle));
}

bool SDKManager::resolveFunctions()
{
    m_fnInit = reinterpret_cast<FnInit>(m_library.resolve("TRI_SUT_Init"));
    m_fnSetConfig = reinterpret_cast<FnSetConfig>(m_library.resolve("TRI_SUT_SetConfigParameter"));
    m_fnLogin = reinterpret_cast<FnLogin>(m_library.resolve("TRI_SUT_Login"));
    m_fnGetUKeyStatus = reinterpret_cast<FnGetUKeyStatus>(m_library.resolve("TRI_SUT_GetUKeyStatus"));
    m_fnCleanup = reinterpret_cast<FnCleanup>(m_library.resolve("TRI_SUT_Cleanup"));
    m_fnGetStreamSecDecode = reinterpret_cast<FnGetStreamSecDecode>(m_library.resolve("TRI_SUT_GetStreamSecDecode"));
    m_fnGetRealStreamSecDecode = reinterpret_cast<FnGetRealStreamSecDecode>(m_library.resolve("TRI_SUT_GetRealStreamSecDecode"));
    m_fnVideoDataSecDecodeExt = reinterpret_cast<FnVideoDataSecDecodeExt>(m_library.resolve("TRI_SUT_VideoDataSecDecodeExt"));
    m_fnFree = reinterpret_cast<FnFree>(m_library.resolve("TRI_SUT_Free"));
    m_fnClose = reinterpret_cast<FnClose>(m_library.resolve("TRI_SUT_Close"));

    if (!m_fnInit || !m_fnSetConfig || !m_fnLogin || !m_fnGetUKeyStatus || !m_fnCleanup
            || !m_fnGetStreamSecDecode || !m_fnGetRealStreamSecDecode
            || !m_fnVideoDataSecDecodeExt || !m_fnFree || !m_fnClose) {
        m_lastError = QStringLiteral("Failed to resolve required p35114_sdk.dll exports");
        return false;
    }
    return true;
}

void SDKManager::clearFunctionPointers()
{
    m_fnInit = nullptr;
    m_fnSetConfig = nullptr;
    m_fnLogin = nullptr;
    m_fnGetUKeyStatus = nullptr;
    m_fnCleanup = nullptr;
    m_fnGetStreamSecDecode = nullptr;
    m_fnGetRealStreamSecDecode = nullptr;
    m_fnVideoDataSecDecodeExt = nullptr;
    m_fnFree = nullptr;
    m_fnClose = nullptr;
}

QString SDKManager::fromApiBuffer(char *buf, ULONG len)
{
    if (!buf || len == 0)
        return {};
    QByteArray bytes(buf, static_cast<int>(len));
    // 去掉 API 可能带回的 '\0'
    while (!bytes.isEmpty() && bytes.endsWith('\0'))
        bytes.chop(1);
    return QString::fromLocal8Bit(bytes);
}

