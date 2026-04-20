#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class SvacImageProvider : public QQuickImageProvider
{
public:
    SvacImageProvider();

    static SvacImageProvider *instance();
    static void setLatestFrame(const QImage &image);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    Q_DISABLE_COPY_MOVE(SvacImageProvider)

    static QMutex s_mutex;
    static QImage s_latestFrame;
};
