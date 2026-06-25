#include "qml/QmlPerfConfig.h"

#include <QCoreApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

namespace QmlPerfConfig {

void applyEnvironment()
{
    qputenv("QSG_RHI_BACKEND", "opengl");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
}

void applyGraphics()
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(0);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

}
