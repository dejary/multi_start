#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

enum class LaunchType {
    Executable,
    Batch,
    Folder
};

struct AppItem {
    QString name;
    QString description;
    LaunchType type = LaunchType::Executable;
    QString path;
    QString iconPath;
    QString workingDirectory;
    QStringList arguments;
    bool visible = true;
};

struct WindowSettings {
    QString title = QStringLiteral("程序启动中心");
    int width = 800;
    int height = 600;
};

struct UpdateSettings {
    bool enabled = false;
    QString serverBaseUrl;
    QString appId = QStringLiteral("multi_start");
    QString platform = QStringLiteral("windows-x86");
    QString channel = QStringLiteral("stable");
    int checkTimeoutMs = 2000;
    int downloadTimeoutMs = 30000;
};

struct AppConfig {
    QString version;
    WindowSettings window;
    UpdateSettings updates;
    QVector<AppItem> items;
};
