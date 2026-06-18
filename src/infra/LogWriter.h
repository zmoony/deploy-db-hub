#pragma once

#include <QFile>
#include <QString>

class LogWriter final
{
public:
    explicit LogWriter(QString path);

    static LogWriter forDeployment(const QString &dataDir, const QString &deployId);

    bool open(QString *error);
    bool writeLine(const QString &line, QString *error);

private:
    QString m_path;
    QFile m_file;
    qint64 m_bytesWritten = 0;
    bool m_truncated = false;
};
