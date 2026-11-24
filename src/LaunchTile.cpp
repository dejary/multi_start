#include "LaunchTile.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <utility>

namespace {
constexpr int Padding = 6;
constexpr int Gap = 3;
constexpr int TextFlags = Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere;
}

LaunchTile::LaunchTile(QString name, QIcon icon, QWidget *parent)
    : QWidget(parent), m_name(std::move(name)), m_icon(std::move(icon))
{
    setAccessibleName(m_name);
    setFocusPolicy(Qt::StrongFocus);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

void LaunchTile::setIconEdge(int edge)
{
    if (edge == m_iconEdge)
        return;
    m_iconEdge = edge;
    updateGeometry();
    update();
}

QSize LaunchTile::sizeHint() const
{
    const int width = qMax(m_iconEdge, fontMetrics().horizontalAdvance(QString(8, QChar(0x56fd))))
                      + 2 * Padding;
    return QSize(width, heightForWidth(width));
}

int LaunchTile::heightForWidth(int width) const
{
    const QRect bounds(0, 0, qMax(1, width - 2 * Padding), 0);
    return 2 * Padding + m_iconEdge + Gap
           + fontMetrics().boundingRect(bounds, TextFlags, m_name).height();
}

void LaunchTile::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QStyleOption option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

    m_icon.paint(&painter, QRect((width() - m_iconEdge) / 2, Padding, m_iconEdge, m_iconEdge));
    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(rect().adjusted(Padding, Padding + m_iconEdge + Gap, -Padding, -Padding),
                     TextFlags, m_name);
}

void LaunchTile::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit activated();
    else
        QWidget::mouseDoubleClickEvent(event);
}

void LaunchTile::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (!event->isAutoRepeat())
            emit activated();
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
        emit navigate(event->key());
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
