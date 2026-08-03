#include "MainWindow.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>

class MainWindowSmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void rendersConfiguredItems();
    void automaticallyLaunchesOnlyVisibleItem();
};

void MainWindowSmokeTest::initTestCase()
{
    QCoreApplication::setApplicationVersion(QStringLiteral(MULTISTART_VERSION));
}

void MainWindowSmokeTest::rendersConfiguredItems()
{
    MainWindow window(QStringLiteral(MULTISTART_TEST_CONFIG));
    window.show();
    QTest::qWait(100);

    QCOMPARE(window.windowTitle(),
             QStringLiteral("程序启动中心 v%1").arg(QStringLiteral(MULTISTART_VERSION)));
    QVERIFY(!window.windowIcon().isNull());
    QVERIFY(window.findChild<QPushButton *>() == nullptr);
    const auto *listWidget = window.findChild<QListWidget *>();
    QVERIFY(listWidget != nullptr);
    QCOMPARE(listWidget->count(), 3);
    QCOMPARE(listWidget->item(0)->text(), QStringLiteral("记事本"));
    QCOMPARE(listWidget->item(1)->text(), QStringLiteral("演示脚本"));
    QCOMPARE(listWidget->item(2)->text(), QStringLiteral("数据目录"));
    QCOMPARE(listWidget->item(0)->icon().pixmap(64, 64).toImage(),
             QIcon(QStringLiteral(":/assets/default-exe.png")).pixmap(64, 64).toImage());
    QCOMPARE(listWidget->item(1)->icon().pixmap(64, 64).toImage(),
             QIcon(QStringLiteral(":/assets/default-bat.png")).pixmap(64, 64).toImage());
    QCOMPARE(listWidget->item(2)->icon().pixmap(64, 64).toImage(),
             QIcon(QStringLiteral(":/assets/default-folder.png")).pixmap(64, 64).toImage());

    const QString screenshotPath = qEnvironmentVariable("MULTISTART_SMOKE_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        const QPixmap screenshot = window.grab();
        QVERIFY(!screenshot.isNull());
        QVERIFY2(screenshot.save(screenshotPath, "PNG"), qPrintable(screenshotPath));
        QVERIFY(QFileInfo::exists(screenshotPath));
    }
}

void MainWindowSmokeTest::automaticallyLaunchesOnlyVisibleItem()
{
#ifndef Q_OS_WIN
    QSKIP("Automatic BAT launch behavior is Windows-specific.");
#else
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString batchPath = directory.filePath(QStringLiteral("launch-once.bat"));
    const QString markerPath = directory.filePath(QStringLiteral("launched.txt"));
    QFile batchFile(batchPath);
    QVERIFY(batchFile.open(QIODevice::WriteOnly | QIODevice::Text));
    batchFile.write("@echo off\r\n> \"%~1\" echo launched\r\n");
    batchFile.close();

    QJsonObject item;
    item.insert(QStringLiteral("name"), QStringLiteral("唯一项目"));
    item.insert(QStringLiteral("type"), QStringLiteral("bat"));
    item.insert(QStringLiteral("path"), QDir::fromNativeSeparators(batchPath));
    item.insert(QStringLiteral("arguments"),
                QJsonArray{QDir::fromNativeSeparators(markerPath)});
    QJsonObject root;
    root.insert(QStringLiteral("items"), QJsonArray{item});

    const QString configPath = directory.filePath(QStringLiteral("apps.json"));
    QFile configFile(configPath);
    QVERIFY(configFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    configFile.write(QJsonDocument(root).toJson());
    configFile.close();

    MainWindow window(configPath);
    QSignalSpy launchSpy(&window, &MainWindow::singleItemLaunchFinished);
    QVERIFY(!QFileInfo::exists(markerPath));
    window.start();
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(markerPath), 5000);
    QCOMPARE(launchSpy.count(), 1);
    QCOMPARE(launchSpy.takeFirst().at(0).toBool(), true);
    QVERIFY(!window.isVisible());
#endif
}

QTEST_MAIN(MainWindowSmokeTest)
#include "MainWindowSmokeTest.moc"
