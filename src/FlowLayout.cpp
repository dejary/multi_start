#include "FlowLayout.h"

FlowLayout::FlowLayout(QWidget *parent) : QLayout(parent)
{
    setContentsMargins(8, 8, 8, 8);
    setSpacing(6);
}

FlowLayout::~FlowLayout()
{
    qDeleteAll(m_items);
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
    invalidate();
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= count())
        return nullptr;
    auto *item = m_items.takeAt(index);
    invalidate();
    return item;
}

int FlowLayout::arrange(const QRect &rect, bool apply) const
{
    const QRect area = rect.marginsRemoved(contentsMargins());
    int x = area.left();
    int y = area.top();
    int rowHeight = 0;
    for (auto *item : m_items) {
        const int width = qMin(item->sizeHint().width(), qMax(1, area.width()));
        if (rowHeight && x + width > area.left() + area.width()) {
            x = area.left();
            y += rowHeight + spacing();
            rowHeight = 0;
        }
        const int height = item->hasHeightForWidth() ? item->heightForWidth(width)
                                                     : item->sizeHint().height();
        if (apply)
            item->setGeometry(QRect(x, y, width, height));
        rowHeight = qMax(rowHeight, height);
        x += width + spacing();
    }
    return y - rect.top() + rowHeight + contentsMargins().bottom();
}

int FlowLayout::heightForWidth(int width) const
{
    return arrange(QRect(0, 0, width, 0), false);
}

QSize FlowLayout::minimumSize() const
{
    QSize size(0, 0);
    for (auto *item : m_items)
        size = size.expandedTo(item->minimumSize());
    return size.grownBy(contentsMargins());
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    arrange(rect, true);
}
