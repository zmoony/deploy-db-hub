#include "infra/ConfigStore.h"

#include "infra/ConfigValidator.h"

#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

ConfigStore::ConfigStore(QString databasePath)
    : m_databasePath(std::move(databasePath))
    , m_connectionName(QStringLiteral("deploy_hub_%1_%2")
                           .arg(QString::number(qHash(m_databasePath)),
                                QString::number(reinterpret_cast<quintptr>(this), 16)))
{
}

ConfigStore::~ConfigStore()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool ConfigStore::open(QString *error)
{
    QSqlDatabase db = QSqlDatabase::contains(m_connectionName)
        ? QSqlDatabase::database(m_connectionName)
        : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        if (error) {
            *error = db.lastError().text();
        }
        return false;
    }

    QSqlQuery query(db);
    const QStringList statements = {
        QStringLiteral("create table if not exists projects (id text primary key, config_json text not null, updated_at text not null)"),
        QStringLiteral("create table if not exists servers (id text primary key, config_json text not null, updated_at text not null)"),
        QStringLiteral("create table if not exists deployments (id text primary key, project_id text not null, server_id text not null, status text not null, record_json text not null, started_at text not null, finished_at text)"),
        QStringLiteral("create table if not exists settings (key text primary key, value text not null)"),
        QStringLiteral("create table if not exists schema_meta (key text primary key, value text not null)")
    };

    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            if (error) {
                *error = query.lastError().text();
            }
            return false;
        }
    }
    return true;
}

bool ConfigStore::upsertProject(const QString &id, const QJsonObject &config, QString *error)
{
    const ValidationResult validation = ConfigValidator::validateProject(config);
    if (!validation.ok) {
        if (error) {
            *error = validation.errors.join(QStringLiteral("; "));
        }
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("insert into projects(id, config_json, updated_at) values(?, ?, datetime('now')) on conflict(id) do update set config_json=excluded.config_json, updated_at=excluded.updated_at"));
    query.addBindValue(id);
    query.addBindValue(QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact)));
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool ConfigStore::upsertServer(const QString &id, const QJsonObject &config, QString *error)
{
    const ValidationResult validation = ConfigValidator::validateServer(config);
    if (!validation.ok) {
        if (error) {
            *error = validation.errors.join(QStringLiteral("; "));
        }
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("insert into servers(id, config_json, updated_at) values(?, ?, datetime('now')) on conflict(id) do update set config_json=excluded.config_json, updated_at=excluded.updated_at"));
    query.addBindValue(id);
    query.addBindValue(QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact)));
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool ConfigStore::upsertDeployment(const QJsonObject &record, QString *error)
{
    const ValidationResult validation = ConfigValidator::validateDeployment(record);
    if (!validation.ok) {
        if (error) {
            *error = validation.errors.join(QStringLiteral("; "));
        }
        return false;
    }

    const QString id = record.value(QStringLiteral("id")).toString();
    const QString projectId = record.value(QStringLiteral("projectId")).toString();
    const QString serverId = record.value(QStringLiteral("serverId")).toString();
    const QString status = record.value(QStringLiteral("status")).toString();
    const QString startedAt = record.value(QStringLiteral("startedAt")).toString();
    const QJsonValue finishedAtValue = record.value(QStringLiteral("finishedAt"));
    const QString finishedAt = finishedAtValue.isNull() ? QString() : finishedAtValue.toString();

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "insert into deployments(id, project_id, server_id, status, record_json, started_at, finished_at) "
        "values(?, ?, ?, ?, ?, ?, nullif(?, '')) "
        "on conflict(id) do update set "
        "project_id=excluded.project_id, server_id=excluded.server_id, status=excluded.status, "
        "record_json=excluded.record_json, started_at=excluded.started_at, finished_at=excluded.finished_at"));
    query.addBindValue(id);
    query.addBindValue(projectId);
    query.addBindValue(serverId);
    query.addBindValue(status);
    query.addBindValue(QString::fromUtf8(QJsonDocument(record).toJson(QJsonDocument::Compact)));
    query.addBindValue(startedAt);
    query.addBindValue(finishedAt);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool ConfigStore::listProjects(QVector<StoredRecord> *records, QString *error) const
{
    if (!records) {
        if (error) {
            *error = QStringLiteral("records output is null");
        }
        return false;
    }

    records->clear();
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("select id, config_json from projects order by updated_at desc"))) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        const QJsonDocument doc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
        records->append({query.value(0).toString(), doc.object()});
    }
    return true;
}

bool ConfigStore::listDeployments(QVector<StoredRecord> *records, QString *error) const
{
    if (!records) {
        if (error) {
            *error = QStringLiteral("records output is null");
        }
        return false;
    }

    records->clear();
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("select id, record_json from deployments order by started_at desc"))) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        const QJsonDocument doc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
        records->append({query.value(0).toString(), doc.object()});
    }
    return true;
}

bool ConfigStore::getLatestDeploymentForProject(const QString &projectId, QJsonObject *record, QString *error) const
{
    if (!record) {
        if (error) {
            *error = QStringLiteral("record output is null");
        }
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "select record_json from deployments where project_id = ? order by started_at desc limit 1"));
    query.addBindValue(projectId);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    if (!query.next()) {
        if (error) {
            *error = QStringLiteral("deployment not found");
        }
        return false;
    }

    *record = QJsonDocument::fromJson(query.value(0).toString().toUtf8()).object();
    return true;
}

bool ConfigStore::listServers(QVector<StoredRecord> *records, QString *error) const
{
    if (!records) {
        if (error) {
            *error = QStringLiteral("records output is null");
        }
        return false;
    }

    records->clear();
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("select id, config_json from servers order by updated_at desc"))) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        const QJsonDocument doc = QJsonDocument::fromJson(query.value(1).toString().toUtf8());
        records->append({query.value(0).toString(), doc.object()});
    }
    return true;
}

bool ConfigStore::getProject(const QString &id, QJsonObject *config, QString *error) const
{
    if (!config) {
        if (error) {
            *error = QStringLiteral("config output is null");
        }
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("select config_json from projects where id = ?"));
    query.addBindValue(id);
    if (!query.exec() || !query.next()) {
        if (error) {
            *error = QStringLiteral("project not found");
        }
        return false;
    }

    *config = QJsonDocument::fromJson(query.value(0).toString().toUtf8()).object();
    return true;
}

bool ConfigStore::getServer(const QString &id, QJsonObject *config, QString *error) const
{
    if (!config) {
        if (error) {
            *error = QStringLiteral("config output is null");
        }
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("select config_json from servers where id = ?"));
    query.addBindValue(id);
    if (!query.exec() || !query.next()) {
        if (error) {
            *error = QStringLiteral("server not found");
        }
        return false;
    }

    *config = QJsonDocument::fromJson(query.value(0).toString().toUtf8()).object();
    return true;
}

bool ConfigStore::deleteProject(const QString &id, QString *error)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("delete from projects where id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ConfigStore::deleteServer(const QString &id, QString *error)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("delete from servers where id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }
    return query.numRowsAffected() > 0;
}
