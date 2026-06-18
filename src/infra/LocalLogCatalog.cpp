#include "infra/LocalLogCatalog.h"

#include "infra/DataPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {
const QRegularExpression LocalLogPattern(QStringLiteral("^logs/[^/\\\\]+\\.log$"));
}

bool LocalLogCatalog::isValidRelativePath(const QString &relativePath)
{
    return LocalLogPattern.match(QDir::fromNativeSeparators(relativePath.trimmed())).hasMatch();
}

QString LocalLogCatalog::absolutePath(const QString &relativePath)
{
    if (!isValidRelativePath(relativePath)) {
        return {};
    }
    const QString fileName = relativePath.mid(QStringLiteral("logs/").size());
    return QDir(DataPaths::logsDir()).filePath(fileName);
}

QStringList LocalLogCatalog::listRelativeLogFiles()
{
    QDir dir(DataPaths::logsDir());
    if (!dir.exists()) {
        return {};
    }

    const QFileInfoList files = dir.entryInfoList(
        {QStringLiteral("*.log")},
        QDir::Files,
        QDir::Time | QDir::Reversed);

    QStringList result;
    result.reserve(files.size());
    for (const QFileInfo &info : files) {
        result.append(QStringLiteral("logs/") + info.fileName());
    }
    return result;
}

QString LocalLogCatalog::loadContent(const QString &relativePath, QString *error)
{
    const QString absolute = absolutePath(relativePath);
    if (absolute.isEmpty()) {
        if (error) {
            *error = QStringLiteral("日志路径格式无效，应为 logs/<filename>.log");
        }
        return {};
    }

    QFile file(absolute);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral("日志文件不存在或无法读取：%1").arg(relativePath);
        }
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

bool LocalLogCatalog::canOpen(const QString &relativePath)
{
    const QString absolute = absolutePath(relativePath);
    return !absolute.isEmpty() && QFile::exists(absolute);
}
