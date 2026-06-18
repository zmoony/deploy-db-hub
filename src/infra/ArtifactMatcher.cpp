#include "infra/ArtifactMatcher.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

namespace {
QString globToRegex(const QString &glob)
{
    QString output;
    output.reserve(glob.size() * 2);
    for (const QChar ch : glob) {
        if (ch == QLatin1Char('*')) {
            output += QStringLiteral("[^/\\\\]*");
        } else if (ch == QLatin1Char('?')) {
            output += QStringLiteral("[^/\\\\]");
        } else if (QStringLiteral(".+()^${}|[]").contains(ch)) {
            output += QLatin1Char('\\');
            output += ch;
        } else if (ch == QLatin1Char('\\')) {
            output += QStringLiteral("[/\\\\]");
        } else if (ch == QLatin1Char('/')) {
            output += QStringLiteral("[/\\\\]");
        } else {
            output += ch;
        }
    }
    return QStringLiteral("^") + output + QStringLiteral("$");
}

QString normalizeRelativePath(const QString &root, const QString &path)
{
    return QDir(root).relativeFilePath(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
}
}

ArtifactMatchResult ArtifactMatcher::match(const QString &projectRoot, const QString &artifactPath, ArtifactMatchPolicy policy)
{
    const QDir root(projectRoot);
    ArtifactMatchResult result;

    if (!root.exists()) {
        result.error = QStringLiteral("项目目录不存在");
        return result;
    }

    const QRegularExpression regex(globToRegex(artifactPath));
    QStringList matches;

    QDirIterator iterator(projectRoot, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString absolutePath = iterator.next();
        const QString relativePath = normalizeRelativePath(projectRoot, absolutePath);
        if (regex.match(relativePath).hasMatch()) {
            matches.append(QFileInfo(absolutePath).absoluteFilePath());
        }
    }

    if (matches.isEmpty()) {
        result.error = QStringLiteral("未找到构建产物");
        return result;
    }

    if (matches.size() > 1 && policy == ArtifactMatchPolicy::FailIfMultiple) {
        result.error = QStringLiteral("匹配到多个构建产物: ") + matches.join(QStringLiteral(", "));
        return result;
    }

    if (matches.size() > 1 && policy == ArtifactMatchPolicy::Newest) {
        std::sort(matches.begin(), matches.end(), [](const QString &left, const QString &right) {
            return QFileInfo(left).lastModified() > QFileInfo(right).lastModified();
        });
        matches = QStringList{matches.first()};
    }

    result.ok = true;
    result.paths = matches;
    return result;
}
