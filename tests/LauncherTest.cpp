#include "Launcher.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

class LauncherTest : public QObject {
    Q_OBJECT

private slots:
    void launchesBatchFileWithWindowsCommandLineQuoting();
};

void LauncherTest::launchesBatchFileWithWindowsCommandLineQuoting()
{
#ifndef Q_OS_WIN
    QSKIP("BAT launch behavior is Windows-specific.");
#else
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString deploymentDirectory = temporaryDirectory.filePath(QStringLiteral("部署 目录(测试)"));
    QVERIFY(QDir().mkpath(deploymentDirectory));

    const QString batchPath = QDir(deploymentDirectory).filePath(QStringLiteral("1.模拟设备.bat"));
    const QString markerPath = QDir(deploymentDirectory).filePath(QStringLiteral("启动 结果.txt"));

    QFile batchFile(batchPath);
    QVERIFY(batchFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&batchFile);
    stream << "@echo off\r\n";
    stream << "> \"%~1\" echo launched\r\n";
    batchFile.close();

    AppItem item;
    item.name = QStringLiteral("批处理测试");
    item.type = LaunchType::Batch;
    item.path = batchPath;
    item.arguments = QStringList{markerPath};

    const LaunchResult result = Launcher::launch(item);
    QVERIFY2(result.success, qPrintable(result.message));
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(markerPath), 5000);
#endif
}

QTEST_GUILESS_MAIN(LauncherTest)

#include "LauncherTest.moc"
