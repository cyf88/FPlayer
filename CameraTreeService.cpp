#include "CameraTreeService.h"

#include "CameraTreeModel.h"

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

CameraTreeService::CameraTreeService(CameraTreeModel *model, QObject *parent)
    : QObject(parent), m_model(model), m_nam(new QNetworkAccessManager(this))
{
}

void CameraTreeService::setEndpoint(const QUrl &url)
{
    if (m_endpoint == url)
        return;
    m_endpoint = url;
    emit endpointChanged();
}

void CameraTreeService::setLoading(bool v)
{
    if (m_loading == v)
        return;
    m_loading = v;
    emit loadingChanged(m_loading);
}

void CameraTreeService::refresh()
{
    if (!m_model) {
        emit errorOccurred(QStringLiteral("model is null"));
        return;
    }

    // 测试阶段：用本地测试数据替代 http 接口，避免网络/接口字段不一致导致无法展示树。
    // 你后续只需要把 refresh() 里这一段替换回网络请求即可。
    static const char *kTestJson = R"json(
{
  "code": 200,
  "msg": "操作成功",
  "data": [
    {
      "id": 46,
      "label": "海南省",
      "children": [
        {
          "id": 4601,
          "label": "海口市",
          "children": [
            {
              "id": 460105,
              "label": "秀英区",
              "children": [
                {
                  "id": 46010551,
                  "label": "秀英派出所",
                  "camera": [
                    { "id": "11133334", "name": "摄像头1" },
                    { "id": "11133335", "name": "摄像头2" }
                  ]
                },
                { "id": 46010552, "label": "海秀西路派出所", "children": [] }
              ]
            }
          ]
        }
      ]
    }
  ]
}
)json";

    setLoading(true);

    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(QByteArray(kTestJson), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        setLoading(false);
        emit errorOccurred(QStringLiteral("json parse error"));
        return;
    }

    m_model->setFromApiJson(doc.object());
    setLoading(false);
}

void CameraTreeService::loadFromJsonString(const QString &json)
{
    if (!m_model) {
        emit errorOccurred(QStringLiteral("model is null"));
        return;
    }
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred(QStringLiteral("json parse error"));
        return;
    }
    m_model->setFromApiJson(doc.object());
}

