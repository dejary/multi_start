#pragma once

#include <QIcon>
#include <QWidget>

class LaunchTile final : public QWidget {
    Q_OBJECT
public:
    LaunchTile(QString name, QIcon icon, QWidget *parent = nullptr);
    const QIcon &icon() const { return m_icon; }
    void setIconEdge(int edge);
    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

signals:
    void activated();
    void navigate(int key);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString m_name;
    QIcon m_icon;
    int m_iconEdge = 80;
};
