#include "SvacImageProvider.h"

#include <QMutexLocker>

QMutex SvacImageProvider::s_mutex;
QHash<QString, QImage> SvacImageProvider::s_latestFrames;

SvacImageProvider::SvacImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

SvacImageProvider *SvacImageProvider::instance()
{
    return new SvacImageProvider();
}

void SvacImageProvider::setLatestFrame(const QString &channelId, const QImage &image)
{
    QMutexLocker locker(&s_mutex);
    s_latestFrames.insert(channelId, image);
}

void SvacImageProvider::clearFrame(const QString &channelId)
{
    QMutexLocker locker(&s_mutex);
    s_latestFrames.remove(channelId);
}

QImage SvacImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString channelId = id.section('?', 0, 0);

    QImage image;
    {
        QMutexLocker locker(&s_mutex);
        image = s_latestFrames.value(channelId);
    }

    if (image.isNull()) {
        QImage fallback(640, 360, QImage::Format_RGB32);
        fallback.fill(Qt::black);
        if (size)
            *size = fallback.size();
        return fallback;
    }

    if (requestedSize.isValid()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (size)
        *size = image.size();

    return image;
}
