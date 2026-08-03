#include "ConfigLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>
#include <optional>

namespace {

QString resolvePath(const QString &value, const QString &baseDirectory)
{
    if (value.trimmed().isEmpty()) {
        return {};
    }

    const QFileInfo pathInfo(value);
    if (pathInfo.isAbsolute()) {
        return QDir::cleanPath(pathInfo.absoluteFilePath());
    }

    return QDir::cleanPath(QFileInfo(QDir(baseDirectory).filePath(value)).absoluteFilePath());
}

std::optional<LaunchType> parseType(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("exe") || normalized == QStringLiteral("executable")) {
        return LaunchType::Executable;
    }
    if (normalized == QStringLiteral("bat") || normalized == QStringLiteral("batch")) {
        return LaunchType::Batch;
    }
    if (normalized == QStringLiteral("folder") || normalized == QStringLiteral("directory")) {
        return LaunchType::Folder;
    }
    return std::nullopt;
}

} // namespace

ConfigResult ConfigLoader::load(const QString &configPath)
{
    ConfigResult result;
    const QFileInfo configInfo(configPath);

    QFile file(configInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("无法打开配置文件：%1\n%2")
                           .arg(configInfo.absoluteFilePath(), file.errorString());
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        result.error = QStringLiteral("配置文件 JSON 格式错误（第 %1 字节）：%2")
                           .arg(parseError.offset)
                           .arg(parseError.errorString());
        return result;
    }
    if (!document.isObject()) {
        result.error = QStringLiteral("配置文件根节点必须是 JSON 对象。");
        return result;
    }

    const QJsonObject root = document.object();
    const QJsonValue windowValue = root.value(QStringLiteral("window"));
    if (windowValue.isObject()) {
        const QJsonObject window = windowValue.toObject();
        const QString title = window.value(QStringLiteral("title")).toString().trimmed();
        if (!title.isEmpty()) {
            result.config.window.title = title;
        }
        result.config.window.width = std::clamp(
            window.value(QStringLiteral("width")).toInt(result.config.window.width), 480, 3840);
        result.config.window.height = std::clamp(
            window.value(QStringLiteral("height")).toInt(result.config.window.height), 360, 2160);
    }

    const QJsonValue itemsValue = root.value(QStringLiteral("items"));
    if (!itemsValue.isArray()) {
        result.error = QStringLiteral("配置字段“items”必须是数组。");
        return result;
    }

    const QString baseDirectory = configInfo.absolutePath();
    const QJsonArray items = itemsValue.toArray();
    for (int index = 0; index < items.size(); ++index) {
        if (!items.at(index).isObject()) {
            result.warnings.append(QStringLiteral("第 %1 个启动项不是对象，已忽略。").arg(index + 1));
            continue;
        }

        const QJsonObject object = items.at(index).toObject();
        AppItem item;
        item.name = object.value(QStringLiteral("name")).toString().trimmed();
        item.description = object.value(QStringLiteral("description")).toString().trimmed();
        item.visible = object.value(QStringLiteral("visible")).toBool(true);

        const auto type = parseType(object.value(QStringLiteral("type")).toString());
        const QString configuredPath = object.value(QStringLiteral("path")).toString().trimmed();
        if (item.name.isEmpty() || !type.has_value() || configuredPath.isEmpty()) {
            result.warnings.append(
                QStringLiteral("第 %1 个启动项缺少有效的 name、type 或 path，已忽略。")
                    .arg(index + 1));
            continue;
        }

        item.type = *type;
        item.path = resolvePath(configuredPath, baseDirectory);
        item.iconPath = resolvePath(object.value(QStringLiteral("icon")).toString(), baseDirectory);
        item.workingDirectory = resolvePath(
            object.value(QStringLiteral("workingDirectory")).toString(), baseDirectory);

        const QJsonValue argumentsValue = object.value(QStringLiteral("arguments"));
        if (!argumentsValue.isUndefined() && !argumentsValue.isArray()) {
            result.warnings.append(
                QStringLiteral("启动项“%1”的 arguments 不是数组，已忽略该字段。").arg(item.name));
        } else {
            for (const QJsonValue &argument : argumentsValue.toArray()) {
                if (argument.isString()) {
                    item.arguments.append(argument.toString());
                } else {
                    result.warnings.append(
                        QStringLiteral("启动项“%1”包含非字符串参数，该参数已忽略。")
                            .arg(item.name));
                }
            }
        }

        result.config.items.append(std::move(item));
    }

    return result;
}
