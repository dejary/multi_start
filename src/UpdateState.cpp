#include "UpdateState.h"

#include <check_auto_update/UpdateManager.h>
#include <QSettings>

void configureUpdateState(CheckAutoUpdate::UpdateManager &manager,
                          QSettings &settings,
                          CheckAutoUpdate::UpdateConfig &config)
{
    config.currentPackageSha256 =
        settings.value(QStringLiteral("updates/currentPackageSha256")).toString();
    config.skippedVersion = settings.value(QStringLiteral("updates/skippedVersion")).toString();
    config.skippedPackageSha256 =
        settings.value(QStringLiteral("updates/skippedPackageSha256")).toString();

    QObject::connect(&manager, &CheckAutoUpdate::UpdateManager::packageInstallStarted,
                     &settings, [&settings](const QString &, const QString &sha256) {
                         settings.setValue(QStringLiteral("updates/currentPackageSha256"), sha256);
                         settings.sync();
                     });
    QObject::connect(&manager, &CheckAutoUpdate::UpdateManager::skippedPackageSelected,
                     &settings, [&settings](const QString &version, const QString &sha256) {
                         settings.setValue(QStringLiteral("updates/skippedVersion"), version);
                         settings.setValue(QStringLiteral("updates/skippedPackageSha256"), sha256);
                         settings.sync();
                     });
}
