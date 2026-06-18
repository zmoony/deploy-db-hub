#include "infra/AppBranding.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QWidget>

namespace {

QString resolveIconPath(const QString &fileName)
{
    const QString appImages = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("images"));
    const QStringList candidates = {
        QDir(appImages).filePath(fileName),
        QStringLiteral(":/images/") + fileName
    };
    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return {};
}

QPixmap renderSvgPixmap(const QString &path, int size)
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        return {};
    }

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(0, 0, size, size));
    return pixmap;
}

QIcon buildVectorIcon()
{
    const QString svgPath = resolveIconPath(QStringLiteral("icon.svg"));
    if (!svgPath.isEmpty()) {
        QIcon icon;
        for (int size : {16, 20, 24, 32, 48, 64, 128, 256}) {
            const QPixmap pixmap = renderSvgPixmap(svgPath, size);
            if (!pixmap.isNull()) {
                icon.addPixmap(pixmap);
            }
        }
        if (!icon.isNull()) {
            return icon;
        }
    }

    const QString pngPath = resolveIconPath(QStringLiteral("icon.png"));
    if (!pngPath.isEmpty()) {
        return QIcon(pngPath);
    }
    return {};
}

}

namespace AppBranding {

QIcon applicationIcon()
{
    static QIcon cached = buildVectorIcon();
    return cached;
}

void applyWindowIcon(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }
    const QIcon icon = applicationIcon();
    if (!icon.isNull()) {
        widget->setWindowIcon(icon);
    }
}

}
