#include "MainWindow.h"

#include <QFileInfo>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QTest>

class MainWindowSmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void rendersConfiguredItems();
};

void MainWindowSmokeTest::rendersConfiguredItems()
{
    MainWindow window(QStringLiteral(MULTISTART_TEST_CONFIG));
    window.show();
    QTest::qWait(100);

    QCOMPARE(window.windowTitle(), QStringLiteral("程序启动中心"));
    QVERIFY(!window.windowIcon().isNull());
    QVERIFY(window.findChild<QPushButton *>() == nullptr);
    const auto *listWidget = window.findChild<QListWidget *>();
    QVERIFY(listWidget != nullptr);
    QCOMPARE(listWidget->count(), 3);
    QCOMPARE(listWidget->item(0)->text(), QStringLiteral("记事本"));
    QCOMPARE(listWidget->item(1)->text(), QStringLiteral("演示脚本"));
    QCOMPARE(listWidget->item(2)->text(), QStringLiteral("数据目录"));

    const QString screenshotPath = qEnvironmentVariable("MULTISTART_SMOKE_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        const QPixmap screenshot = window.grab();
        QVERIFY(!screenshot.isNull());
        QVERIFY2(screenshot.save(screenshotPath, "PNG"), qPrintable(screenshotPath));
        QVERIFY(QFileInfo::exists(screenshotPath));
    }
}

QTEST_MAIN(MainWindowSmokeTest)
#include "MainWindowSmokeTest.moc"
