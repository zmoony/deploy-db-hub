#include "infra/JdkProfileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

JdkProfileStore::JdkProfileStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

bool JdkProfileStore::load(QVector<JdkProfile> *profiles, QString *error) const
{
    if (profiles == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("profiles output is null");
        }
        return false;
    }

    profiles->clear();
    QFile file(m_filePath);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        if (error != nullptr) {
            *error = QStringLiteral("JDK profile file must contain an array");
        }
        return false;
    }

    const QJsonArray array = document.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        const JdkProfile profile{
            object.value(QStringLiteral("id")).toString(),
            object.value(QStringLiteral("version")).toString(),
            object.value(QStringLiteral("path")).toString()
        };
        if (!profile.id.isEmpty() && !profile.path.isEmpty()) {
            profiles->append(profile);
        }
    }
    return true;
}

bool JdkProfileStore::save(const QVector<JdkProfile> &profiles, QString *error) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray array;
    for (const JdkProfile &profile : profiles) {
        if (profile.id.trimmed().isEmpty() || profile.path.trimmed().isEmpty()) {
            continue;
        }
        array.append(QJsonObject{
            {QStringLiteral("id"), profile.id.trimmed()},
            {QStringLiteral("version"), profile.version.trimmed()},
            {QStringLiteral("path"), QDir::fromNativeSeparators(profile.path.trimmed())}
        });
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

JdkProfile JdkProfileStore::systemProfile()
{
    return {QStringLiteral("system"), QStringLiteral("系统环境"), QString()};
}

void JdkProfileStore::applyToEnvironment(const JdkProfile &profile, QMap<QString, QString> *environment)
{
    if (environment == nullptr || profile.id == QStringLiteral("system") || profile.path.trimmed().isEmpty()) {
        return;
    }

    const QString normalizedPath = QDir::fromNativeSeparators(profile.path.trimmed());
    environment->insert(QStringLiteral("JAVA_HOME"), normalizedPath);

    const QString pathKey = environment->contains(QStringLiteral("PATH"))
        ? QStringLiteral("PATH")
        : QStringLiteral("Path");
    const QString existingPath = environment->value(pathKey);
#ifdef Q_OS_WIN
    const QChar separator = QLatin1Char(';');
#else
    const QChar separator = QLatin1Char(':');
#endif
    const QString binPath = normalizedPath + QStringLiteral("/bin");
    environment->insert(pathKey,
                        existingPath.isEmpty()
                            ? binPath
                            : binPath + separator + existingPath);
}
