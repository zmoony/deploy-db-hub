#pragma once

#include <memory>

class QQmlEngine;
class QQuickWidget;
class QColor;
class QWidget;

class QmlEngineProvider final
{
public:
    static QmlEngineProvider &instance();

    QQmlEngine *engine() const;
    QQuickWidget *createWidget(QWidget *parent, const QColor &clearColor) const;

private:
    QmlEngineProvider();

    std::unique_ptr<QQmlEngine> m_engine;
};
