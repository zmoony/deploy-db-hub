#include "adapters/remote/StubRemoteVfs.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

QString joinPath(const QString &parent, const QString &name)
{
    if (parent == QStringLiteral("/")) {
        return QStringLiteral("/") + name;
    }
    return parent + QLatin1Char('/') + name;
}

}

StubRemoteVfs::StubRemoteVfs(const QJsonObject &serverConfig)
    : m_serverConfig(serverConfig)
{
    seedLinuxLayout();
}

QString StubRemoteVfs::normalizePath(const QString &path) const
{
    QString normalized = path.trimmed();
    if (normalized.isEmpty()) {
        normalized = QStringLiteral("/");
    }
    if (!normalized.startsWith(QLatin1Char('/'))) {
        normalized.prepend(QLatin1Char('/'));
    }
    while (normalized.size() > 1 && normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized;
}

QString StubRemoteVfs::parentPath(const QString &path) const
{
    const QString normalized = normalizePath(path);
    if (normalized == QStringLiteral("/")) {
        return QStringLiteral("/");
    }
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    return slash <= 0 ? QStringLiteral("/") : normalized.left(slash);
}

QString StubRemoteVfs::baseName(const QString &path) const
{
    const QString normalized = normalizePath(path);
    if (normalized == QStringLiteral("/")) {
        return QStringLiteral("/");
    }
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    return normalized.mid(slash + 1);
}

bool StubRemoteVfs::hasNode(const QString &path) const
{
    return m_nodes.contains(normalizePath(path));
}

StubRemoteVfs::Node *StubRemoteVfs::node(const QString &path)
{
    const QString key = normalizePath(path);
    auto it = m_nodes.find(key);
    return it == m_nodes.end() ? nullptr : &it.value();
}

const StubRemoteVfs::Node *StubRemoteVfs::node(const QString &path) const
{
    const QString key = normalizePath(path);
    auto it = m_nodes.constFind(key);
    return it == m_nodes.constEnd() ? nullptr : &it.value();
}

void StubRemoteVfs::addDirectory(const QString &path)
{
    const QString normalized = normalizePath(path);
    if (hasNode(normalized)) {
        return;
    }

    Node dir;
    dir.path = normalized;
    dir.name = baseName(normalized);
    dir.type = RemoteFileType::Directory;
    dir.sizeBytes = -1;
    dir.writable = true;
    m_nodes.insert(normalized, dir);

    const QString parent = parentPath(normalized);
    if (parent != normalized) {
        if (!hasNode(parent)) {
            addDirectory(parent);
        }
        Node *parentNode = node(parent);
        if (parentNode != nullptr && !parentNode->children.contains(dir.name)) {
            parentNode->children.append(dir.name);
        }
    }
}

void StubRemoteVfs::addFile(const QString &path, const QString &content, bool writable)
{
    const QString normalized = normalizePath(path);
    const QString parent = parentPath(normalized);
    if (!hasNode(parent)) {
        addDirectory(parent);
    }

    Node file;
    file.path = normalized;
    file.name = baseName(normalized);
    file.type = RemoteFileType::File;
    file.content = content;
    file.sizeBytes = content.toUtf8().size();
    file.writable = writable;
    m_nodes.insert(normalized, file);

    Node *parentNode = node(parent);
    if (parentNode != nullptr && !parentNode->children.contains(file.name)) {
        parentNode->children.append(file.name);
    }
}

void StubRemoteVfs::seedLinuxLayout()
{
    const QString username = m_serverConfig.value(QStringLiteral("username")).toString(QStringLiteral("deploy"));
    const QString deployRoot = m_serverConfig.value(QStringLiteral("defaultRemoteBaseDir")).toString(QStringLiteral("/opt/deploy-hub"));
    const QString host = m_serverConfig.value(QStringLiteral("host")).toString(QStringLiteral("srv"));

    const QStringList rootDirs = {
        QStringLiteral("bin"), QStringLiteral("boot"), QStringLiteral("dev"), QStringLiteral("etc"),
        QStringLiteral("home"), QStringLiteral("lib"), QStringLiteral("lib64"), QStringLiteral("media"),
        QStringLiteral("mnt"), QStringLiteral("opt"), QStringLiteral("proc"), QStringLiteral("root"),
        QStringLiteral("run"), QStringLiteral("sbin"), QStringLiteral("srv"), QStringLiteral("sys"),
        QStringLiteral("tmp"), QStringLiteral("usr"), QStringLiteral("var")
    };
    addDirectory(QStringLiteral("/"));
    for (const QString &name : rootDirs) {
        addDirectory(joinPath(QStringLiteral("/"), name));
    }

    addDirectory(joinPath(QStringLiteral("/home"), username));
    addDirectory(joinPath(joinPath(QStringLiteral("/home"), username), QStringLiteral(".ssh")));
    addFile(joinPath(joinPath(joinPath(QStringLiteral("/home"), username), QStringLiteral(".ssh")), QStringLiteral("authorized_keys")),
            QStringLiteral("# managed keys\n"), false);

    addDirectory(QStringLiteral("/etc"));
    addFile(QStringLiteral("/etc/hosts"),
            QStringLiteral("127.0.0.1 localhost\n127.0.1.1 %1\n").arg(host), false);
    addFile(QStringLiteral("/etc/os-release"),
            QStringLiteral("NAME=\"Ubuntu\"\nVERSION=\"22.04 LTS (Jammy Jellyfish)\"\n"), false);

    addDirectory(QStringLiteral("/var"));
    addDirectory(QStringLiteral("/var/log"));
    addFile(QStringLiteral("/var/log/syslog"),
            QStringLiteral("[%1] deploy-hub stub syslog preview\n").arg(host), false);
    addFile(QStringLiteral("/var/log/auth.log"),
            QStringLiteral("Accepted publickey for %1 from 10.0.0.8\n").arg(username), false);

    addDirectory(deployRoot);
    addDirectory(joinPath(deployRoot, QStringLiteral("apps")));
    addDirectory(joinPath(deployRoot, QStringLiteral("releases")));
    addDirectory(joinPath(joinPath(deployRoot, QStringLiteral("apps")), QStringLiteral("demo")));
    addFile(joinPath(joinPath(joinPath(deployRoot, QStringLiteral("apps")), QStringLiteral("demo")), QStringLiteral("restart.sh")),
            QStringLiteral("#!/bin/bash\nsystemctl restart demo.service\n"), true);
    addFile(joinPath(joinPath(joinPath(deployRoot, QStringLiteral("apps")), QStringLiteral("demo")), QStringLiteral("application.yml")),
            QStringLiteral("server:\n  port: 8080\nspring:\n  profiles:\n    active: prod\n"), true);
    addFile(joinPath(joinPath(joinPath(deployRoot, QStringLiteral("apps")), QStringLiteral("demo")), QStringLiteral("app.log")),
            QStringLiteral("[INFO] demo service started\n[INFO] listening on 8080\n"), true);

    addDirectory(QStringLiteral("/tmp"));
    addDirectory(QStringLiteral("/tmp/deploy-hub-uploads"));
}

RemoteFileListResult StubRemoteVfs::listDirectory(const QString &remotePath) const
{
    RemoteFileListResult result;
    result.currentPath = normalizePath(remotePath);
    const Node *dir = node(result.currentPath);
    if (dir == nullptr || dir->type != RemoteFileType::Directory) {
        result.error = QStringLiteral("directory does not exist: %1").arg(result.currentPath);
        return result;
    }

    result.entries.append({QStringLiteral("."), result.currentPath, RemoteFileType::Directory, -1, true});
    if (result.currentPath != QStringLiteral("/")) {
        result.entries.append({QStringLiteral(".."), parentPath(result.currentPath), RemoteFileType::Directory, -1, true});
    }

    QStringList names = dir->children;
    names.sort(Qt::CaseInsensitive);
    for (const QString &name : names) {
        const QString childPath = joinPath(result.currentPath, name);
        const Node *child = node(childPath);
        if (child == nullptr) {
            continue;
        }
        RemoteFileEntry entry;
        entry.name = child->name;
        entry.path = child->path;
        entry.type = child->type;
        entry.sizeBytes = child->sizeBytes;
        entry.writable = child->writable;
        result.entries.append(entry);
    }

    result.ok = true;
    return result;
}

RemoteFileReadResult StubRemoteVfs::readFile(const QString &remotePath) const
{
    RemoteFileReadResult result;
    result.remotePath = normalizePath(remotePath);
    const Node *file = node(result.remotePath);
    if (file == nullptr || file->type != RemoteFileType::File) {
        result.error = QStringLiteral("file does not exist: %1").arg(result.remotePath);
        return result;
    }
    result.content = file->content;
    result.ok = true;
    return result;
}

RemoteFileReadResult StubRemoteVfs::readFileTail(const QString &remotePath, int lineCount) const
{
    RemoteFileReadResult result = readFile(remotePath);
    if (!result.ok) {
        return result;
    }

    const int count = qMax(1, lineCount);
    const QStringList lines = result.content.split(QLatin1Char('\n'));
    if (lines.size() <= count) {
        return result;
    }
    result.content = lines.mid(lines.size() - count).join(QLatin1Char('\n'));
    return result;
}

RemoteFileWriteResult StubRemoteVfs::writeFile(const QString &remotePath, const QString &content)
{
    RemoteFileWriteResult result;
    result.remotePath = normalizePath(remotePath);
    Node *file = node(result.remotePath);
    if (file == nullptr || file->type != RemoteFileType::File) {
        result.error = QStringLiteral("file does not exist: %1").arg(result.remotePath);
        return result;
    }
    if (!file->writable) {
        result.error = QStringLiteral("file is read-only: %1").arg(result.remotePath);
        return result;
    }
    file->content = content;
    file->sizeBytes = content.toUtf8().size();
    result.ok = true;
    return result;
}

RemoteFileOperationResult StubRemoteVfs::createDirectory(const QString &remotePath)
{
    RemoteFileOperationResult result;
    const QString normalized = normalizePath(remotePath);
    if (hasNode(normalized)) {
        result.error = QStringLiteral("path already exists: %1").arg(normalized);
        return result;
    }
    addDirectory(normalized);
    result.ok = true;
    return result;
}

RemoteFileUploadResult StubRemoteVfs::uploadFile(const QString &localPath, const QString &remotePath)
{
    RemoteFileUploadResult result;
    QFileInfo localFile(localPath);
    if (!localFile.exists() || !localFile.isFile()) {
        result.error = QStringLiteral("local file does not exist: %1").arg(localPath);
        return result;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("cannot read local file: %1").arg(localPath);
        return result;
    }

    const QString normalized = normalizePath(remotePath);
    const QByteArray bytes = file.readAll();
    if (hasNode(normalized)) {
        Node *existing = node(normalized);
        if (existing != nullptr && existing->type == RemoteFileType::File) {
            existing->content = QString::fromUtf8(bytes);
            existing->sizeBytes = bytes.size();
            result.remotePath = normalized;
            result.totalBytes = bytes.size();
            result.bytesSent = bytes.size();
            result.ok = true;
            return result;
        }
        result.error = QStringLiteral("remote path already exists: %1").arg(normalized);
        return result;
    }

    addFile(normalized, QString::fromUtf8(bytes), true);
    result.remotePath = normalized;
    result.totalBytes = bytes.size();
    result.bytesSent = bytes.size();
    result.ok = true;
    return result;
}

RemoteFileOperationResult StubRemoteVfs::deleteEntry(const QString &remotePath)
{
    RemoteFileOperationResult result;
    const QString normalized = normalizePath(remotePath);
    if (normalized == QStringLiteral("/")) {
        result.error = QStringLiteral("cannot delete root directory");
        return result;
    }

    Node *target = node(normalized);
    if (target == nullptr) {
        result.error = QStringLiteral("path does not exist: %1").arg(normalized);
        return result;
    }
    if (target->type == RemoteFileType::Directory && !target->children.isEmpty()) {
        result.error = QStringLiteral("directory is not empty: %1").arg(normalized);
        return result;
    }

    Node *parent = node(parentPath(normalized));
    if (parent != nullptr) {
        parent->children.removeAll(target->name);
    }
    m_nodes.remove(normalized);
    result.ok = true;
    return result;
}
