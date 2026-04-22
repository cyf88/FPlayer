#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QtGlobal>
#include <atomic>
#include <array>
#include <memory>
#include <mutex>

#include "PsParser.h"
#include "SDKManager.h"
#include "SVACDecoder.h"
#include "RTSPClientWrapper.h"

class PlaybackManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qulonglong frameSerial READ frameSerial NOTIFY frameSerialChanged)
public:
    explicit PlaybackManager(QObject *parent = nullptr);
    ~PlaybackManager() override;

    Q_INVOKABLE bool playPsFile(const QUrl &fileUrl);
    Q_INVOKABLE bool playRtspUrl(const QString &rtspUrl);
    Q_INVOKABLE bool playRtspInSlot(int slot, const QString &rtspUrl);
    Q_INVOKABLE void stopRtsp();
    Q_INVOKABLE void stopRtspInSlot(int slot);
    Q_INVOKABLE void setSdkPin(const QString &pin);
    Q_INVOKABLE void setCoreDllPath(const QString &path);
    Q_INVOKABLE void setSvacDllPath(const QString &path);
    qulonglong frameSerial() const;
    Q_INVOKABLE qulonglong frameSerialForSlot(int slot) const;

signals:
    void playStarted(const QString &filePath);
    void playFinished(int decodedFrameCount);
    void playFailed(const QString &reason);
    void frameSerialChanged();

private:
    bool ensureReady(QString &reason);
    bool startRtspSlotLocked(int slot, const QString &rtspUrl, QString &reason);
    void stopRtspSlotLocked(int slot);
    static constexpr int kMaxRtspSlots = 9;

private:
    SDKManager& m_sdkManager;
    SVACDecoder m_svacDecoder;
    QString m_sdkPin;
    QString m_coreDllPath;
    QString m_svacDllPath;
    bool m_sdkInited = false;
    bool m_svacLibLoaded = false;
    int m_decodedFrameCount = 0;
    qulonglong m_frameSerial = 0;
    /// Ps 包内若先出现 pack header(BA)，frametype 常被标成 P，导致 SVAC 从未 open。首帧出画面前强制按关键帧打开。
    bool m_svacGotPicture = false;
    std::atomic_bool m_isPlaying{false};
    std::atomic_bool m_frameUpdateQueued{false};
    std::mutex m_rtspMutex;

    struct RtspSlotContext {
        rtspwrap::RTSPClientWrapper::Ptr client;
        std::unique_ptr<SVACDecoder> decoder;
        std::atomic_bool playing{false};
        std::atomic_ullong generation{0};
        unsigned long decodeHandle = 0;
        bool svacGotPicture = false;
        qulonglong frameSerial = 0;
        std::atomic_bool frameUpdateQueued{false};
        QString url;
    };
    std::array<RtspSlotContext, kMaxRtspSlots> m_rtspSlots;
};

