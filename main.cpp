#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "LoginManager.h"
#include "CameraTreeModel.h"
#include "CameraTreeService.h"
#include "PlaybackManager.h"
#include "SvacImageProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    LoginManager loginManager;
    engine.rootContext()->setContextProperty("LoginManager", &loginManager);

    auto *cameraTreeModel = new CameraTreeModel(&engine);
    auto *cameraTreeService = new CameraTreeService(cameraTreeModel, &engine);
    auto *playbackManager = new PlaybackManager(&engine);
    engine.addImageProvider(QStringLiteral("svac"), SvacImageProvider::instance());
    engine.rootContext()->setContextProperty("CameraTreeModel", cameraTreeModel);
    engine.rootContext()->setContextProperty("CameraTreeService", cameraTreeService);
    engine.rootContext()->setContextProperty("PlaybackManager", playbackManager);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("FPlayer", "Login");

    SetConsoleOutputCP(65001);

    return app.exec();
}
