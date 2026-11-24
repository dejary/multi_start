#pragma once

#include "AppItem.h"

#include <QMainWindow>

class QIcon;
class FlowLayout;
class LaunchTile;
class QResizeEvent;
class QScreen;
class QScrollArea;
class QShowEvent;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QString configPath, QWidget *parent = nullptr);
    [[nodiscard]] const UpdateSettings &updateSettings() const { return m_config.updates; }

public slots:
    void start();

signals:
    void singleItemLaunchFinished(bool success);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void adaptToScreen(QScreen *screen);
    void buildUi();
    void loadConfiguration();
    void populateItems();
    void updateItemMetrics();
    [[nodiscard]] bool launchItem(int index);
    void moveFocus(int index, int key);
    [[nodiscard]] QIcon iconForItem(const AppItem &item) const;

    QString m_configPath;
    AppConfig m_config;
    QVector<AppItem> m_visibleItems;
    QScrollArea *m_scroll = nullptr;
    FlowLayout *m_flow = nullptr;
    QVector<LaunchTile *> m_tiles;
    bool m_hasAdaptedToScreen = false;
    bool m_screenTrackingConnected = false;
};
