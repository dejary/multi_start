#pragma once

#include "AppItem.h"

#include <QString>
#include <QStringList>

struct ConfigResult {
    AppConfig config;
    QString error;
    QStringList warnings;

    [[nodiscard]] bool ok() const { return error.isEmpty(); }
};

class ConfigLoader {
public:
    [[nodiscard]] static ConfigResult load(const QString &configPath);
};

