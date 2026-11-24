#include "MainWindow.h"

#include "ConfigLoader.h"
#include "Launcher.h"

#include "FlowLayout.h"
#include "LaunchTile.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QSize>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#include <algorithm>
#include <utility>

MainWindow::MainWindow(QString configPath, QWidget *parent)
    : QMainWindow(parent), m_configPath(std::move(configPath))
{
    buildUi();
    loadConfiguration();
}

void MainWindow::start()
{
    if (m_visibleItems.size() != 1) {
        show();
        return;
    }

    QTimer::singleShot(0, this, [this]() {
        const bool success = launchItem(0);
        emit singleItemLaunchFinished(success);
    });
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    QScreen *screen = windowHandle() != nullptr ? windowHandle()->screen() : nullptr;
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    adaptToScreen(screen);

    if (!m_screenTrackingConnected && windowHandle() != nullptr) {
        connect(windowHandle(), &QWindow::screenChanged, this, &MainWindow::adaptToScreen);
        m_screenTrackingConnected = true;
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateItemMetrics();
}

void MainWindow::adaptToScreen(QScreen *screen)
{
    if (screen == nullptr) {
        return;
    }

    const QRect available = screen->availableGeometry();
    if (!available.isValid()) {
        return;
    }

    constexpr qreal ScreenUsage = 0.92;
    const int maximumWidth = std::max(320, static_cast<int>(available.width() * ScreenUsage));
    const int maximumHeight = std::max(240, static_cast<int>(available.height() * ScreenUsage));
    const int minimumWidth = std::min(480, maximumWidth);
    const int minimumHeight = std::min(360, maximumHeight);
    const int desiredWidth = m_hasAdaptedToScreen ? width() : m_config.window.width;
    const int desiredHeight = m_hasAdaptedToScreen ? height() : m_config.window.height;
    const int targetWidth = std::clamp(desiredWidth, minimumWidth, maximumWidth);
    const int targetHeight = std::clamp(desiredHeight, minimumHeight, maximumHeight);

    setMinimumSize(minimumWidth, minimumHeight);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(targetWidth, targetHeight);
    move(available.center() - rect().center());
    m_hasAdaptedToScreen = true;
    updateItemMetrics();
}

void MainWindow::updateItemMetrics()
{
    const qreal scale = std::clamp(std::min(width() / 800.0, height() / 600.0), 0.70, 1.40);
    for (auto *tile : m_tiles)
        tile->setIconEdge(qRound(80 * scale));
}

void MainWindow::buildUi()
{
    setWindowIcon(QIcon(QStringLiteral(":/assets/app.png")));

    auto *centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(20, 18, 20, 18);
    rootLayout->setSpacing(14);

    m_scroll = new QScrollArea(centralWidget);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFocusPolicy(Qt::NoFocus);
    m_scroll->setAccessibleName(QStringLiteral("程序列表"));
    auto *panel = new QWidget;
    panel->setObjectName(QStringLiteral("launchPanel"));
    m_flow = new FlowLayout(panel);
    m_scroll->setWidget(panel);
    rootLayout->addWidget(m_scroll);
    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("正在读取配置…"));

    setStyleSheet(QStringLiteral(R"(
        * { font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI"; }
        QMainWindow { background: #f4f7fb; }
        QScrollArea { background: white; border: 1px solid #dce4ef; border-radius: 10px; }
        QWidget#launchPanel { background: white; }
        LaunchTile { color: #1e293b; border: 1px solid transparent; border-radius: 8px; }
        LaunchTile:hover { background: #eef4ff; border-color: #c9dafb; }
        LaunchTile:focus { background: #dfeaff; border-color: #8db0ef; }
        QStatusBar { color: #526174; }
    )"));
}

void MainWindow::loadConfiguration()
{
    const ConfigResult result = ConfigLoader::load(m_configPath);
    if (!result.ok()) {
        statusBar()->showMessage(QStringLiteral("配置读取失败"));
        QMessageBox::critical(this, QStringLiteral("配置错误"), result.error);
        return;
    }

    m_config = result.config;
    if (!m_config.version.isEmpty()) {
        QCoreApplication::setApplicationVersion(m_config.version);
    }
    setWindowTitle(QStringLiteral("%1 v%2")
                       .arg(m_config.window.title, QCoreApplication::applicationVersion()));
    resize(m_config.window.width, m_config.window.height);

    populateItems();
    if (result.warnings.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("已加载 %1 个项目 · 双击或按 Enter 启动").arg(m_visibleItems.size()));
    } else {
        statusBar()->showMessage(
            QStringLiteral("已加载 %1 个项目，%2 条配置警告")
                .arg(m_visibleItems.size())
                .arg(result.warnings.size()));
        QMessageBox::warning(this,
                             QStringLiteral("配置警告"),
                             result.warnings.join(QLatin1Char('\n')));
    }
}

void MainWindow::populateItems()
{
    for (const AppItem &item : m_config.items) {
        if (item.visible) {
            m_visibleItems.append(item);
        }
    }

    if (m_visibleItems.isEmpty()) {
        m_flow->addWidget(new QLabel(QStringLiteral("配置中没有可显示的项目")));
        return;
    }

    for (int index = 0; index < m_visibleItems.size(); ++index) {
        const AppItem &item = m_visibleItems.at(index);
        auto *tile = new LaunchTile(item.name, iconForItem(item));
        m_flow->addWidget(tile);
        m_tiles.append(tile);
        connect(tile, &LaunchTile::activated, this, [this, index] { (void)launchItem(index); });
        connect(tile, &LaunchTile::navigate, this, [this, index](int key) { moveFocus(index, key); });

        QString toolTip = item.description;
        if (!toolTip.isEmpty()) {
            toolTip += QStringLiteral("\n\n");
        }
        toolTip += item.path;
        tile->setToolTip(toolTip);
        tile->setStatusTip(item.description);
    }

    updateItemMetrics();
    m_tiles.first()->setFocus();
}

QIcon MainWindow::iconForItem(const AppItem &item) const
{
    if (!item.iconPath.isEmpty() && QFileInfo::exists(item.iconPath)) {
        const QIcon configuredIcon(item.iconPath);
        if (!configuredIcon.isNull()) {
            return configuredIcon;
        }
    }

    if (item.type == LaunchType::Folder) {
        return QIcon(QStringLiteral(":/assets/default-folder.png"));
    }
    if (item.type == LaunchType::Batch) {
        return QIcon(QStringLiteral(":/assets/default-bat.png"));
    }
    return QIcon(QStringLiteral(":/assets/default-exe.png"));
}

void MainWindow::moveFocus(int index, int key)
{
    const int direction = (key == Qt::Key_Left || key == Qt::Key_Up) ? -1 : 1;
    const auto origin = m_tiles.at(index)->geometry();
    int target = index + direction;
    if (key == Qt::Key_Up || key == Qt::Key_Down) {
        while (target >= 0 && target < m_tiles.size()
               && m_tiles.at(target)->y() == origin.y())
            target += direction;
        if (target < 0 || target >= m_tiles.size())
            return;
        const int row = m_tiles.at(target)->y();
        for (int i = target + direction; i >= 0 && i < m_tiles.size()
             && m_tiles.at(i)->y() == row; i += direction) {
            if (qAbs(m_tiles.at(i)->x() - origin.x()) < qAbs(m_tiles.at(target)->x() - origin.x()))
                target = i;
        }
    }
    if (target >= 0 && target < m_tiles.size()) {
        m_tiles.at(target)->setFocus(Qt::TabFocusReason);
        m_scroll->ensureWidgetVisible(m_tiles.at(target), 0, 4);
    }
}

bool MainWindow::launchItem(int index)
{
    const AppItem &item = m_visibleItems.at(index);
    const LaunchResult result = Launcher::launch(item);
    statusBar()->showMessage(result.message, 8000);
    if (!result.success) {
        QMessageBox::warning(this, QStringLiteral("无法启动“%1”").arg(item.name), result.message);
    }
    return result.success;
}
