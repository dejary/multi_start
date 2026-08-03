#pragma once

#include "AppItem.h"

#include <QMainWindow>

class QIcon;
class QListWidget;
class QListWidgetItem;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QString configPath, QWidget *parent = nullptr);

private slots:
    void activateItem(QListWidgetItem *listItem);

private:
    void buildUi();
    void loadConfiguration();
    void populateItems();
    [[nodiscard]] QIcon iconForItem(const AppItem &item) const;

    QString m_configPath;
    AppConfig m_config;
    QVector<AppItem> m_visibleItems;
    QListWidget *m_listWidget = nullptr;
};
