#pragma once

class QSettings;

namespace CheckAutoUpdate {
class UpdateManager;
struct UpdateConfig;
}

// Restore package identity and persist the update library's package notifications.
void configureUpdateState(CheckAutoUpdate::UpdateManager &manager,
                          QSettings &settings,
                          CheckAutoUpdate::UpdateConfig &config);
