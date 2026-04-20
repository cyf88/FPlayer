#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class CameraTreeModel;

class CameraTreeService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl endpoint READ endpoint WRITE setEndpoint NOTIFY endpointChanged)
public:
    explicit CameraTreeService(CameraTreeModel *model, QObject *parent = nullptr);

    QUrl endpoint() const { return m_endpoint; }
    void setEndpoint(const QUrl &url);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadFromJsonString(const QString &json); // 方便本地测试

signals:
    void endpointChanged();
    void loadingChanged(bool loading);
    void errorOccurred(const QString &message);

private:
    CameraTreeModel *m_model = nullptr;
    QUrl m_endpoint;
    QNetworkAccessManager *m_nam = nullptr;
    bool m_loading = false;

    void setLoading(bool v);
};

