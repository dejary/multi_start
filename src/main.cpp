#include "MainWindow.h"

#include <check_auto_update/UpdateManager.h>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QSettings>

namespace {

QString defaultConfigPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("apps.json"));
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);


#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 高分辨率图标
#endif

    QCoreApplication::setApplicationName(QStringLiteral("MultiStart"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MULTISTART_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("MultiStart"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/assets/app.png")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("多个程序的统一启动入口"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configOption(
        {QStringLiteral("c"), QStringLiteral("config")},
        QStringLiteral("指定 JSON 配置文件。"),
        QStringLiteral("file"));
    parser.addOption(configOption);
    parser.process(application);

    const QString configPath = parser.isSet(configOption)
                                   ? QFileInfo(parser.value(configOption)).absoluteFilePath()
                                   : defaultConfigPath();

    MainWindow window(configPath);
    QObject::connect(&window,
                     &MainWindow::singleItemLaunchFinished,
                     &application,
                     [&application](bool) { application.quit(); });
    const UpdateSettings &settings = window.updateSettings();
    if (settings.enabled) {
        auto *updateManager = new CheckAutoUpdate::UpdateManager(&application);
        QObject::connect(updateManager,
                         &CheckAutoUpdate::UpdateManager::skippedVersionSelected,
                         &application,
                         [](const QString &version) {
                             QSettings().setValue(QStringLiteral("updates/skippedVersion"), version);
                         });
        QObject::connect(updateManager,
                         &CheckAutoUpdate::UpdateManager::finished,
                         &window,
                         [&window](bool proceedToApplication) {
                             if (proceedToApplication) {
                                 window.start();
                             }
                         });

        CheckAutoUpdate::UpdateConfig updateConfig;
        updateConfig.serverBaseUrl = settings.serverBaseUrl;
        updateConfig.appId = settings.appId;
        updateConfig.platform = settings.platform;
        updateConfig.channel = settings.channel;
        updateConfig.currentVersion = QCoreApplication::applicationVersion();
        updateConfig.skippedVersion =
            QSettings().value(QStringLiteral("updates/skippedVersion")).toString();
        updateConfig.dialogParent = &window;
        updateConfig.checkTimeoutMs = settings.checkTimeoutMs;
        updateConfig.downloadTimeoutMs = settings.downloadTimeoutMs;
        updateManager->startCheck(updateConfig);
    } else {
        window.start();
    }
    return application.exec();
}
