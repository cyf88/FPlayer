#include "PlaybackManager.h"
#include "SvacImageProvider.h"

#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QMetaObject>
#include <QtConcurrent>

namespace {
static inline int clampToByte(int value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

QImage yuv420pToRgbImage(const uint8_t *data, int width, int height)
{
    if (!data || width <= 0 || height <= 0)
        return {};

    QImage image(width, height, QImage::Format_RGB32);
    const int ySize = width * height;
    const uint8_t *yPlane = data;
    const uint8_t *uPlane = data + ySize;
    const uint8_t *vPlane = data + ySize + ySize / 4;

    for (int y = 0; y < height; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        const int uvRow = (y / 2) * (width / 2);
        for (int x = 0; x < width; ++x) {
            const int yy = static_cast<int>(yPlane[y * width + x]);
            const int uu = static_cast<int>(uPlane[uvRow + x / 2]) - 128;
            const int vv = static_cast<int>(vPlane[uvRow + x / 2]) - 128;

            const int r = clampToByte(yy + static_cast<int>(1.402 * vv));
            const int g = clampToByte(yy - static_cast<int>(0.344136 * uu + 0.714136 * vv));
            const int b = clampToByte(yy + static_cast<int>(1.772 * uu));
            line[x] = qRgb(r, g, b);
        }
    }

    return image;
}
}

void PlaybackManager::stopRtsp()
{
    std::lock_guard<std::mutex> lock(m_rtspMutex);
    for (int i = 0; i < kMaxRtspSlots; ++i) {
        stopRtspSlotLocked(i);
    }
}

void PlaybackManager::stopRtspInSlot(int slot)
{
    if (slot < 0 || slot >= kMaxRtspSlots)
        return;
    std::lock_guard<std::mutex> lock(m_rtspMutex);
    stopRtspSlotLocked(slot);
}

void PlaybackManager::stopRtspSlotLocked(int slot)
{
    auto &ctx = m_rtspSlots[slot];
    const QString channelId = QStringLiteral("ch%1").arg(slot);
    ctx.playing = false;
    if (ctx.client) {
        ctx.client->stop();
        ctx.client.reset();
    }
    if (ctx.decodeHandle != 0) {
        m_sdkManager.closeHandle(ctx.decodeHandle);
        ctx.decodeHandle = 0;
    }
    ctx.decoder.reset();
    ctx.svacGotPicture = false;
    ctx.frameUpdateQueued = false;
    SvacImageProvider::clearFrame(channelId);
    ++ctx.frameSerial;
    ++m_frameSerial;
    emit frameSerialChanged();
    ctx.url.clear();
}

PlaybackManager::PlaybackManager(QObject *parent)
    : QObject(parent)
    , m_sdkManager(SDKManager::instance())
    , m_svacDecoder(4)
{
    m_coreDllPath = QStringLiteral("p35114_sdk.dll");
    m_svacDllPath = QStringLiteral("ZXSVACDecLib.dll");

    m_svacDecoder.setOnDecode([this](const Frame &frame) {
        if (frame.data.empty() || frame.width <= 0 || frame.height <= 0) {
            ++m_decodedFrameCount;
            return;
        }

        const QImage rgbImage = yuv420pToRgbImage(frame.data.data(), frame.width, frame.height);
        if (!rgbImage.isNull()) {
            m_svacGotPicture = true;
            bool expected = false;
            if (!m_frameUpdateQueued.compare_exchange_strong(expected, true)) {
                ++m_decodedFrameCount;
                return;
            }
            QMetaObject::invokeMethod(this, [this, rgbImage]() {
                SvacImageProvider::setLatestFrame(QStringLiteral("ch0"), rgbImage);
                ++m_frameSerial;
                ++m_rtspSlots[0].frameSerial;
                emit frameSerialChanged();
                m_frameUpdateQueued = false;
            }, Qt::QueuedConnection);
        }
        ++m_decodedFrameCount;
    });
}

PlaybackManager::~PlaybackManager()
{
    stopRtsp();
}

void PlaybackManager::setSdkPin(const QString &pin)
{
    m_sdkPin = pin;
}

void PlaybackManager::setCoreDllPath(const QString &path)
{
    m_coreDllPath = path;
}

void PlaybackManager::setSvacDllPath(const QString &path)
{
    m_svacDllPath = path;
}

qulonglong PlaybackManager::frameSerial() const
{
    return m_frameSerial;
}

qulonglong PlaybackManager::frameSerialForSlot(int slot) const
{
    if (slot < 0 || slot >= kMaxRtspSlots)
        return 0;
    return m_rtspSlots[slot].frameSerial;
}

bool PlaybackManager::ensureReady(QString &reason)
{
    if (!m_sdkManager.isLibraryLoaded()) {
        reason = QStringLiteral("SDK 未登录，请先登录");
        return false;
    }

    if (!m_svacLibLoaded) {
        if (!m_svacDecoder.loadLibrary(m_svacDllPath.toStdString())) {
            reason = QStringLiteral("加载 SVAC 解码库失败: %1").arg(m_svacDllPath);
            return false;
        }
        m_svacLibLoaded = true;
    }

    return true;
}

bool PlaybackManager::playPsFile(const QUrl &fileUrl)
{
    if (m_isPlaying.exchange(true)) {
        emit playFailed(QStringLiteral("当前已有播放任务在执行"));
        return false;
    }

    const QString filePath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (filePath.isEmpty()) {
        m_isPlaying = false;
        emit playFailed(QStringLiteral("无效文件路径"));
        return false;
    }

    QtConcurrent::run([this, filePath]() {
        auto fail = [this](const QString &reason) {
            QMetaObject::invokeMethod(this, [this, reason]() { emit playFailed(reason); }, Qt::QueuedConnection);
            m_isPlaying = false;
        };

        QFile file(filePath);
        if (!file.exists()) {
            fail(QStringLiteral("文件不存在: %1").arg(filePath));
            return;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("文件打开失败: %1").arg(filePath));
            return;
        }

        QString reason;
        if (!ensureReady(reason)) {
            file.close();
            fail(reason);
            return;
        }

        unsigned long decodeHandle = 0;
        const auto ret = m_sdkManager.getStreamSecDecode(decodeHandle);
        if (ret != 0) {
            file.close();
            fail(QStringLiteral("申请解密通道失败, code=%1").arg(ret));
            return;
        }

        QMetaObject::invokeMethod(this, [this, filePath]() { emit playStarted(filePath); }, Qt::QueuedConnection);
        m_decodedFrameCount = 0;
        m_svacGotPicture = false;

        PsParser parser;
        parser.setFrameCallback([this, &decodeHandle](uint8_t *frameData, size_t frameLength, PsParser::FrameType frameType, int codecId) {
            Q_UNUSED(codecId)
            if (!frameData || frameLength == 0)
                return;
            if (frameType == PsParser::FrameType::Audio_frame)
                return;

            IMG_FRAME_UNIT inFrame{};
            inFrame.type = (frameType == PsParser::FrameType::I_frame) ? 'I' : 'P';
            inFrame.imgsz = static_cast<unsigned int>(frameLength);
            inFrame.img_buf = frameData;

            IMG_FRAME_UNIT *outFrame = nullptr;
            const auto decRet = m_sdkManager.videoDataSecDecodeExt(decodeHandle, &inFrame, &outFrame, false);
            if (decRet != 0 || !outFrame || !outFrame->img_buf || outFrame->imgsz == 0) {
                if (outFrame)
                    m_sdkManager.freeFrame(&outFrame);
                return;
            }

            Frame svacFrame;
            //svacFrame.keyFrame = !m_svacGotPicture || (frameType == PsParser::FrameType::I_frame);
            svacFrame.keyFrame = outFrame->type == 'I';
            svacFrame.data.assign(outFrame->img_buf, outFrame->img_buf + outFrame->imgsz);
            svacFrame.size = svacFrame.data.size();
            svacFrame.timestamp = 0;
            svacFrame.width = static_cast<int>(outFrame->width);
            svacFrame.height = static_cast<int>(outFrame->height);

            m_svacDecoder.inputFrame(svacFrame);
            m_sdkManager.freeFrame(&outFrame);
        });

        constexpr qint64 kChunkSize = 64 * 1024;
        while (!file.atEnd()) {
            const QByteArray chunk = file.read(kChunkSize);
            if (chunk.isEmpty())
                break;
            parser.input(reinterpret_cast<const uint8_t *>(chunk.constData()), static_cast<size_t>(chunk.size()));
        }
        file.close();

        m_sdkManager.closeHandle(decodeHandle);
        const int finishedFrames = m_decodedFrameCount;
        QMetaObject::invokeMethod(this, [this, finishedFrames]() { emit playFinished(finishedFrames); }, Qt::QueuedConnection);
        m_isPlaying = false;
    });

