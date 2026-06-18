#pragma once

#include "adapters/remote/RemoteFileBrowser.h"

#include <QHash>
#include <QJsonObject>
#include <QStringList>

class StubRemoteVfs final
{
public:
    explicit StubRemoteVfs(const QJsonObject &serverConfig);

    QString normalizePath(const QString &path) const;
    RemoteFileListResult listDirectory(const QString &remotePath) const;
    RemoteFileReadResult readFile(const QString &remotePath) const;
    RemoteFileReadResult readFileTail(const QString &remotePath, int lineCount) const;
    RemoteFileWriteResult writeFile(const QString &remotePath, const QString &content);
    RemoteFileOperationResult createDirectory(const QString &remotePath);
    RemoteFileUploadResult uploadFile(const QString &localPath, const QString &remotePath);
    RemoteFileOperationResult deleteEntry(const QString &remotePath);

private:
    struct Node {
        QString path;
        QString name;
        RemoteFileType type = RemoteFileType::Unknown;
        qint64 sizeBytes = -1;
        bool writable = true;
        QString content;
        QStringList children;
    };

    void seedLinuxLayout();
    void addDirectory(const QString &path);
    void addFile(const QString &path, const QString &content, bool writable = true);
    bool hasNode(const QString &path) const;
    Node *node(const QString &path);
    const Node *node(const QString &path) const;
    QString parentPath(const QString &path) const;
    QString baseName(const QString &path) const;

    QJsonObject m_serverConfig;
    QHash<QString, Node> m_nodes;
};
