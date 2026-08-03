#pragma once

#include "AppItem.h"

#include <QString>

struct LaunchResult {
    bool success = false;
    QString message;
};

class Launcher {
public:
    [[nodiscard]] static LaunchResult launch(const AppItem &item);
};

