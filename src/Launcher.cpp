#include "Launcher.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

namespace {

QString effectiveWorkingDirectory(const AppItem &item)
{
    if (!item.workingDirectory.isEmpty()) {
        return item.workingDirectory;
    }
    const QFileInfo target(item.path);
    return item.type == LaunchType::Folder ? target.absoluteFilePath() : target.absolutePath();
}

#ifdef Q_OS_WIN
QString quoteForCommandPrompt(QString value)
{
    value.replace(QStringLiteral("\""), QStringLiteral("\"\""));
    return QStringLiteral("\"") + value + QStringLiteral("\"");
}
#endif

LaunchResult startDetached(const QString &program,
                           const QStringList &arguments,
                           const QString &workingDirectory,
                           const QString &nativeArguments = QString())
{
    if (!workingDirectory.isEmpty() && !QFileInfo(workingDirectory).isDir()) {
        return {false, QStringLiteral("工作目录不存在：%1").arg(workingDirectory)};
    }

    QProcess process;
    process.setProgram(program);
#ifdef Q_OS_WIN
    if (!nativeArguments.isNull()) {
        process.setNativeArguments(nativeArguments);
    } else {
        process.setArguments(arguments);
    }
#else
    Q_UNUSED(nativeArguments)
    process.setArguments(arguments);
#endif
    process.setWorkingDirectory(workingDirectory);
    qint64 processId = 0;
    if (!process.startDetached(&processId)) {
        return {false, QStringLiteral("启动失败：%1").arg(program)};
    }
    return {true, QStringLiteral("已启动（PID %1）").arg(processId)};
}

} // namespace

LaunchResult Launcher::launch(const AppItem &item)
{
    const QFileInfo target(item.path);
    if (!target.exists()) {
        return {false, QStringLiteral("目标不存在：%1").arg(item.path)};
    }

    if (item.type == LaunchType::Folder) {
        if (!target.isDir()) {
            return {false, QStringLiteral("目标不是目录：%1").arg(item.path)};
        }
        const bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(target.absoluteFilePath()));
        return opened
                   ? LaunchResult{true, QStringLiteral("已打开目录：%1").arg(item.name)}
                   : LaunchResult{false, QStringLiteral("无法打开目录：%1").arg(item.path)};
    }

    if (!target.isFile()) {
        return {false, QStringLiteral("目标不是文件：%1").arg(item.path)};
    }

    const QString workingDirectory = effectiveWorkingDirectory(item);
    if (item.type == LaunchType::Executable) {
        return startDetached(target.absoluteFilePath(), item.arguments, workingDirectory);
    }

#ifdef Q_OS_WIN
    const QString suffix = target.suffix().toLower();
    if (suffix != QStringLiteral("bat") && suffix != QStringLiteral("cmd")) {
        return {false, QStringLiteral("批处理目标必须是 .bat 或 .cmd 文件：%1").arg(item.path)};
    }

    QString command = QStringLiteral("call ") + quoteForCommandPrompt(target.absoluteFilePath());
    for (const QString &argument : item.arguments) {
        command += QLatin1Char(' ') + quoteForCommandPrompt(argument);
    }

    const QString commandProcessor = qEnvironmentVariable("ComSpec", QStringLiteral("cmd.exe"));
    const QString nativeArguments = QStringLiteral("/d /v:off /s /c ") + command;
    return startDetached(commandProcessor, {}, workingDirectory, nativeArguments);
#else
    return {false, QStringLiteral("BAT 文件只能在 Windows 系统上执行。")} ;
#endif
}

