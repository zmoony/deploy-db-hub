#include "qml/WidgetHostItem.h"

#include <QQuickWindow>
#include <QWidget>

WidgetHostItem::WidgetHostItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

QWidget *WidgetHostItem::widget() const
{
    return m_widget;
}

void WidgetHostItem::setWidget(QWidget *widget)
{
    if (m_widget == widget) {
        return;
    }
    m_widget = widget;
    updateContainer();
    emit widgetChanged();
}

void WidgetHostItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    if (change == QQuickItem::ItemSceneChange && data.window != nullptr) {
        updateContainer();
        return;
    }
    if (change == QQuickItem::ItemVisibleHasChanged) {
        updateContainerGeometry();
    }
}

void WidgetHostItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateContainerGeometry();
}

void WidgetHostItem::updateContainerGeometry()
{
    if (m_container == nullptr) {
        return;
    }

    if (!isVisible() || width() <= 0 || height() <= 0 || window() == nullptr) {
        m_container->hide();
        return;
    }

    const QPointF topLeft = mapToScene(QPointF(0, 0));
    m_container->setGeometry(qRound(topLeft.x()),
                             qRound(topLeft.y()),
                             qMax(0, qRound(width())),
                             qMax(0, qRound(height())));
    m_container->show();
}

void WidgetHostItem::updateContainer()
{
    if (m_container != nullptr) {
        m_container->deleteLater();
        m_container = nullptr;
    }
    if (m_widget == nullptr || window() == nullptr) {
        return;
    }

    if (m_widget->windowHandle() == nullptr) {
        m_widget->setAttribute(Qt::WA_DontCreateNativeAncestors);
        m_widget->winId();
    }

    m_container = QWidget::createWindowContainer(m_widget->windowHandle(), nullptr, Qt::FramelessWindowHint);
    m_container->show();

    if (QWindow *quickWindow = window()) {
        m_container->windowHandle()->setParent(quickWindow);
    }

    updateContainerGeometry();
}
