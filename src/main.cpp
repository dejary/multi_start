#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QIcon>

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
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
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
    window.show();
    return application.exec();
}
