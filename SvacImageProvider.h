#pragma once

#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class SvacImageProvider : public QQuickImageProvider
{
public:
    SvacImageProvider();

    static SvacImageProvider *instance();
    static void setLatestFrame(const QString &channelId, const QImage &image);
    static void clearFrame(const QString &channelId);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    Q_DISABLE_COPY_MOVE(SvacImageProvider)

    static QMutex s_mutex;
    static QHash<QString, QImage> s_latestFrames;
};