    return true;
}

bool PlaybackManager::playRtspUrl(const QString &rtspUrl)
{
    return playRtspInSlot(0, rtspUrl);
}

bool PlaybackManager::playRtspInSlot(int slot, const QString &rtspUrl)
{
    if (slot < 0 || slot >= kMaxRtspSlots) {
        emit playFailed(QStringLiteral("无效预览窗口索引: %1").arg(slot));
        return false;
    }
    if (rtspUrl.trimmed().isEmpty()) {
        emit playFailed(QStringLiteral("RTSP 地址为空"));
        return false;
    }

    QString reason;
    if (!ensureReady(reason)) {
        emit playFailed(reason);
        return false;
    }

    std::lock_guard<std::mutex> lock(m_rtspMutex);
    stopRtspSlotLocked(slot);
    QString startReason;
    if (!startRtspSlotLocked(slot, rtspUrl, startReason)) {
        emit playFailed(QStringLiteral("RTSP 启动失败(slot=%1): %2").arg(slot).arg(startReason));
        return false;
    }
    emit playStarted(QStringLiteral("slot%1: %2").arg(slot).arg(rtspUrl));
    return true;
}

bool PlaybackManager::startRtspSlotLocked(int slot, const QString &rtspUrl, QString &reason)
{
    auto &ctx = m_rtspSlots[slot];
    const QString channelId = QStringLiteral("ch%1").arg(slot);
    unsigned long decodeHandle = 0;
    const auto ret = m_sdkManager.getStreamSecDecode(decodeHandle);
    if (ret != 0) {
        reason = QStringLiteral("申请解密通道失败, code=%1").arg(ret);
        return false;
    }
    ctx.decodeHandle = decodeHandle;
    ctx.svacGotPicture = false;
    ctx.frameUpdateQueued = false;
    ctx.frameSerial = 0;
    ctx.url = rtspUrl;
    const auto generation = ctx.generation.fetch_add(1, std::memory_order_relaxed) + 1;
    ctx.decoder = std::make_unique<SVACDecoder>(4);
    SvacImageProvider::clearFrame(channelId);

    ctx.decoder->setOnDecode([this, slot, channelId, generation](const Frame &frame) {
        if (frame.data.empty() || frame.width <= 0 || frame.height <= 0)
            return;
        auto &slotCtx = m_rtspSlots[slot];
        if (slotCtx.generation.load(std::memory_order_relaxed) != generation || !slotCtx.playing)
            return;
        const QImage rgbImage = yuv420pToRgbImage(frame.data.data(), frame.width, frame.height);
        if (rgbImage.isNull())
            return;
        slotCtx.svacGotPicture = true;
        bool expected = false;
        if (!slotCtx.frameUpdateQueued.compare_exchange_strong(expected, true))
            return;
        QMetaObject::invokeMethod(this, [this, slot, channelId, rgbImage, generation]() {
            auto &uiCtx = m_rtspSlots[slot];
            if (uiCtx.generation.load(std::memory_order_relaxed) != generation || !uiCtx.playing) {
                uiCtx.frameUpdateQueued = false;
                return;
            }
            SvacImageProvider::setLatestFrame(channelId, rgbImage);
            ++uiCtx.frameSerial;
            ++m_frameSerial;
            emit frameSerialChanged();
            uiCtx.frameUpdateQueued = false;
        }, Qt::QueuedConnection);
    });

    ctx.client = std::make_shared<rtspwrap::RTSPClientWrapper>();
    ctx.playing = true;
    const std::string url = rtspUrl.toStdString();
    const bool started = ctx.client->start(url, [this, slot, generation](const uint8_t *data, unsigned size, timeval) {
        auto &slotCtx = m_rtspSlots[slot];
        if (slotCtx.generation.load(std::memory_order_relaxed) != generation)
            return;
        if (!slotCtx.playing || !slotCtx.decoder || !data || size == 0 || slotCtx.decodeHandle == 0)
            return;

        IMG_FRAME_UNIT inFrame{};
        inFrame.type = 'I';
        inFrame.imgsz = size;
        inFrame.img_buf = const_cast<uint8_t *>(data);

        IMG_FRAME_UNIT *outFrame = nullptr;
        const auto decRet = m_sdkManager.videoDataSecDecodeExt(slotCtx.decodeHandle, &inFrame, &outFrame, false);
        if (decRet != 0 || !outFrame || !outFrame->img_buf || outFrame->imgsz == 0) {
            if (outFrame)
                m_sdkManager.freeFrame(&outFrame);
            return;
        }

        Frame svacFrame;
        svacFrame.keyFrame = !slotCtx.svacGotPicture || (outFrame->type == 'I');
        svacFrame.data.assign(outFrame->img_buf, outFrame->img_buf + outFrame->imgsz);
        svacFrame.size = svacFrame.data.size();
        svacFrame.timestamp = 0;
        svacFrame.width = static_cast<int>(outFrame->width);
        svacFrame.height = static_cast<int>(outFrame->height);

        slotCtx.decoder->inputFrame(svacFrame);
        m_sdkManager.freeFrame(&outFrame);
    });

    if (!started) {
        stopRtspSlotLocked(slot);
        reason = QStringLiteral("RTSP client start 返回 false");
        return false;
    }
    return true;
}

