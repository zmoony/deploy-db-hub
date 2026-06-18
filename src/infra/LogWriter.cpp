#include "infra/LogWriter.h"

#include "infra/LogPath.h"
#include "infra/LogSanitizer.h"

#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include <utility>

namespace {
constexpr qint64 MaxLogBytes = 50LL * 1024LL * 1024LL;
}

LogWriter::LogWriter(QString path)
    : m_path(std::move(path))
    , m_file(m_path)
{
}

LogWriter LogWriter::forDeployment(const QString &dataDir, const QString &deployId)
{
    return LogWriter(LogPath::absolutePath(dataDir, LogPath::relativePath(deployId)));
}

bool LogWriter::open(QString *error)
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        if (error) {
            *error = m_file.errorString();
        }
        return false;
    }
    m_bytesWritten = m_file.size();
    return true;
}

bool LogWriter::writeLine(const QString &line, QString *error)
{
    if (!m_file.isOpen()) {
        if (error) {
            *error = QStringLiteral("日志文件未打开");
        }
        return false;
    }
    if (m_truncated) {
        return true;
    }

    const QByteArray bytes = (LogSanitizer::sanitize(line) + QStringLiteral("\n")).toUtf8();
    if (m_bytesWritten + bytes.size() > MaxLogBytes) {
        m_file.write("=== LOG TRUNCATED ===\n");
        m_truncated = true;
        return true;
    }

    if (m_file.write(bytes) != bytes.size()) {
        if (error) {
            *error = m_file.errorString();
        }
        return false;
    }

    m_bytesWritten += bytes.size();
    return true;
}
