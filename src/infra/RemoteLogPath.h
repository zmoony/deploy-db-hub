#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct RemoteLogGlobSpec {
    QString directory;
    QString filePattern;
    bool isGlob() const { return !filePattern.isEmpty(); }
};

QString normalizedRemoteLogPath(QString path);
QString defaultRemoteLogPath(const QJsonObject &project);
QString remoteBaseDirFromProject(const QJsonObject &project);
QStringList defaultRemoteLogGlobPatterns();
QStringList candidateRemoteLogDirectories(const QJsonObject &project);
QStringList buildRemoteLogPathOptions(const QStringList &directories, const QStringList &patterns);
QStringList deployLogPathOptionsForProject(const QJsonObject &project);
RemoteLogGlobSpec parseRemoteLogGlobPath(const QString &path);
bool isRemoteDeployLogPath(const QString &path);
QString remoteDiscoverLogDirectoriesCommand(const QJsonObject &server, const QString &remoteBaseDir);
QString remoteDiscoverLogFilesCommand(const QJsonObject &server, const QStringList &directories);
QStringList parseDiscoveredLogDirectories(const QString &output);
QStringList parseDiscoveredLogFiles(const QString &output);
QStringList mergeRemoteLogDirectories(const QJsonObject &project, const QStringList &discovered);
QString sshTailLatestMatchingFileCommand(const RemoteLogGlobSpec &spec, int lineCount);
QString winRmTailLatestMatchingFileCommand(const RemoteLogGlobSpec &spec, int lineCount);
