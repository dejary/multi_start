#include "UpdateState.h"

#include <check_auto_update/UpdateManager.h>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class UpdateStateTest : public QObject {
    Q_OBJECT

private slots:
    void restoresInstalledPackageAfterRestart();
    void restoresSkippedPackageWithoutChangingInstalledPackage();
    void preservesLegacySkippedVersion();
};

void UpdateStateTest::restoresInstalledPackageAfterRestart()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("updates.ini"));
    const QString sha256(64, QLatin1Char('a'));
    {
        QSettings settings(path, QSettings::IniFormat);
        CheckAutoUpdate::UpdateManager manager;
        CheckAutoUpdate::UpdateConfig config;
        configureUpdateState(manager, settings, config);
        QVERIFY(config.currentPackageSha256.isEmpty());
        manager.finished(true); // No update, later, or a failed check must not mark a package installed.
        QVERIFY(!settings.contains(QStringLiteral("updates/currentPackageSha256")));
        manager.packageInstallStarted(QStringLiteral("1.0.0"), sha256);
        QCOMPARE(settings.status(), QSettings::NoError);
    }
    QSettings settings(path, QSettings::IniFormat);
    CheckAutoUpdate::UpdateManager manager;
    CheckAutoUpdate::UpdateConfig config;
    configureUpdateState(manager, settings, config);
    QCOMPARE(config.currentPackageSha256, sha256);
    QVERIFY(config.skippedVersion.isEmpty());
    QVERIFY(config.skippedPackageSha256.isEmpty());
}

void UpdateStateTest::restoresSkippedPackageWithoutChangingInstalledPackage()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("updates.ini"));
    const QString installedHash(64, QLatin1Char('a'));
    const QString skippedHash(64, QLatin1Char('b'));
    {
        QSettings settings(path, QSettings::IniFormat);
        CheckAutoUpdate::UpdateManager manager;
        CheckAutoUpdate::UpdateConfig config;
        configureUpdateState(manager, settings, config);
        manager.packageInstallStarted(QStringLiteral("1.0.0"), installedHash);
        manager.skippedPackageSelected(QStringLiteral("1.0.0"), skippedHash);
        QCOMPARE(settings.status(), QSettings::NoError);
    }
    QSettings settings(path, QSettings::IniFormat);
    CheckAutoUpdate::UpdateManager manager;
    CheckAutoUpdate::UpdateConfig config;
    configureUpdateState(manager, settings, config);
    QCOMPARE(config.currentPackageSha256, installedHash);
    QCOMPARE(config.skippedVersion, QStringLiteral("1.0.0"));
    QCOMPARE(config.skippedPackageSha256, skippedHash);
}

void UpdateStateTest::preservesLegacySkippedVersion()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("updates.ini")), QSettings::IniFormat);
    settings.setValue(QStringLiteral("updates/skippedVersion"), QStringLiteral("2.0.0"));
    CheckAutoUpdate::UpdateManager manager;
    CheckAutoUpdate::UpdateConfig config;
    configureUpdateState(manager, settings, config);
    QCOMPARE(config.skippedVersion, QStringLiteral("2.0.0"));
    QVERIFY(config.currentPackageSha256.isEmpty());
    QVERIFY(config.skippedPackageSha256.isEmpty());
}

QTEST_GUILESS_MAIN(UpdateStateTest)
#include "UpdateStateTest.moc"
