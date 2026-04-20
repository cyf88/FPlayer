#include "SvacImageProvider.h"

#include <QMutexLocker>

QMutex SvacImageProvider::s_mutex;
QImage SvacImageProvider::s_latestFrame;

SvacImageProvider::SvacImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

SvacImageProvider *SvacImageProvider::instance()
{
    return new SvacImageProvider();
}

void SvacImageProvider::setLatestFrame(const QImage &image)
{
    QMutexLocker locker(&s_mutex);
    s_latestFrame = image;
}

QImage SvacImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)

    QImage image;
    {
        QMutexLocker locker(&s_mutex);
        image = s_latestFrame;
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
