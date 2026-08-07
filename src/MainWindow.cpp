#include "MainWindow.h"

#include "ConfigLoader.h"
#include "Launcher.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QListWidget>
#include <QMessageBox>
#include <QSize>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

namespace {
constexpr int ItemIndexRole = Qt::UserRole + 1;
}

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
        const bool success = m_listWidget->count() == 1 && launchItem(m_listWidget->item(0));
        emit singleItemLaunchFinished(success);
    });
}

void MainWindow::buildUi()
{
    setWindowIcon(QIcon(QStringLiteral(":/assets/app.png")));

    auto *centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(20, 18, 20, 18);
    rootLayout->setSpacing(14);

    m_listWidget = new QListWidget(centralWidget);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setMovement(QListView::Static);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setWrapping(true);
    m_listWidget->setWordWrap(true);
    m_listWidget->setIconSize(QSize(64, 64));
    m_listWidget->setGridSize(QSize(148, 118));
    m_listWidget->setSpacing(8);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setAccessibleName(QStringLiteral("程序列表"));
    connect(m_listWidget, &QListWidget::itemActivated, this, &MainWindow::activateItem);

    rootLayout->addWidget(m_listWidget, 1);
    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("正在读取配置…"));

    setStyleSheet(QStringLiteral(R"(
        * { font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI"; }
        QMainWindow { background: #f4f7fb; }
        QListWidget {
            background: #ffffff;
            border: 1px solid #dce4ef;
            border-radius: 10px;
            padding: 10px;
            outline: none;
        }
        QListWidget::item {
            color: #1e293b;
            border: 1px solid transparent;
            border-radius: 9px;
            padding: 8px;
        }
        QListWidget::item:hover {
            background: #eef4ff;
            border-color: #c9dafb;
        }
        QListWidget::item:selected {
            color: #173a76;
            background: #dfeaff;
            border-color: #8db0ef;
        }
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
    m_listWidget->clear();
    m_visibleItems.clear();

    for (const AppItem &item : m_config.items) {
        if (item.visible) {
            m_visibleItems.append(item);
        }
    }

    if (m_visibleItems.isEmpty()) {
        auto *emptyItem = new QListWidgetItem(
            style()->standardIcon(QStyle::SP_MessageBoxInformation),
            QStringLiteral("配置中没有可显示的项目"),
            m_listWidget);
        emptyItem->setFlags(Qt::NoItemFlags);
        return;
    }

    for (int index = 0; index < m_visibleItems.size(); ++index) {
        const AppItem &item = m_visibleItems.at(index);
        auto *listItem = new QListWidgetItem(iconForItem(item), item.name, m_listWidget);
        listItem->setData(ItemIndexRole, index);
        listItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);

        QString toolTip = item.description;
        if (!toolTip.isEmpty()) {
            toolTip += QStringLiteral("\n\n");
        }
        toolTip += item.path;
        listItem->setToolTip(toolTip);
        listItem->setStatusTip(item.description);
    }

    m_listWidget->setCurrentRow(0);
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

void MainWindow::activateItem(QListWidgetItem *listItem)
{
    (void)launchItem(listItem);
}

bool MainWindow::launchItem(QListWidgetItem *listItem)
{
    if (listItem == nullptr) {
        return false;
    }

    bool validIndex = false;
    const int index = listItem->data(ItemIndexRole).toInt(&validIndex);
    if (!validIndex || index < 0 || index >= m_visibleItems.size()) {
        return false;
    }

    const AppItem &item = m_visibleItems.at(index);
    const LaunchResult result = Launcher::launch(item);
    statusBar()->showMessage(result.message, 8000);
    if (!result.success) {
        QMessageBox::warning(this, QStringLiteral("无法启动“%1”").arg(item.name), result.message);
    }
    return result.success;
}
