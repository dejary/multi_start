#pragma once

#include <QLayout>
#include <QList>

class FlowLayout final : public QLayout {
public:
    explicit FlowLayout(QWidget *parent);
    ~FlowLayout() override;
    void addItem(QLayoutItem *item) override;
    int count() const override { return m_items.size(); }
    QLayoutItem *itemAt(int index) const override { return m_items.value(index); }
    QLayoutItem *takeAt(int index) override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;

private:
    int arrange(const QRect &rect, bool apply) const;
    QList<QLayoutItem *> m_items;
};
