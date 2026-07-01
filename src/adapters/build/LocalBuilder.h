#pragma once

#include <QMap>
#include <QString>

struct BuildRequest {
    QString workingDirectory;
    QString command;
    QMap<QString, QString> environment;
    int timeoutSec = 600;
    std::function<bool()> shouldCancel = {};
};

struct BuildResult {
    bool ok = false;
    int exitCode = -1;
    QString stdoutText;
    QString stderrText;
    QString error;
};

class LocalBuilder final
{
public:
    BuildResult run(const BuildRequest &request) const;
};
