#include "qml/QmlEngineProvider.h"

#include <QQmlEngine>
#include <QQuickWidget>

QmlEngineProvider &QmlEngineProvider::instance()
{
    static QmlEngineProvider provider;
    return provider;
}

QmlEngineProvider::QmlEngineProvider()
    : m_engine(std::make_unique<QQmlEngine>())
{
    m_engine->addImportPath(QStringLiteral("qrc:/"));
}

QQmlEngine *QmlEngineProvider::engine() const
{
    return m_engine.get();
}

QQuickWidget *QmlEngineProvider::createWidget(QWidget *parent, const QColor &clearColor) const
{
    auto *widget = new QQuickWidget(m_engine.get(), parent);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget->setClearColor(clearColor);
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    return widget;
}
