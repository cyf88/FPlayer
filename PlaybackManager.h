#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QtGlobal>
#include <atomic>

#include "PsParser.h"
#include "SDKManager.h"
#include "SVACDecoder.h"

class PlaybackManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qulonglong frameSerial READ frameSerial NOTIFY frameSerialChanged)
public:
    explicit PlaybackManager(QObject *parent = nullptr);

    Q_INVOKABLE bool playPsFile(const QUrl &fileUrl);
    Q_INVOKABLE void setSdkPin(const QString &pin);
    Q_INVOKABLE void setCoreDllPath(const QString &path);
    Q_INVOKABLE void setSvacDllPath(const QString &path);
    qulonglong frameSerial() const;

signals:
    void playStarted(const QString &filePath);
    void playFinished(int decodedFrameCount);
    void playFailed(const QString &reason);
    void frameSerialChanged();

private:
    bool ensureReady(QString &reason);

private:
    SDKManager& m_sdkManager;
    SVACDecoder m_svacDecoder;
    QString m_sdkPin;
    QString m_coreDllPath;
    QString m_svacDllPath;
    bool m_sdkInited = false;
    int m_decodedFrameCount = 0;
    qulonglong m_frameSerial = 0;
    /// Ps 包内若先出现 pack header(BA)，frametype 常被标成 P，导致 SVAC 从未 open。首帧出画面前强制按关键帧打开。
    bool m_svacGotPicture = false;
    std::atomic_bool m_isPlaying{false};
};

