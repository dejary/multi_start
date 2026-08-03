#include "ConfigLoader.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class ConfigLoaderTest final : public QObject {
    Q_OBJECT

private slots:
    void loadsValidConfiguration();
    void skipsInvalidItems();
    void rejectsMalformedJson();
};

namespace {

QString writeConfig(QTemporaryDir &directory, const QByteArray &contents)
{
    const QString path = QDir(directory.path()).filePath(QStringLiteral("apps.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    file.write(contents);
    file.close();
    return path;
}

} // namespace

void ConfigLoaderTest::loadsValidConfiguration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = writeConfig(directory, R"({
        "window": {"title": "测试中心", "width": 900, "height": 650},
        "updates": {
            "enabled": true,
            "serverBaseUrl": "https://updates.example.com",
            "appId": "test-app",
            "platform": "windows-x86",
            "channel": "beta"
        },
        "items": [
            {"name": "工具", "type": "exe", "path": "bin/tool.exe", "arguments": ["--safe"]},
            {"name": "脚本", "type": "bat", "path": "run.bat"},
            {"name": "目录", "type": "folder", "path": "data"}
        ]
    })");

    const ConfigResult result = ConfigLoader::load(path);
    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.config.window.title, QStringLiteral("测试中心"));
    QCOMPARE(result.config.window.width, 900);
    QVERIFY(result.config.updates.enabled);
    QCOMPARE(result.config.updates.serverBaseUrl, QStringLiteral("https://updates.example.com"));
    QCOMPARE(result.config.updates.appId, QStringLiteral("test-app"));
    QCOMPARE(result.config.updates.channel, QStringLiteral("beta"));
    QCOMPARE(result.config.items.size(), 3);
    QCOMPARE(result.config.items.at(0).arguments, QStringList{QStringLiteral("--safe")});
    QCOMPARE(result.config.items.at(0).path,
             QDir::cleanPath(QDir(directory.path()).filePath(QStringLiteral("bin/tool.exe"))));
    QCOMPARE(result.config.items.at(2).type, LaunchType::Folder);
}

void ConfigLoaderTest::skipsInvalidItems()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = writeConfig(directory, R"({
        "items": [
            {"name": "有效目录", "type": "folder", "path": "data"},
            {"name": "缺少路径", "type": "exe"},
            {"name": "未知类型", "type": "url", "path": "https://example.com"}
        ]
    })");

    const ConfigResult result = ConfigLoader::load(path);
    QVERIFY(result.ok());
    QCOMPARE(result.config.items.size(), 1);
    QCOMPARE(result.warnings.size(), 2);
}

void ConfigLoaderTest::rejectsMalformedJson()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = writeConfig(directory, QByteArrayLiteral("{ invalid json"));
    const ConfigResult result = ConfigLoader::load(path);
    QVERIFY(!result.ok());
    QVERIFY(!result.error.isEmpty());
}

QTEST_MAIN(ConfigLoaderTest)
#include "ConfigLoaderTest.moc"
